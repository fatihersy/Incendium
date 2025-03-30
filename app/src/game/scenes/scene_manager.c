#include "scene_manager.h"
#include "defines.h"

#include "core/event.h"
#include "core/fmemory.h"

#include "game/camera.h"
#include "game/scenes/scene_in_game.h"
#include "game/scenes/scene_main_menu.h"
#include "game/scenes/scene_editor.h"

typedef struct scene_manager_system_state {
  scene_id scene_data;
  Vector2 target;
} scene_manager_system_state;

static scene_manager_system_state *scene_manager_state;

bool scene_manager_on_event(u16 code, event_context context);
void begin_scene(scene_id scene_id);
void end_scene(scene_id scene_id);

bool scene_manager_initialize(void) {
  if (scene_manager_state) return false;

  scene_manager_state = (scene_manager_system_state *)allocate_memory_linear(sizeof(scene_manager_system_state), true);

  event_register(EVENT_CODE_SCENE_IN_GAME, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_EDITOR, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_MAIN_MENU, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_MANAGER_SET_TARGET, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_MANAGER_SET_ZOOM, scene_manager_on_event);

  event_fire(EVENT_CODE_SCENE_MAIN_MENU, (event_context) {0});
  
  return true;
}

void update_scene_scene(void) {
  update_camera(scene_manager_state->target);

  switch (scene_manager_state->scene_data) {
    case SCENE_TYPE_MAIN_MENU: update_scene_main_menu();     break;
    case SCENE_TYPE_IN_GAME:   update_scene_in_game();       break;
    case SCENE_TYPE_EDITOR:    update_scene_editor();        break;
  default: break;
  }
}
void render_scene_world(void) {
  switch (scene_manager_state->scene_data) {
    case SCENE_TYPE_MAIN_MENU: render_scene_main_menu();     break;
    case SCENE_TYPE_IN_GAME:   render_scene_in_game();       break;
    case SCENE_TYPE_EDITOR:    render_scene_editor();        break;
  default: break;
  }
}
void render_scene_interface(void) {
  switch (scene_manager_state->scene_data) {
    case SCENE_TYPE_IN_GAME:   render_interface_in_game();   break;
    case SCENE_TYPE_MAIN_MENU: render_interface_main_menu(); break;
    case SCENE_TYPE_EDITOR:    render_interface_editor();    break;
  default: break;
  }
}

void set_current_scene_type(scene_id type) {
  scene_manager_state->scene_data = type;
}
scene_id get_current_scene_type(void) {
  return scene_manager_state->scene_data;
}
Vector2 get_spectator_position(void) {
  return scene_manager_state->target;
}
void end_scene(scene_id scene_id) {
  switch (scene_id) {
  case SCENE_TYPE_UNSPECIFIED: {
    TraceLog(LOG_WARNING, "scene_manager::end_scene()::Scene has not unspecified");
    return;
  }
  case SCENE_TYPE_MAIN_MENU: end_scene_main_menu(); return;
  case SCENE_TYPE_IN_GAME: end_scene_in_game(); return;
  case SCENE_TYPE_EDITOR: end_scene_editor(); return;

  default:
  TraceLog(LOG_ERROR, "scene_manager::end_scene()::Unsupported scene");
  return;
  }

  TraceLog(LOG_ERROR, "scene_manager::end_scene()::Function ended unexpectedly");
}

bool scene_manager_on_event(u16 code, event_context context) {
  switch (code) {
  case EVENT_CODE_SCENE_IN_GAME: {
    end_scene(scene_manager_state->scene_data);
    scene_manager_state->scene_data = SCENE_TYPE_IN_GAME;
    initialize_scene_in_game(get_in_game_camera());
    return true;
    break;
  }
  case EVENT_CODE_SCENE_EDITOR: {
    end_scene(scene_manager_state->scene_data);
    scene_manager_state->scene_data = SCENE_TYPE_EDITOR;
    initialize_scene_editor(get_in_game_camera());
    return true;
    break;
  }
  case EVENT_CODE_SCENE_MAIN_MENU: {
    end_scene(scene_manager_state->scene_data);
    scene_manager_state->scene_data = SCENE_TYPE_MAIN_MENU;
    initialize_scene_main_menu(get_in_game_camera());
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
    scene_manager_state->target.x = context.data.f32[0];
    scene_manager_state->target.y = context.data.f32[1];
    get_in_game_camera()->handle.target.x = context.data.f32[0];
    get_in_game_camera()->handle.target.y = context.data.f32[1];
    return true;
    break;
  }
  case EVENT_CODE_SCENE_MANAGER_SET_ZOOM: {
    get_in_game_camera()->handle.zoom = context.data.f32[0];
    return true;
    break;
  }
  default:
    break;
  }

  return false;
}
