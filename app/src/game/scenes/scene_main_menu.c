#include "scene_main_menu.h"
#include "core/event.h"

#include "defines.h"
#include "game/user_interface.h"

void initialize_scene_main_menu() {
  user_interface_system_initialize();

}

void update_scene_main_menu() {
  update_user_interface();

}

void render_interface_main_menu() {
  gui_draw_texture_to_background(TEX_BACKGROUND);
  gui_draw_spritesheet_to_background(SCREEN_CRT_SHEET, 
  (Color) {218, 165, 32, 180}
  );

  if (gui_button(BTN_ID_MAINMENU_BUTTON_PLAY, false)) {
    event_fire(EVENT_CODE_SCENE_IN_GAME, 0, (event_context){0});
  }
  if (gui_button(BTN_ID_MAINMENU_BUTTON_EDITOR, false)) {
    event_fire(EVENT_CODE_SCENE_EDITOR, 0, (event_context){0});
  }
  if (gui_button(BTN_ID_MAINMENU_BUTTON_SETTINGS, false)) {
    // TODO: Settings
  }
  if (gui_button(BTN_ID_MAINMENU_BUTTON_EXTRAS, false)) {
      
  }
  if (gui_button(BTN_ID_MAINMENU_BUTTON_EXIT, false)) {
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context){0});
  }
}
