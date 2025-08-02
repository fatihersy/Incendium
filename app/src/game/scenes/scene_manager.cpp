#include "scene_manager.h"
#include "game/game_types.h"

#include "core/event.h"
#include "core/fmemory.h"

#include "game/scenes/scene_in_game.h"
#include "game/scenes/scene_main_menu.h"
#include "game/scenes/scene_editor.h"

typedef struct scene_manager_system_state {
  scene_id scene_data;
  const app_settings * in_app_settings;
} scene_manager_system_state;

static scene_manager_system_state * state;

bool scene_manager_on_event(i32 code, event_context context);
void begin_scene(scene_id scene_id);
void end_scene(scene_id scene_id);
bool scene_manager_reinit(void);

bool scene_manager_initialize(const app_settings * _in_app_settings) {
  if (state != nullptr) {
    return scene_manager_reinit();
  }
  state = (scene_manager_system_state *)allocate_memory_linear(sizeof(scene_manager_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "scene_manager::scene_manager_initialize()::State allocation failed");
    return false;
  }

  event_register(EVENT_CODE_SCENE_IN_GAME, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_EDITOR, scene_manager_on_event);
  event_register(EVENT_CODE_SCENE_MAIN_MENU, scene_manager_on_event);

  state->in_app_settings = _in_app_settings;

  return scene_manager_reinit();
}
bool scene_manager_reinit(void) {
  if (!state || state == nullptr ) {
    TraceLog(LOG_ERROR, "scene_manager::scene_manager_reinit()::State is not valid");
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
  case SCENE_TYPE_UNSPECIFIED: { return; }

  case SCENE_TYPE_MAIN_MENU: end_scene_main_menu(); return;
  case SCENE_TYPE_IN_GAME: end_scene_in_game(); return;
  case SCENE_TYPE_EDITOR: end_scene_editor(); return;

  default:
  TraceLog(LOG_ERROR, "scene_manager::end_scene()::Unsupported scene");
  return;
  }

  TraceLog(LOG_ERROR, "scene_manager::end_scene()::Function ended unexpectedly");
}

bool scene_manager_on_event(i32 code,[[maybe_unused]] event_context context) {
  switch (code) {
  case EVENT_CODE_SCENE_IN_GAME: {
    end_scene(state->scene_data);
    state->scene_data = SCENE_TYPE_IN_GAME;
    if(!initialize_scene_in_game(state->in_app_settings, context.data.i32[0])) {
      if (!begin_scene_in_game(context.data.i32[0])) {
        TraceLog(LOG_ERROR, "scene_manager::EVENT_CODE_SCENE_IN_GAME::User interface failed to initialize!");
        return false;
      }
    }
    return true;
  }
  case EVENT_CODE_SCENE_EDITOR: {
    end_scene(state->scene_data);
    state->scene_data = SCENE_TYPE_EDITOR;
    if(!initialize_scene_editor(state->in_app_settings, context.data.i32[0])) {
      if (!begin_scene_editor(context.data.i32[0])) {
        TraceLog(LOG_ERROR, "scene_manager::EVENT_CODE_SCENE_IN_GAME::User interface failed to initialize!");
        return false;
      }
    }
    return true;
  }
  case EVENT_CODE_SCENE_MAIN_MENU: {
    end_scene(state->scene_data);
    state->scene_data = SCENE_TYPE_MAIN_MENU;
    if (!initialize_scene_main_menu(state->in_app_settings, context.data.i32[0])) {
      if (!begin_scene_main_menu(context.data.i32[0])) {
        TraceLog(LOG_ERROR, "scene_manager::EVENT_CODE_SCENE_IN_GAME::User interface failed to initialize!");
        return false;
      }
    }
    return true;
  }
  default:
    break;
  }

  return false;
}
