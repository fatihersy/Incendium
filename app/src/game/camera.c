#include "camera.h"
#include "raylib.h"
#include <settings.h>

#include "core/fmath.h"
#include "core/fmemory.h"


typedef struct main_camera_system_state {
  Camera2D camera;
  float camera_min_speed;
  float camera_min_effect_lenght;
  float camera_fraction_speed;
} main_camera_system_state;

static main_camera_system_state *state;

void create_camera(Vector2 position) {
  if (state) {
    TraceLog(LOG_ERROR, "camera::create_camera()::Called twice");
    return;
  }

  state = (main_camera_system_state *)allocate_memory_linear(sizeof(main_camera_system_state), true);

  state->camera.offset = get_resolution_div2();
  state->camera.target = (Vector2){position.x, position.y};
  state->camera.rotation = 0;
  state->camera.zoom = 1.0f;
  state->camera_min_speed = 30;
  state->camera_min_effect_lenght = 10;
  state->camera_fraction_speed = 5.f;
}

Camera2D *get_active_camera() { return &state->camera; }

bool update_camera(Vector2 position) {
  state->camera.offset = get_resolution_div2();

  Vector2 diff = vec2_subtract(position, state->camera.target);
  float length = vec2_lenght(diff);

  if (length > state->camera_min_effect_lenght) {
    float speed = FMAX(state->camera_fraction_speed * length, state->camera_min_speed);
    state->camera.target = vec2_add(state->camera.target, vec2_scale(diff, speed * GetFrameTime() / length));
  }

  return true;
}
