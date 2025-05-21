#include "camera.h"
#include <settings.h>

#include "core/fmath.h"
#include "core/fmemory.h"


typedef struct main_camera_system_state {
  float camera_min_speed;
  float camera_min_effect_lenght;
  float camera_fraction_speed;

  camera_metrics in_camera_metrics;
} main_camera_system_state;

static main_camera_system_state *state;

bool create_camera(Vector2 position) {
  if (state) {
    TraceLog(LOG_WARNING, "camera::create_camera()::Called twice");
    return false;
  }

  state = (main_camera_system_state *)allocate_memory_linear(sizeof(main_camera_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "camera::create_camera()::State allocation failed");
    return false;
  }

  state->in_camera_metrics.handle.offset = BASE_RENDER_SCALE(.5f);
  state->in_camera_metrics.handle.target = Vector2 {position.x, position.y};
  state->in_camera_metrics.handle.rotation = 0;
  state->in_camera_metrics.handle.zoom = 1.0f;
  state->camera_min_speed = 30;
  state->camera_min_effect_lenght = 10;
  state->camera_fraction_speed = 5.f;

  return true;
}

camera_metrics* get_in_game_camera(void) { return &state->in_camera_metrics; }

bool update_camera(Vector2 position) {
  state->in_camera_metrics.handle.offset = BASE_RENDER_SCALE(.5f);

  Vector2 diff = vec2_subtract(position, state->in_camera_metrics.handle.target);
  float length = vec2_lenght(diff);

  if (length > state->camera_min_effect_lenght) {
    float speed = FMAX(state->camera_fraction_speed * length, state->camera_min_speed);
    state->in_camera_metrics.handle.target = vec2_add(state->in_camera_metrics.handle.target, vec2_scale(diff, speed * GetFrameTime() / length));
  }

  return true;
}
