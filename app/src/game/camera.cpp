#include "camera.h"
#include <settings.h>

#include "core/fmath.h"
#include "core/fmemory.h"
#include "core/event.h"

typedef struct main_camera_system_state {
  camera_metrics camera_metrics;

  Vector2 offset;
  Vector2 position;

  float camera_min_speed;
  float camera_min_effect_lenght;
  float camera_fraction_speed;
} main_camera_system_state;

static main_camera_system_state * state;

bool camera_on_event(i32 code, event_context context);

bool create_camera(i32 width, i32 height) {
  if (state) {
    TraceLog(LOG_WARNING, "camera::create_camera()::Called twice");
    return false;
  }

  state = (main_camera_system_state *)allocate_memory_linear(sizeof(main_camera_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "camera::create_camera()::State allocation failed");
    return false;
  }

  state->camera_metrics.handle.target = Vector2 {static_cast<f32>(width), static_cast<f32>(height)};
  state->camera_metrics.handle.rotation = 0;
  state->camera_metrics.handle.zoom = 1.0f;
  state->camera_min_speed = 30;
  state->camera_min_effect_lenght = 10;
  state->camera_fraction_speed = 5.f;

  event_register(EVENT_CODE_CAMERA_SET_FRUSTUM, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_OFFSET, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_ZOOM, camera_on_event);
  event_register(EVENT_CODE_CAMERA_ADD_ZOOM, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_TARGET, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, camera_on_event);

  return true;
}

const camera_metrics* get_in_game_camera(void) { return &state->camera_metrics; }

bool update_camera(void) {
  Vector2 diff = vec2_subtract(state->position, state->camera_metrics.handle.target);
  float length = vec2_lenght(diff);

  if (length > state->camera_min_effect_lenght) {
    float speed = FMAX(state->camera_fraction_speed * length, state->camera_min_speed);
    state->camera_metrics.handle.target = vec2_add(state->camera_metrics.handle.target, vec2_scale(diff, speed * GetFrameTime() / length));
  }

  return true;
}

bool camera_on_event(i32 code, event_context context) {
  switch (code) {
    case EVENT_CODE_CAMERA_SET_FRUSTUM: {
      state->camera_metrics.frustum = Rectangle {
        static_cast<f32>(context.data.i32[0]),
        static_cast<f32>(context.data.i32[1]),
        static_cast<f32>(context.data.i32[2]),
        static_cast<f32>(context.data.i32[3])
      };
      return true;
    }
    case EVENT_CODE_CAMERA_SET_OFFSET: {
      state->camera_metrics.handle.offset = Vector2 {
        context.data.f32[0],
        context.data.f32[1]
      };
      return true;
    }
    case EVENT_CODE_CAMERA_SET_ZOOM: {
      state->camera_metrics.handle.zoom = context.data.f32[0];
      return true;
    }
    case EVENT_CODE_CAMERA_ADD_ZOOM: {
      state->camera_metrics.handle.zoom += context.data.f32[0];
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
      state->camera_metrics.handle.target = Vector2 {
        context.data.f32[0],
        context.data.f32[1]
      };
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

