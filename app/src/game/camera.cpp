#include "camera.h"
#include <settings.h>

#include "core/fmath.h"
#include "core/fmemory.h"
#include "core/event.h"

typedef struct camera_system_state {
  camera_metrics cam_met;

  Vector2 offset;
  Vector2 position;
  Vector2 drawing_extent;

  float camera_min_speed;
  float camera_min_effect_lenght;
  float camera_fraction_speed;
  float padding;
  camera_system_state(void) {
    this->cam_met = camera_metrics();
    this->offset = ZEROVEC2;
    this->position = ZEROVEC2;
    this->drawing_extent = ZEROVEC2;
    this->camera_min_speed = 0.f;
    this->camera_min_effect_lenght = 0.f;
    this->camera_fraction_speed = 0.f;
  }
} camera_system_state;

static camera_system_state * state = nullptr;

bool camera_on_event(i32 code, event_context context);
bool recreate_camera(i32 target_x, i32 target_y, i32 render_width, i32 render_height);

bool create_camera(i32 target_x, i32 target_y, i32 render_width, i32 render_height) {
  if (state) {
    return recreate_camera(target_x, target_y, render_width, render_height);
  }

  state = (camera_system_state *)allocate_memory_linear(sizeof(camera_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "camera::create_camera()::State allocation failed");
    return false;
  }
  *state = camera_system_state();
  event_register(EVENT_CODE_CAMERA_SET_FRUSTUM, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_OFFSET, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_ZOOM, camera_on_event);
  event_register(EVENT_CODE_CAMERA_ADD_ZOOM, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_TARGET, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_DRAWING_EXTENT, camera_on_event);

  return recreate_camera(target_x, target_y, render_width, render_height);
}

bool recreate_camera(i32 width, i32 height, i32 render_width, i32 render_height) {
  if (!state) {
    return false;
  }
  state->cam_met.handle.target = Vector2 {static_cast<f32>(width), static_cast<f32>(height)};
  state->cam_met.handle.rotation = 0;
  state->cam_met.handle.zoom = 1.0f;
  state->camera_min_speed = 30;
  state->camera_min_effect_lenght = 10;
  state->camera_fraction_speed = 5.f;
  state->drawing_extent.x = render_width;
  state->drawing_extent.y = render_height;
  state->offset.x = render_width * .5f;
  state->offset.y = render_height * .5f;

  return true;
}


const camera_metrics* get_in_game_camera(void) {
  if (!state || state == nullptr) {
    return nullptr;
  }
  return __builtin_addressof(state->cam_met); 
}

bool update_camera(void) {
  Vector2 diff = vec2_subtract(state->position, state->cam_met.handle.target);
  float length = vec2_lenght(diff);

  if (length > state->camera_min_effect_lenght) {
    float speed = FMAX(state->camera_fraction_speed * length, state->camera_min_speed);
    state->cam_met.handle.target = vec2_add(state->cam_met.handle.target, vec2_scale(diff, speed * GetFrameTime() / length));
  }

  f32 view_width = state->drawing_extent.x / state->cam_met.handle.zoom;
  f32 view_height = state->drawing_extent.y / state->cam_met.handle.zoom;

  f32 x = state->cam_met.handle.target.x;
  f32 y = state->cam_met.handle.target.y;

  x -= state->cam_met.handle.offset.x / state->cam_met.handle.zoom;
  y -= state->cam_met.handle.offset.y / state->cam_met.handle.zoom;

  state->cam_met.frustum = Rectangle { x, y, view_width, view_height };
  return true;
}

bool camera_on_event(i32 code, event_context context) {
  switch (code) {
    case EVENT_CODE_CAMERA_SET_FRUSTUM: {
      state->cam_met.frustum = Rectangle {
        static_cast<f32>(context.data.i32[0]),
        static_cast<f32>(context.data.i32[1]),
        static_cast<f32>(context.data.i32[2]),
        static_cast<f32>(context.data.i32[3])
      };
      return true;
    }
    case EVENT_CODE_CAMERA_SET_OFFSET: {
      state->cam_met.handle.offset = Vector2 {
        context.data.f32[0],
        context.data.f32[1]
      };
      return true;
    }
    case EVENT_CODE_CAMERA_SET_ZOOM: {
      state->cam_met.handle.zoom = context.data.f32[0];
      return true;
    }
    case EVENT_CODE_CAMERA_ADD_ZOOM: {
      state->cam_met.handle.zoom += context.data.f32[0];
      return true;
    }
    case EVENT_CODE_CAMERA_SET_TARGET: {
      state->position = Vector2 {
        context.data.f32[0],
        context.data.f32[1]
      };
      return true;
    }
    case EVENT_CODE_CAMERA_SET_CAMERA_POSITION: {
      state->cam_met.handle.target.x = context.data.f32[0];
      state->cam_met.handle.target.y = context.data.f32[1];
      state->position.x = context.data.f32[0];
      state->position.y = context.data.f32[1];
      return true;
    }
    case EVENT_CODE_CAMERA_SET_DRAWING_EXTENT: {
      state->drawing_extent.x = context.data.i32[0];
      state->drawing_extent.y = context.data.i32[1];
      return true;
    }
    default: {
      TraceLog(LOG_WARNING, "camera::camera_on_event()::Unknown code");
      return false;
    }
  }

  TraceLog(LOG_ERROR, "camera::camera_on_event()::Fire event ended unexpectedly");
  return false;
}

