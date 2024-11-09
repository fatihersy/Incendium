#include "user_interface.h"

#include "core/event.h"

#include "game/resource.h"
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// raygui embedded styles
// NOTE: Included in the same order as selector
#define MAX_GUI_STYLES_AVAILABLE 12     // NOTE: Included light style
#include "game/styles/style_ashes.h"    // raygui style: ashes
#include "game/styles/style_bluish.h"   // raygui style: bluish
#include "game/styles/style_candy.h"    // raygui style: candy
#include "game/styles/style_cherry.h"   // raygui style: cherry
#include "game/styles/style_cyber.h"    // raygui style: cyber
#include "game/styles/style_dark.h"     // raygui style: dark
#include "game/styles/style_enefete.h"  // raygui style: enefete
#include "game/styles/style_jungle.h"   // raygui style: jungle
#include "game/styles/style_lavanda.h"  // raygui style: lavanda
#include "game/styles/style_sunny.h"    // raygui style: sunny
#include "game/styles/style_terminal.h" // raygui style: terminal

#define BTN_DIM_X 180
#define BTN_DIM_Y 40
#define BTN_DIM_X_DIV2 BTN_DIM_X / 2.f
#define BTN_DIM_Y_DIV2 BTN_DIM_Y / 2.f
#define BTN_SPACE_BTW(i) (BTN_DIM_Y+15)*i

#define PAUSE_MENU_TOTAL_WIDTH 850
#define PAUSE_MENU_TOTAL_HEIGHT 480

Vector2 screen_center = {0};
Vector2 offset = {0};

scene_type gm_current_scene_type = 0;
bool b_show_pause_screen = false;

void show_pause_screen();

bool user_interface_on_event(u16 code, void *sender, void *listener_inst,
                             event_context context);

void user_interface_system_initialize() {

  GuiLoadStyleCherry();

  event_register(EVENT_CODE_UI_SHOW_PAUSE_SCREEN, 0, user_interface_on_event);
  event_register(EVENT_CODE_UI_SHOW_UNPAUSE_SCREEN, 0, user_interface_on_event);
}

void update_user_interface(Vector2 _offset, Vector2 _screen_half_size,
                           scene_type _current_scene_type, Camera2D _camera) {
  screen_center = _screen_half_size;

  offset = _offset;
  gm_current_scene_type = _current_scene_type;
}

void render_user_interface() {
  switch (gm_current_scene_type) {
  case scene_main_menu: {

    DrawTexturePro(
        get_texture_by_enum(BACKGROUND),
        (Rectangle){.x = 0,
                    .y = 0,
                    .width = get_texture_by_enum(BACKGROUND).width,
                    .height = get_texture_by_enum(BACKGROUND).height},
        (Rectangle){.x = 0,
                    .y = 0,
                    .width = GetScreenWidth(),
                    .height = GetScreenHeight()},
        (Vector2){.x = 0, .y = 0}, 0,
        WHITE); // Draws the background to main menu

    if (gui_button(STANDARD, screen_center.x, screen_center.y + BTN_SPACE_BTW(-1), "Play")) {
      event_fire(EVENT_CODE_SCENE_IN_GAME, 0, (event_context){0});
    };
    if (gui_button(STANDARD, screen_center.x, screen_center.y, "Settings")) {
      // TODO: Settings
    };
    if (gui_button(STANDARD, screen_center.x, screen_center.y + BTN_SPACE_BTW(1), "Quit")) {
      event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context){0});
    };

    break;
  }
  case scene_in_game: {
    DrawFPS(offset.x, offset.y);
    break;
  }
  default:
    break;
  }

  if (b_show_pause_screen && gm_current_scene_type == scene_in_game) {
    show_pause_screen();
  }
}

bool gui_button(button_type type, int x, int y, const char *text) {
  switch (type) {
  case UNDEFINED: {
    return false;
  }
  case STANDARD: {
    return GuiButton((Rectangle){
      x - BTN_DIM_X_DIV2, 
      y - BTN_DIM_Y_DIV2,
      BTN_DIM_X, BTN_DIM_Y
    },
    text);
  }
  };
}

void show_pause_screen() {

  DrawRectangle(
    0, 
    0, 
    GetScreenWidth(), 
    GetScreenHeight(),
    (Color){53, 59, 72, 255}); // rgba()



  if (gui_button(STANDARD, screen_center.x/2.f, screen_center.y + BTN_SPACE_BTW(-2), "Continue")) {
    event_fire(EVENT_CODE_UNPAUSE_GAME, 0, (event_context){0});
  }
  if (gui_button(STANDARD, screen_center.x/2.f, screen_center.y + BTN_SPACE_BTW(-1), "Tips")) {
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context){0});
  }
  if (gui_button(STANDARD, screen_center.x/2.f, screen_center.y     , "Settings")) {
    // TODO: Settings
  }
  if (gui_button(STANDARD, screen_center.x/2.f, screen_center.y + BTN_SPACE_BTW(1), "Main Menu")) {
    event_fire(EVENT_CODE_RETURN_MAIN_MENU_GAME, 0, (event_context) {0});
  }
  if (gui_button(STANDARD, screen_center.x/2.f, screen_center.y + BTN_SPACE_BTW(2), "Quit")) {
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context){0});
  }
}

bool user_interface_on_event(u16 code, void *sender, void *listener_inst,
                             event_context context) {
  switch (code) {
  case EVENT_CODE_UI_SHOW_PAUSE_SCREEN: { // DON'T FIRE GAME_ENGINE PAUSE EVENTS
                                          // HERE
    b_show_pause_screen = true;
    return true;
    break;
  }
  case EVENT_CODE_UI_SHOW_UNPAUSE_SCREEN: { // DON'T FIRE GAME_ENGINE PAUSE
                                            // EVENTS HERE
    b_show_pause_screen = false;
    return true;
    break;
  }
  };

  return false;
}
