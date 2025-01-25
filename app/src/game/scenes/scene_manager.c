#include "scene_manager.h"
#include "raylib.h"
#include "defines.h"

#include "core/event.h"
#include "core/fmemory.h"

#include "game/camera.h"
#include "game/scenes/scene_in_game.h"
#include "game/scenes/scene_main_menu.h"
#include "game/scenes/scene_editor.h"

static scene_manager_system_state *scene_manager_state;

bool scene_manager_on_event(u16 code, void *sender, void *listener_inst, event_context context);

bool scene_manager_initialize() {
  if (scene_manager_state) return false;

  scene_manager_state = (scene_manager_system_state *)allocate_memory_linear(sizeof(scene_manager_system_state), true);

  scene_manager_state->gridsize = 33;
  scene_manager_state->map_size = scene_manager_state->gridsize * 100;

  event_register(EVENT_CODE_SCENE_IN_GAME, 0, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_EDITOR, 0, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_MAIN_MENU, 0, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, 0, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_MANAGER_SET_ZOOM, 0, scene_manager_on_event);

  event_fire(EVENT_CODE_SCENE_MAIN_MENU, 0, (event_context) {0});
  
  scene_manager_state->is_scene_manager_initialized = true;
  return true;
}

void update_scene_scene() {
  update_camera(scene_manager_state->target);

  switch (scene_manager_state->scene_data) {
    case SCENE_MAIN_MENU: update_scene_main_menu();     break;
    case SCENE_IN_GAME:   update_scene_in_game();       break;
    case SCENE_EDITOR:    update_scene_editor();        break;
  default: break;
  }
}
void render_scene_world() {
  switch (scene_manager_state->scene_data) {
    case SCENE_MAIN_MENU: render_scene_main_menu();     break;
    case SCENE_IN_GAME:   render_scene_in_game();       break;
    case SCENE_EDITOR:    render_scene_editor();        break;
  default: break;
  }
}
void render_scene_interface() {
  switch (scene_manager_state->scene_data) {
    case SCENE_IN_GAME:   render_interface_in_game();   break;
    case SCENE_MAIN_MENU: render_interface_main_menu(); break;
    case SCENE_EDITOR:    render_interface_editor();    break;
  default: break;
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


bool scene_manager_on_event(u16 code, void *sender, void *listener_inst, event_context context) {
  switch (code) {
  case EVENT_CODE_SCENE_IN_GAME: {
    scene_manager_state->scene_data = SCENE_IN_GAME;
    initialize_scene_in_game(get_active_metrics());
    return true;
    break;
  }
  case EVENT_CODE_SCENE_EDITOR: {
    scene_manager_state->scene_data = SCENE_EDITOR;
    initialize_scene_editor(get_active_metrics());
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
    scene_manager_state->target.x = context.data.f32[0];
    scene_manager_state->target.y = context.data.f32[1];
    return true;
    break;
  }
  case EVENT_CODE_SCENE_MANAGER_SET_CAM_POS: {
    //TraceLog(LOG_INFO, "x:%f, y:%f", context.data.f32[0], context.data.f32[1]);
    scene_manager_state->target.x = context.data.f32[0];
    scene_manager_state->target.y = context.data.f32[1];
    get_active_metrics()->handle.target.x = context.data.f32[0];
    get_active_metrics()->handle.target.y = context.data.f32[1];
    return true;
    break;
  }
  case EVENT_CODE_SCENE_MANAGER_SET_ZOOM: {
    get_active_metrics()->handle.zoom = context.data.f32[0];
    return true;
    break;
  }
  default:
    break;
  }

  return false;
}
