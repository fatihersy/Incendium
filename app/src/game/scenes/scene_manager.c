#include "scene_manager.h"
#include "defines.h"

#include "core/event.h"
#include "core/fmemory.h"

#include "game/camera.h"
#include "scene_main_menu.h"
#include "scene_in_game.h"
#include "scene_in_game_edit.h"

void clean_up();
void draw_background();

static scene_manager_system_state *scene_manager_state;

bool scene_manager_on_event(u16 code, void *sender, void *listener_inst, event_context context);

bool scene_manager_initialize(Vector2 _screen_size, Vector2 _screen_half_size) {
  if (scene_manager_state) return false;

  scene_manager_state = (scene_manager_system_state *)allocate_memory_linear(sizeof(scene_manager_system_state), true);

  scene_manager_state->gridsize = 33;
  scene_manager_state->map_size = scene_manager_state->gridsize * 100;

  event_register(EVENT_CODE_SCENE_IN_GAME, 0, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_IN_GAME_EDIT, 0, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_MAIN_MENU, 0, scene_manager_on_event);

  event_fire(EVENT_CODE_SCENE_MAIN_MENU, 0, (event_context) {0});
  
  scene_manager_state->is_scene_manager_initialized = true;
  return true;
}

void update_scene_manager() {
  update_camera(scene_manager_state->target);
  switch (scene_manager_state->scene_data) {
  case SCENE_IN_GAME_EDIT:  {
    update_scene_in_game_edit();
    break;
  }
  case SCENE_IN_GAME:  {
    update_scene_in_game();
    break;
  }
  case SCENE_MAIN_MENU:  {
    update_scene_main_menu();
    break;
  }
  }
}

bool scene_manager_on_event(u16 code, void *sender, void *listener_inst,
                            event_context context) {
  switch (code) {
  case EVENT_CODE_SCENE_IN_GAME: {
    scene_manager_state->scene_data = SCENE_IN_GAME;
    return true;
    break;
  }
  case EVENT_CODE_SCENE_IN_GAME_EDIT: {
    scene_manager_state->scene_data = SCENE_IN_GAME_EDIT;
    initialize_scene_in_game_edit();
    return true;
    break;
  }
  case EVENT_CODE_SCENE_MAIN_MENU: {
    scene_manager_state->scene_data = SCENE_MAIN_MENU;
    initialize_scene_main_menu();
    return true;
    break;
  }
  case EVENT_CODE_SCENE_MANAGER_SET_TARGET: {
    scene_manager_state->target.x = context.data.u16[0];
    scene_manager_state->target.y = context.data.u16[1];
    return true;
    break;
  }
  default:
    break;
  }

  return false;
}
void draw_background() {
  u16 ms = scene_manager_state->map_size;

  switch (scene_manager_state->scene_data) {
  case SCENE_MAIN_MENU: {
    break;
  }
  case SCENE_IN_GAME: {

    break;
  }
  case SCENE_IN_GAME_EDIT: {

    break;
  }

  default:
    break;
  }
}

void set_current_scene_type(scene_type type) {
  scene_manager_state->scene_data = type;
}


scene_type get_current_scene_type() {
  return scene_manager_state->scene_data;
}

Vector2 get_spectator_position() {
  return scene_manager_state->target;
}

void update_scene_world() {
  switch (scene_manager_state->scene_data) {
    case SCENE_MAIN_MENU: break;

    case SCENE_IN_GAME: update_scene_in_game(); break;
    case SCENE_IN_GAME_EDIT: update_scene_in_game_edit(); break;
  default: break;
  }
}

void update_scene_interface() {
  switch (scene_manager_state->scene_data) {
    case SCENE_IN_GAME: break;
    case SCENE_IN_GAME_EDIT: break;

    case SCENE_MAIN_MENU: update_scene_main_menu();  break;
  default: break;
  }
}
void render_scene_world() {
  switch (scene_manager_state->scene_data) {
    case SCENE_MAIN_MENU: break;

    case SCENE_IN_GAME: render_scene_in_game(); break;
    case SCENE_IN_GAME_EDIT: render_scene_in_game_edit(); break;
  default: break;
  }
}
void render_scene_interface() {
  switch (scene_manager_state->scene_data) {
    case SCENE_IN_GAME: break;
    case SCENE_IN_GAME_EDIT: break;

    case SCENE_MAIN_MENU: render_scene_main_menu();  break;
  default: break;
  }
}
