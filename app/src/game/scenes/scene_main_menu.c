#include "scene_main_menu.h"
#include "raylib.h"
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

  event_fire(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, 0, (event_context) {
    .data.f32[0] = GetScreenWidth()/2.f,
    .data.f32[1] = GetScreenHeight()/2.f,
  });
}

void update_scene_main_menu() {
  update_user_interface();

}

void render_scene_main_menu() {
  gui_draw_texture_to_background(TEX_ID_BACKGROUND);
  gui_draw_spritesheet_to_background(
    SCREEN_CRT_SHEET, 
    (Color) {218, 165, 32, 100}
  );
}

void render_interface_main_menu() {
  Font* ui_font = user_interface_state_get_font();

  if (state->type == MAIN_MENU_SCENE_DEFAULT) {
    if (gui_button("Play", BTN_ID_MAINMENU_BUTTON_PLAY, ui_font->baseSize)) {
      event_fire(EVENT_CODE_SCENE_IN_GAME, 0, (event_context){0});
    }
    if (gui_button("Editor", BTN_ID_MAINMENU_BUTTON_EDITOR, ui_font->baseSize)) {
      event_fire(EVENT_CODE_SCENE_EDITOR, 0, (event_context){0});
    }
    if (gui_button("Settings", BTN_ID_MAINMENU_BUTTON_SETTINGS, ui_font->baseSize)) {
      state->type = MAIN_MENU_SCENE_SETTINGS;
      event_fire(EVENT_CODE_UI_SHOW_SETTINGS_MENU, 0, (event_context) {0});
    }
    if (gui_button("Extras", BTN_ID_MAINMENU_BUTTON_EXTRAS, ui_font->baseSize)) {
    
    }
    if (gui_button("Exit", BTN_ID_MAINMENU_BUTTON_EXIT, ui_font->baseSize)) {
      event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context){0});
    }
  }
  if (state->type == MAIN_MENU_SCENE_SETTINGS) {
    if(gui_button("Cancel", BTN_ID_SETTINGS_APPLY_SETTINGS_BUTTON, ui_font->baseSize)) {
      state->type = MAIN_MENU_SCENE_DEFAULT;
      event_fire(EVENT_CODE_UI_SHOW_SETTINGS_MENU, 0, (event_context) {0});
    }
  }

  render_user_interface();
}
