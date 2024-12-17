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
  draw_to_background(BACKGROUND);

  if (gui_button(BTN_TYPE_MAINMENU_BUTTON_PLAY)) {
    event_fire(EVENT_CODE_SCENE_IN_GAME, 0, (event_context){0});
  };
  if (gui_button(BTN_TYPE_MAINMENU_BUTTON_EDITOR)) {
    event_fire(EVENT_CODE_SCENE_EDITOR, 0, (event_context){0});
  };
  if (gui_button(BTN_TYPE_MAINMENU_BUTTON_SETTINGS)) {
    // TODO: Settings
  };
  if (gui_button(BTN_TYPE_MAINMENU_BUTTON_EXTRAS)) {
      
  };
  if (gui_button(BTN_TYPE_MAINMENU_BUTTON_EXIT)) {
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context){0});
  };
}
