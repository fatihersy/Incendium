#include "user_interface.h"

#include "core/event.h"
#include "game/camera.h"
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

Vector2 screen_center = {0};
Vector2 offset = {0};

scene_type gm_current_scene_type = 0;
bool b_show_pause_screen = false;

void show_pause_screen();

bool user_interface_on_event(u16 code, void *sender, void *listener_inst,
                             event_context context);

void user_interface_system_initialize() {

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
    if (GuiButton((Rectangle){screen_center.x - BTN_DIM_X_DIV2,
                              screen_center.y - BTN_DIM_Y_DIV2 - 40, BTN_DIM_X,
                              BTN_DIM_Y},
                  "Play")) {
      event_fire(EVENT_CODE_SCENE_IN_GAME, 0, (event_context){0});
    };
    if (GuiButton((Rectangle){screen_center.x - BTN_DIM_X_DIV2,
                              screen_center.y - BTN_DIM_Y_DIV2, BTN_DIM_X,
                              BTN_DIM_Y},
                  "Settings")) {
      // TODO: Settings
    };
    if (GuiButton((Rectangle){screen_center.x - BTN_DIM_X_DIV2,
                              screen_center.y - BTN_DIM_Y_DIV2 + 40, BTN_DIM_X,
                              BTN_DIM_Y},
                  "Quit")) {
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

  if (b_show_pause_screen) {
    show_pause_screen();
  }
}

bool gui_button(button_type type, int x, int y, const char *text) {
  switch (type) {
  case UNDEFINED: {
    return false;
  }
  case STANDARD: {
    return GuiButton((Rectangle){x - BTN_DIM_X_DIV2, y - BTN_DIM_Y_DIV2, BTN_DIM_X, BTN_DIM_Y}, text);
  }
  };
}

void show_pause_screen() 
{
  Vector2 screen_pos = GetScreenToWorld2D((Vector2) {screen_center.x, screen_center.y - 40}, get_active_camera());
  
  DrawRectangle(screen_pos.x - 150, 0, 300, GetScreenHeight(), (Color){53, 59, 72, 155}); // rgba()

  if (gui_button(STANDARD, screen_center.x, screen_center.y - 40, "Continue")) 
  {
    event_fire(EVENT_CODE_UNPAUSE_GAME, 0, (event_context) {0});
  }
  if (gui_button(STANDARD, screen_center.x, screen_center.y, "Settings")) 
  {
    // TODO: Settings
  }
  if (gui_button(STANDARD, screen_center.x, screen_center.y + 40, "Quit")) 
  {
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context){0});
  }
}

bool user_interface_on_event(u16 code, void *sender, void *listener_inst, event_context context) 
{
  switch (code) {
    case EVENT_CODE_UI_SHOW_PAUSE_SCREEN: { // DON'T FIRE GAME_ENGINE PAUSE EVENTS HERE
      b_show_pause_screen = true;
      return true;
      break;
    }
    case EVENT_CODE_UI_SHOW_UNPAUSE_SCREEN: { // DON'T FIRE GAME_ENGINE PAUSE EVENTS HERE
      b_show_pause_screen = false;
      return true;
      break;
    }
  };

  return false;
}
