#include "scene_in_game_edit.h"

#include "core/event.h"
#include "core/fmemory.h"

#include "game/camera.h"
#include "game/tilemap.h"
#include "game/game_manager.h"
#include "game/user_interface.h"
#include "raylib.h"

typedef struct scene_in_game_edit_state {
  Vector2 target;
} scene_in_game_edit_state;

static scene_in_game_edit_state *state;

void update_bindings();

void initialize_scene_in_game_edit() {
  if(!create_tilemap((Vector2) {0, 0}, 500, 16*3, WHITE)) {
    TraceLog(LOG_ERROR, "ERROR::scene_in_game_edit::initialize_scene_in_game_edit()::tilemap initialization failed");
  }
  user_interface_system_initialize();

  state = (scene_in_game_edit_state*)allocate_memory_linear(sizeof(scene_in_game_edit_state), true);

  event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, (event_context) 
  {
    .data.i16[0] = _get_screen_half_size().x,
    .data.i16[1] = _get_screen_half_size().y,
  });



}

void update_scene_in_game_edit() {
  update_bindings();
}

void render_scene_in_game_edit() {
  render_tilemap();

}

void render_interface_in_game_edit() {

}

void update_bindings() {
  f32 speed = 3;

  if (IsKeyDown(KEY_W)) {
    state->target.y -= speed;
    event_context context = (event_context)
    {
      .data.i16[0] = state->target.x, 
      .data.i16[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, context);
  }
  if (IsKeyDown(KEY_A)) {
    state->target.x -= speed;
    event_context context = (event_context)
    {
      .data.i16[0] = state->target.x, 
      .data.i16[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, context);
  }
  if (IsKeyDown(KEY_S)) {
    state->target.y += speed;
    event_context context = (event_context)
    {
      .data.i16[0] = state->target.x, 
      .data.i16[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, context);
  }
  if (IsKeyDown(KEY_D)) {
    state->target.x += speed;
    event_context context = (event_context)
    {
      .data.i16[0] = state->target.x, 
      .data.i16[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, context);
  }

  if (IsKeyReleased(KEY_K)) {

  }

  if (IsKeyReleased(KEY_ESCAPE)) {
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context) {0});
  }

  // Camera zoom controls
  get_active_camera()->zoom += ((float)GetMouseWheelMove()*0.05f);

  if (get_active_camera()->zoom > 3.0f) get_active_camera()->zoom = 3.0f;
  else if (get_active_camera()->zoom < 0.1f) get_active_camera()->zoom = 0.1f;
}

