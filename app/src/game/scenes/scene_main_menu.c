#include "scene_main_menu.h"
#include "core/event.h"

#include "core/fmemory.h"
#include "defines.h"
#include "game/user_interface.h"
#include "raylib.h"

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

}

void render_interface_main_menu() {
  gui_draw_texture_to_background(TEX_BACKGROUND);
  gui_draw_spritesheet_to_background(
    SCREEN_CRT_SHEET, 
    (Color) {218, 165, 32, 100}
  );

  if (state->type == MAIN_MENU_SCENE_DEFAULT) {
    if (gui_button("Play", BTN_ID_MAINMENU_BUTTON_PLAY)) {
      event_fire(EVENT_CODE_SCENE_IN_GAME, 0, (event_context){0});
    }
    if (gui_button("Editor", BTN_ID_MAINMENU_BUTTON_EDITOR)) {
      event_fire(EVENT_CODE_SCENE_EDITOR, 0, (event_context){0});
    }
    if (gui_button("Settings", BTN_ID_MAINMENU_BUTTON_SETTINGS)) {
      state->type = MAIN_MENU_SCENE_SETTINGS;
      event_fire(EVENT_CODE_UI_SHOW_SETTINGS_MENU, 0, (event_context) {0});
    }
    if (gui_button("Extras", BTN_ID_MAINMENU_BUTTON_EXTRAS)) {
    
    }
    if (gui_button("Exit", BTN_ID_MAINMENU_BUTTON_EXIT)) {
      event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context){0});
    }
  }


  render_user_interface();
}
