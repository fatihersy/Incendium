#include "scene_in_game_edit.h"
#include "core/event.h"

#include "core/window.h"
#include "defines.h"
#include "game/tilemap.h"
//#include "game/game_manager.h"
#include "game/user_interface.h"

typedef struct scene_in_game_edit_state {
  //game_manager_system_state *p_game_manager;
  Vector2 target;
} scene_in_game_edit_state;

static scene_in_game_edit_state *state;

void update_bindings();

void initialize_scene_in_game_edit() {
  if(!tilemap_system_initialize()) {
    TraceLog(LOG_ERROR, "ERROR::scene_in_game_edit::initialize_scene_in_game_edit()::tilemap initialization failed");
  }
  user_interface_system_initialize();

  event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, (event_context) 
  {
    .data.i16[0] = get_screen_half_size().x,
    .data.i16[1] = get_screen_half_size().y,
  });
}

void update_scene_in_game_edit() {
  update_bindings();
}

void render_scene_in_game_edit() {

  render_tilemap();
  i16 ms = 1000;

  for (i16 i = -ms; i < ms; i += 16*3) { // X Axis
    DrawLine(i, -ms, i, i + (ms), (Color){255, 255, 255, 255});
  }
  for (i16 i = -ms; i < ms; i += 16*3) { // Y Axis
    DrawLine(-ms, i, i + (ms), i, (Color){255, 255, 255, 255});
  }
}

void render_interface_in_game_edit() {

}

void update_bindings() {
  if (IsKeyDown(KEY_W)) {
    state->target.y -= 2;
    event_context context = (event_context)
    {
      .data.i16[0] = state->target.x, 
      .data.i16[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, context);
  }
  if (IsKeyDown(KEY_A)) {
    state->target.x -= 2;
    event_context context = (event_context)
    {
      .data.i16[0] = state->target.x, 
      .data.i16[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, (event_context) {.data.i16[0] = -2});
  }
  if (IsKeyDown(KEY_S)) {
    state->target.y += 2;
    event_context context = (event_context)
    {
      .data.i16[0] = state->target.x, 
      .data.i16[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, (event_context) {.data.i16[1] = 2});
  }
  if (IsKeyDown(KEY_D)) {
    state->target.x += 2;
    event_context context = (event_context)
    {
      .data.i16[0] = state->target.x, 
      .data.i16[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, (event_context) {.data.i16[0] = 2});
  }

  if (IsKeyReleased(KEY_K)) {

  }
}