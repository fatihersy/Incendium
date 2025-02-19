#include "scene_main_menu.h"
#include "defines.h"

#include "core/event.h"
#include "core/fmemory.h"
#include "game/user_interface.h"

typedef enum main_menu_scene_type {
  MAIN_MENU_SCENE_DEFAULT,
  MAIN_MENU_SCENE_SETTINGS,
  MAIN_MENU_SCENE_EXTRAS,
} main_menu_scene_type;

typedef struct main_menu_scene_state {
main_menu_scene_type type;
} main_menu_scene_state;

static main_menu_scene_state *state;

void initialize_scene_main_menu() {
  if (state) {return;}
  
  state = (main_menu_scene_state*)allocate_memory_linear(sizeof(main_menu_scene_state), true);

  user_interface_system_initialize();
}

void update_scene_main_menu() {
  update_user_interface();

  event_fire(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, (event_context) {
    .data.f32[0] = GetScreenWidth()/2.f,
    .data.f32[1] = GetScreenHeight()/2.f,
  });
}

void render_scene_main_menu() {
  gui_draw_texture_to_background(TEX_ID_BACKGROUND);
  gui_draw_spritesheet_to_background(
    SCREEN_CRT_SHEET, 
    (Color) {WHITE_ROCK.r, WHITE_ROCK.g, WHITE_ROCK.b, 100} //(Color) {218, 165, 32, 100}
  );
}

void render_interface_main_menu() {

  if (state->type == MAIN_MENU_SCENE_DEFAULT) {
    if (gui_menu_button("Play", BTN_ID_MAINMENU_BUTTON_PLAY, VECTOR2(0,  0))) {
      event_fire(EVENT_CODE_SCENE_IN_GAME, (event_context){0});
    }
    if (gui_menu_button("Editor", BTN_ID_MAINMENU_BUTTON_EDITOR, VECTOR2(0,  4))) {
      event_fire(EVENT_CODE_SCENE_EDITOR, (event_context){0});
    }
    if (gui_menu_button("Settings", BTN_ID_MAINMENU_BUTTON_SETTINGS, VECTOR2(0,  8))) {
      state->type = MAIN_MENU_SCENE_SETTINGS;
      event_fire(EVENT_CODE_UI_SHOW_SETTINGS_MENU, (event_context) {0});
    }
    if (gui_menu_button("Extras", BTN_ID_MAINMENU_BUTTON_EXTRAS, VECTOR2(0,  12))) {
    
    }
    if (gui_menu_button("Exit", BTN_ID_MAINMENU_BUTTON_EXIT, VECTOR2(0,  16))) {
      event_fire(EVENT_CODE_APPLICATION_QUIT, (event_context){0});
    }
  }
  if (state->type == MAIN_MENU_SCENE_SETTINGS) {
    if(gui_menu_button("Cancel", BTN_ID_SETTINGS_APPLY_SETTINGS_BUTTON, VECTOR2(2,  15))) {
      state->type = MAIN_MENU_SCENE_DEFAULT;
      event_fire(EVENT_CODE_UI_SHOW_SETTINGS_MENU, (event_context) {0});
    }
  }

  render_user_interface();
}
