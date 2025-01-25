#include "camera.h"
#include <settings.h>

#include "core/fmath.h"
#include "core/fmemory.h"


typedef struct main_camera_system_state {
  float camera_min_speed;
  float camera_min_effect_lenght;
  float camera_fraction_speed;

  camera_metrics out_camera_metrics;
} main_camera_system_state;

static main_camera_system_state *state;

void create_camera(Vector2 position) {
  if (state) {
    TraceLog(LOG_ERROR, "camera::create_camera()::Called twice");
    return;
  }

  state = (main_camera_system_state *)allocate_memory_linear(sizeof(main_camera_system_state), true);

  state->out_camera_metrics.handle.offset = *get_resolution_div2();
  state->out_camera_metrics.handle.target = (Vector2){position.x, position.y};
  state->out_camera_metrics.handle.rotation = 0;
  state->out_camera_metrics.handle.zoom = 1.0f;
  state->camera_min_speed = 30;
  state->camera_min_effect_lenght = 10;
  state->camera_fraction_speed = 5.f;
}

camera_metrics* get_active_metrics() { return &state->out_camera_metrics; }

bool update_camera(Vector2 position) {
  state->out_camera_metrics.handle.offset = *get_resolution_div2();

  Vector2 diff = vec2_subtract(position, state->out_camera_metrics.handle.target);
  float length = vec2_lenght(diff);

  if (length > state->camera_min_effect_lenght) {
    float speed = FMAX(state->camera_fraction_speed * length, state->camera_min_speed);
    state->out_camera_metrics.handle.target = vec2_add(state->out_camera_metrics.handle.target, vec2_scale(diff, speed * GetFrameTime() / length));
  }

  return true;
}
