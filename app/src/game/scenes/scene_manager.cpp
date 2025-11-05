#include "scene_manager.h"
#include "game/game_types.h"

#include "core/event.h"
#include "core/fmemory.h"
#include "core/logger.h"

#include "game/scenes/scene_in_game.h"
#include "game/scenes/scene_main_menu.h"
#include "game/scenes/scene_editor.h"

typedef struct scene_manager_system_state {
  scene_id scene_data;
  const app_settings * in_app_settings;
  scene_manager_system_state(void) {
    this->scene_data = SCENE_TYPE_UNSPECIFIED;
    this->in_app_settings = nullptr;
  }
} scene_manager_system_state;

static scene_manager_system_state * state = nullptr;

bool scene_manager_on_event(i32 code, event_context context);
void begin_scene(scene_id scene_id);
void end_scene(scene_id scene_id);
bool scene_manager_reinit(void);

bool scene_manager_initialize(const app_settings * _in_app_settings) {
  if (state and state != nullptr) {
    return scene_manager_reinit();
  }
  state = (scene_manager_system_state *)allocate_memory_linear(sizeof(scene_manager_system_state), true);
  if (not state or state == nullptr) {
    IERROR("scene_manager::scene_manager_initialize()::State allocation failed");
    return false;
  }
  *state = scene_manager_system_state();

  event_register(EVENT_CODE_SCENE_IN_GAME, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_EDITOR, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_MAIN_MENU, scene_manager_on_event);

  state->in_app_settings = _in_app_settings;
  return scene_manager_reinit();
}
bool scene_manager_reinit(void) {
  if (not state or state == nullptr ) {
    IERROR("scene_manager::scene_manager_reinit()::State is not valid");
    return false;
  }
  return event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context(static_cast<i32>(true)));
}

void update_scene_scene(void) {
  switch (state->scene_data) {
    case SCENE_TYPE_MAIN_MENU: update_scene_main_menu();     break;
    case SCENE_TYPE_IN_GAME:   update_scene_in_game();       break;
    case SCENE_TYPE_EDITOR:    update_scene_editor();        break;
    default: {
      event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context(static_cast<i32>(true)));
      return;
    }
  }
}
void render_scene_world(void) {
  switch (state->scene_data) {
    case SCENE_TYPE_MAIN_MENU: render_scene_main_menu();     break;
    case SCENE_TYPE_IN_GAME:   render_scene_in_game();       break;
    case SCENE_TYPE_EDITOR:    render_scene_editor();        break;
    default: {
      event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context(static_cast<i32>(true)));
      return;
    }
  }
}
void render_scene_interface(void) {
  switch (state->scene_data) {
    case SCENE_TYPE_IN_GAME:   render_interface_in_game();   break;
    case SCENE_TYPE_MAIN_MENU: render_interface_main_menu(); break;
    case SCENE_TYPE_EDITOR:    render_interface_editor();    break;
    default: {
      event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context(static_cast<i32>(true)));
      return;
    }
  }
}

void end_scene(scene_id scene_id) {
  switch (scene_id) {
    case SCENE_TYPE_MAIN_MENU: {
      end_scene_main_menu(); 
      return;
    }
    case SCENE_TYPE_IN_GAME: {
      end_scene_in_game(); 
      return;
    }
    case SCENE_TYPE_EDITOR: {
      end_scene_editor(); 
      return;
    }
    default: {
      return;
    }
  }
  IERROR("scene_manager::end_scene()::Function ended unexpectedly");
}

bool scene_manager_on_event(i32 code,[[maybe_unused]] event_context context) {
  switch (code) {
  case EVENT_CODE_SCENE_IN_GAME: {
    end_scene(state->scene_data);
    state->scene_data = SCENE_TYPE_IN_GAME;
    if(not initialize_scene_in_game(state->in_app_settings, context.data.i32[0])) {
      IERROR("scene_manager::scene_manager_on_event()::EVENT_CODE_SCENE_IN_GAME::Scene In-game failed to initialize");
      return false;
    }
    return true;
  }
  case EVENT_CODE_SCENE_EDITOR: {
    end_scene(state->scene_data);
    state->scene_data = SCENE_TYPE_EDITOR;
    if(not initialize_scene_editor(state->in_app_settings, context.data.i32[0])) {
      IERROR("scene_manager::scene_manager_on_event()::EVENT_CODE_SCENE_EDITOR::Scene Editor failed to initialize");
      return false;
    }
    return true;
  }
  case EVENT_CODE_SCENE_MAIN_MENU: {
    end_scene(state->scene_data);
    state->scene_data = SCENE_TYPE_MAIN_MENU;
    if (not initialize_scene_main_menu(state->in_app_settings, context.data.i32[0])) {
      IERROR("scene_manager::scene_manager_on_event()::EVENT_CODE_SCENE_MAIN_MENU::Scene Mainmenu failed to initialize");
      return false;
    }
    return true;
  }
  default:{
      IWARN("scene_manager::scene_manager_on_event()::Unsupported scene");
      event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context(static_cast<i32>(true)));
      return false;
  }
  }
  IERROR("scene_manager::scene_manager_on_event()::Function ended unexpectedly");
  return false;
}
