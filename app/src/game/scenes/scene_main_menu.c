#include "scene_main_menu.h"
#include "defines.h"

#include "core/event.h"
#include "core/fmemory.h"

#include "game/world.h"
#include "game/user_interface.h"

typedef enum main_menu_scene_type {
  MAIN_MENU_SCENE_DEFAULT,
  MAIN_MENU_SCENE_SETTINGS,
  MAIN_MENU_SCENE_EXTRAS,
} main_menu_scene_type;

typedef struct main_menu_scene_state {
  main_menu_scene_type type;
  scene_type next_scene;
  bool in_scene_changing_process;
  bool scene_changing_process_complete;
} main_menu_scene_state;

static main_menu_scene_state *state;

#define MAIN_MENU_FADE_DURATION 1 * TARGET_FPS

void initialize_scene_main_menu(void) {
  if (state) {return;}
  
  state = (main_menu_scene_state*)allocate_memory_linear(sizeof(main_menu_scene_state), true);

  user_interface_system_initialize();

  set_worldmap_location(WORLDMAP_MAINMENU_MAP); // NOTE: Worldmap index 0 is mainmenu background now 
}

void update_scene_main_menu(void) {
  update_user_interface();

  event_fire(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, (event_context) {
    .data.f32[0] = GetScreenWidth()/2.f,
    .data.f32[1] = GetScreenHeight()/2.f,
  });
  if (state->in_scene_changing_process && is_ui_fade_anim_about_to_complete()) {
    state->scene_changing_process_complete = true;
  }
  if (state->in_scene_changing_process && is_ui_fade_anim_complete()) {
    switch (state->next_scene) {
      case SCENE_IN_GAME: event_fire(EVENT_CODE_SCENE_IN_GAME, (event_context){0}); break;
      case SCENE_EDITOR: event_fire(EVENT_CODE_SCENE_EDITOR, (event_context){0}); break;
      default: {
        TraceLog(LOG_ERROR, "scene_main_menu::update_scene_main_menu::Unknown scene");
        break;
      }
    }
  }
}

void render_scene_main_menu(void) {
  if (!state->scene_changing_process_complete) {
    render_map();
  }
}

void render_interface_main_menu(void) {
  if (!state->scene_changing_process_complete) {
    if (state->type == MAIN_MENU_SCENE_DEFAULT) {
      if (gui_menu_button("Play", BTN_ID_MAINMENU_BUTTON_PLAY, VECTOR2(0,  0))) {
        state->in_scene_changing_process = true;
        state->next_scene = SCENE_IN_GAME;
        event_fire(EVENT_CODE_UI_START_FADEOUT_EFFECT, (event_context){ .data.u16[0] = MAIN_MENU_FADE_DURATION});
      }
      if (gui_menu_button("Editor", BTN_ID_MAINMENU_BUTTON_EDITOR, VECTOR2(0,  4))) {
        state->in_scene_changing_process = true;
        state->next_scene = SCENE_EDITOR;
        event_fire(EVENT_CODE_UI_START_FADEOUT_EFFECT, (event_context){ .data.u16[0] = MAIN_MENU_FADE_DURATION});
      }
      if (gui_menu_button("Settings", BTN_ID_MAINMENU_BUTTON_SETTINGS, VECTOR2(0,  8))) {
        state->type = MAIN_MENU_SCENE_SETTINGS;
        event_fire(EVENT_CODE_UI_SHOW_SETTINGS_MENU, (event_context) {0});
      }
      if (gui_menu_button("Extras", BTN_ID_MAINMENU_BUTTON_EXTRAS, VECTOR2(0,  12))) {}
      if (gui_menu_button("Exit", BTN_ID_MAINMENU_BUTTON_EXIT, VECTOR2(0,  16))) {
        event_fire(EVENT_CODE_APPLICATION_QUIT, (event_context){0});
      }
    }
    else if (state->type == MAIN_MENU_SCENE_SETTINGS) {
      if(gui_menu_button("Cancel", BTN_ID_MAINMENU_SETTINGS_CANCEL, VECTOR2(2,  15))) {
         state->type = MAIN_MENU_SCENE_DEFAULT;
        event_fire(EVENT_CODE_UI_SHOW_SETTINGS_MENU, (event_context) {0});
      }
    }
    render_user_interface();
  }
}
