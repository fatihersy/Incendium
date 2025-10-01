#include "camera.h"
#include <settings.h>
#include <reasings.h>

#include "core/fmath.h"
#include "core/fmemory.h"
#include "core/event.h"
#include "core/logger.h"

#define CAMERA_SHAKE_DIRECTION (Vector2 {.01f, 1.f})
#define CAMERA_SHAKE_NOISE_SEED 555u
#define CAMERA_SHAKE_NOISE_SPEED 2.f
#define CAMERA_SHAKE_MAX_MAGNITUDE 4.f
#define CAMERA_SHAKE_NOISE_MAGNITUDE 2.25f
#define CAMERA_SHAKE_DURATION .085f

#define PERLIN_NOISE_L 256

#define ZOOM_CAMERA_DURATION 0.8f

#ifdef _DEBUG
  #define ZOOM_CAMERA_MIN 0.01f
#else
  #define ZOOM_CAMERA_MIN 0.85f
#endif

#ifdef _DEBUG
  #define ZOOM_CAMERA_MAX 2.0f
#else
  #define ZOOM_CAMERA_MAX 1.5f
#endif

#define ZOOM_CAMERA_DEFAULT 1.f

typedef struct camera_shake_control_system {
  Image perlin_noise_img;
  Color* noise_data;

  Vector2 direction;
  u16 seed;
  f32 speed;
  f32 max_magnitude;
  f32 noise_magnitude;
  f32 duration;
  f32 accumulator;
  bool is_active;
  camera_shake_control_system(void) {
    this->perlin_noise_img = ZERO_IMAGE;
    this->noise_data = nullptr;

    this->direction = CAMERA_SHAKE_DIRECTION;
    this->seed = CAMERA_SHAKE_NOISE_SEED;
    this->speed = CAMERA_SHAKE_NOISE_SPEED;
    this->max_magnitude = CAMERA_SHAKE_MAX_MAGNITUDE;
    this->noise_magnitude = CAMERA_SHAKE_NOISE_MAGNITUDE;
    this->duration = CAMERA_SHAKE_DURATION; 
    this->accumulator = 0.f;
    this->is_active = false;
  }
} camera_shake_control_system;

typedef struct camera_zoom_control_system {
  f32 accumulator;
  f32 camera_old_zoom;
  f32 camera_target_zoom;
  bool is_active;

  camera_zoom_control_system(void) {
    this->accumulator = 0.f;
    this->camera_target_zoom = 0.f;
    this->camera_old_zoom = 0.f;
    this->is_active = false;
  }
  camera_zoom_control_system(f32 old_zoom, f32 new_zoom) {
    this->camera_target_zoom = new_zoom;
    this->camera_old_zoom = old_zoom;
    this->is_active = true;
  }
} camera_zoom_control_system;

typedef struct camera_system_state {
  camera_metrics cam_met;

  Vector2 offset;
  Vector2 drawing_extent;
  Vector2 position;
  
  f32 camera_min_speed;
  f32 camera_min_effect_lenght;
  f32 camera_fraction_speed;
  camera_shake_control_system shake_ctrl;
  camera_zoom_control_system zoom_ctrl;

  camera_system_state(void) {
    this->cam_met = camera_metrics();
    this->offset = ZEROVEC2;
    this->drawing_extent = ZEROVEC2;
    this->position = ZEROVEC2;
    this->camera_min_speed = 0.f;
    this->camera_min_effect_lenght = 0.f;
    this->camera_fraction_speed = 0.f;
    this->shake_ctrl = camera_shake_control_system();
    this->zoom_ctrl = camera_zoom_control_system();
  }
} camera_system_state;

static camera_system_state * state = nullptr;

bool camera_on_event(i32 code, event_context context);
bool recreate_camera(i32 target_x, i32 target_y, i32 render_width, i32 render_height);
Vector2 get_perlin_2d(u16 seed);
Vector2 update_camera_shake(void);
void set_camera_zoom(f32 value);

bool create_camera(i32 target_x, i32 target_y, i32 render_width, i32 render_height) {
  if (state and state != nullptr) {
    return recreate_camera(target_x, target_y, render_width, render_height);
  }
  state = (camera_system_state *)allocate_memory_linear(sizeof(camera_system_state), true);
  if (not state or state == nullptr) {
    IERROR("camera::create_camera()::State allocation failed");
    return false;
  }
  *state = camera_system_state();
  event_register(EVENT_CODE_CAMERA_SET_FRUSTUM, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_OFFSET, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_ZOOM, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_ZOOM_TARGET, camera_on_event);
  event_register(EVENT_CODE_CAMERA_ADD_ZOOM, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_TARGET, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, camera_on_event);
  event_register(EVENT_CODE_CAMERA_SET_DRAWING_EXTENT, camera_on_event);
  event_register(EVENT_CODE_BEGIN_CAMERA_SHAKE, camera_on_event);

  state->shake_ctrl = camera_shake_control_system();
  camera_shake_control_system& shake = state->shake_ctrl;

  shake.perlin_noise_img = GenImagePerlinNoise(PERLIN_NOISE_L, PERLIN_NOISE_L, 0, 0, 5.f);
  shake.noise_data = reinterpret_cast<Color*>(shake.perlin_noise_img.data);

  return recreate_camera(target_x, target_y, render_width, render_height);
}

bool recreate_camera(i32 width, i32 height, i32 render_width, i32 render_height) {
  if (not state or state == nullptr) {
    IERROR("camera::recreate_camera()::State is invalid");
    return false;
  }
  state->cam_met.handle.target = Vector2 {static_cast<f32>(width), static_cast<f32>(height)};
  state->cam_met.handle.rotation = 0;
  state->cam_met.handle.zoom = 1.0f;
  state->zoom_ctrl.camera_target_zoom = 1.f;
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
  if (not state or state == nullptr) {
    IERROR("camera::get_in_game_camera()::State is invalid");
    return nullptr;
  }
  return __builtin_addressof(state->cam_met);
}

bool update_camera(void) {
  Vector2 diff = vec2_subtract(state->position, state->cam_met.handle.target);
  float length = vec2_lenght(diff);

  if (state->zoom_ctrl.is_active) {
    camera_zoom_control_system& zoom_ctrl = state->zoom_ctrl;

    set_camera_zoom(EaseCircOut(zoom_ctrl.accumulator, zoom_ctrl.camera_old_zoom, zoom_ctrl.camera_target_zoom - zoom_ctrl.camera_old_zoom, ZOOM_CAMERA_DURATION));

    if (zoom_ctrl.accumulator >= ZOOM_CAMERA_DURATION) {
      set_camera_zoom(zoom_ctrl.camera_target_zoom);
      zoom_ctrl = camera_zoom_control_system();
    }
    else {
      zoom_ctrl.accumulator += GetFrameTime();
    }
  }

  if (length > state->camera_min_effect_lenght) {
    float speed = FMAX(state->camera_fraction_speed * length, state->camera_min_speed);

    state->cam_met.handle.target = vec2_add(state->cam_met.handle.target, vec2_scale(diff, speed * GetFrameTime() / length));

    if (state->shake_ctrl.is_active) {
      Vector2 shake = update_camera_shake();
      state->cam_met.handle.target.x += shake.x;
      state->cam_met.handle.target.y += shake.y;
    }
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

Vector2 update_camera_shake(void) {
  if (not state or state == nullptr) {
    IERROR("camera::update_camera_shake()::State is not valid");
    return ZEROVEC2;
  }
  camera_shake_control_system& shake = state->shake_ctrl;

  if (not shake.noise_data or shake.noise_data == nullptr) {
    IWARN("camera::update_camera_shake()::Noise data is not valid");
    return ZEROVEC2;
  }
  if (shake.accumulator >= shake.duration) {
    shake.is_active = false;
    return ZEROVEC2;
  }
  shake.accumulator += GetFrameTime();

  f32 sin = static_cast<f32>(fast_sin(static_cast<f64>(shake.speed) * (static_cast<f64>(shake.seed) + GetTime())));
  Vector2 noise = vec2_scale(get_perlin_2d(shake.seed), shake.noise_magnitude);
  Vector2 dir = Vector2 {
    shake.direction.x * noise.x,
    shake.direction.y * noise.y,
  };
  Vector2 nd = vec2_normalize(dir);
  
  Vector2 mdl = vec2_scale(nd, sin);
  Vector2 amp = vec2_scale(mdl, shake.max_magnitude);
  Vector2 att = vec2_scale(amp, (shake.duration - shake.accumulator) / shake.duration);
  return att;
}

Vector2 get_perlin_2d(u16 seed) {
  if (not state or state == nullptr) {
    IERROR("camera::get_perlin_2d()::State is not valid");
    return ZEROVEC2;
  }
  camera_shake_control_system& shake = state->shake_ctrl;

  if (not shake.noise_data or shake.noise_data == nullptr) {
    IWARN("camera::get_perlin_2d()::Noise data is not valid");
    return ZEROVEC2;
  }

  f32 val = static_cast<f32>(255.f / shake.noise_data[seed].r);

  return Vector2 { val, val };
}

void set_camera_zoom(f32 value) {
  if (not state or state == nullptr) {
    IERROR("camera::set_camera_zoom()::State is invalid");
    return;
  }
  state->cam_met.handle.zoom = value;
  state->cam_met.handle.zoom = FCLAMP(state->cam_met.handle.zoom, ZOOM_CAMERA_MIN, ZOOM_CAMERA_MAX);
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
      set_camera_zoom(context.data.f32[0]);
      return true;
    }
    case EVENT_CODE_CAMERA_SET_ZOOM_TARGET: {
      f32 new_zoom = FCLAMP(context.data.f32[0], ZOOM_CAMERA_MIN, ZOOM_CAMERA_MAX);
      state->zoom_ctrl = camera_zoom_control_system(state->cam_met.handle.zoom, new_zoom);
      return true;
    }
    case EVENT_CODE_CAMERA_ADD_ZOOM: {
      set_camera_zoom(state->cam_met.handle.zoom + context.data.f32[0]);
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
    case EVENT_CODE_BEGIN_CAMERA_SHAKE: {
      if (state->shake_ctrl.is_active) {
        return true;
      }

      state->shake_ctrl.is_active = true;
      state->shake_ctrl.accumulator = 0.f; 
      return true;
    }
    default: {
      IWARN("camera::camera_on_event()::Unsupported code");
      return false;
    }
  }

  IERROR("camera::camera_on_event()::Function ended unexpectedly");
  return false;
}

