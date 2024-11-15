#include "user_interface.h"

#include "core/fmemory.h"
#include "core/event.h"

#include "defines.h"
#include "game/resource.h"
#include "game/player.h"

/* #define RAYGUI_IMPLEMENTATION
#include "raygui.h" */

// raygui embedded styles
// NOTE: Included in the same order as selector
/* #define MAX_GUI_STYLES_AVAILABLE 12     // NOTE: Included light style
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
#include "game/styles/style_terminal.h" // raygui style: terminal */
#include "raylib.h"

#define BTN_MENU_DIM_X 250
#define BTN_MENU_DIM_Y 50
#define BTN_MENU_DIM_X_DIV2 BTN_MENU_DIM_X / 2.f
#define BTN_MENU_DIM_Y_DIV2 BTN_MENU_DIM_Y / 2.f
#define BTN_SQUARE_DIM 70
#define BTN_SQUARE_DIM_DIV2 BTN_SQUARE_DIM / 2.f
#define BTN_SPACE_BTW(i) (BTN_DIM_Y+15)*i

#define PAUSE_MENU_TOTAL_WIDTH 850
#define PAUSE_MENU_TOTAL_HEIGHT 480

static user_interface_system_state* ui_system_state;

bool user_interface_on_event(u16 code, void *sender, void *listener_inst, event_context context);
void register_button(const char* _text, u16 _x, u16 _y, button_type _btn_type, texture_type _tex_type, Vector2 source_dim);
bool gui_button(button _btn);
void show_pause_screen();
void show_skill_up();

bool b_user_interface_system_initialized = false;

void user_interface_system_initialize() {
  if(b_user_interface_system_initialized) return;

  ui_system_state = (user_interface_system_state*)allocate_memory_linear(sizeof(user_interface_system_state), true);

  ui_system_state->p_player = get_player_state();
  ui_system_state->p_player_exp = ui_system_state->p_player->exp_current;
  ui_system_state->p_player_health = ui_system_state->p_player->health_current;
  
  b_user_interface_system_initialized = true;
  
  register_button("Play", SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2, BTN_TYPE_STANDARD, BUTTON_TEXTURE, (Vector2) {80, 16}); 


  event_register(EVENT_CODE_UI_SHOW_PAUSE_SCREEN, 0, user_interface_on_event);
  event_register(EVENT_CODE_UI_SHOW_UNPAUSE_SCREEN, 0, user_interface_on_event);

}

void update_user_interface(Vector2 _offset, Vector2 _screen_half_size, scene_type _current_scene_type, Camera2D _camera) {
  ui_system_state->screen_center = _screen_half_size;  
  ui_system_state->p_player_health = (float)ui_system_state->p_player->health_current;
  ui_system_state->p_player_exp = (float)ui_system_state->p_player->exp_current;
  ui_system_state->mouse_pos = GetMousePosition();
  ui_system_state->offset = _offset;
  ui_system_state->gm_current_scene_type = _current_scene_type;

  for (int i = 0; i < ui_system_state->button_count; ++i) {
    if(ui_system_state->buttons[i].btn_type == BTN_TYPE_UNDEFINED) continue;
    button btn = ui_system_state->buttons[i];

    if(CheckCollisionPointRec(ui_system_state->mouse_pos, btn.dest)) {

      if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        stop_sprite(BUTTON_REFLECTION_SHEET, true);
        btn.state = BTN_STATE_PRESSED;
        btn.source.x = btn.source.width;
      }
      else 
      {
        if(btn.state == BTN_STATE_UP)
        {
          play_sprite(BUTTON_REFLECTION_SHEET, ON_SITE, true, (Rectangle){btn.dest.x, btn.dest.y, .width = BTN_MENU_DIM_X, .height = BTN_MENU_DIM_Y}, false, 0);
        }
        btn.state = BTN_STATE_HOVER;
        btn.source.x = 0;
      }
    }
    else 
    {      
      btn.state = BTN_STATE_UP;
      btn.source.x = 0;
    }

    ui_system_state->buttons[i] = btn;
  }
}

void render_user_interface() {
  switch (ui_system_state->gm_current_scene_type) {
  case SCENE_MAIN_MENU: {

/*     DrawTexturePro(
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
        WHITE); // Draws the background to main menu */

    if (gui_button(ui_system_state->buttons[0])) {
      //event_fire(EVENT_CODE_SCENE_IN_GAME, 0, (event_context){0});
    };
/*     if (gui_button((button) {0})) {
      // TODO: Settings
    };
    if (gui_button((button) {0})) {
      event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context){0});
    }; */

    break;
  }
  case SCENE_IN_GAME: {

/*     GuiProgressBar(
      (Rectangle) {20, 20, 200, 12}, 
      "", 
      TextFormat("%d", p_player->health_current), 
      &p_player_health, 
      0, 
      p_player->health_max);

    GuiProgressBar(
      (Rectangle) {screen_center.x - 250, 20, 500, 12}, 
      "", 
      "", 
      &p_player_exp, 
      0, 
      p_player->exp_to_next_level); */

    if (ui_system_state->p_player->player_have_skill_points) {
      show_skill_up();
    }

    break;
  }
  default:
    break;
  }

  if (ui_system_state->b_show_pause_screen) {
    show_pause_screen();
  }
}

void show_pause_screen() {

  DrawRectangle(
    0, 
    0, 
    GetScreenWidth(), 
    GetScreenHeight(),
    (Color){53, 59, 72, 255}); // rgba()



  if (gui_button((button){0})) {
    event_fire(EVENT_CODE_UNPAUSE_GAME, 0, (event_context){0});
  }
  if (gui_button((button){0})) {
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context){0});
  }
  if (gui_button((button){0})) {
    // TODO: Settings
  }
  if (gui_button((button){0})) {
    ui_system_state->b_show_pause_screen = false;
    event_fire(EVENT_CODE_RETURN_MAIN_MENU_GAME, 0, (event_context) {0});
  }
  if (gui_button((button){0})) {
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context){0});
  }
}

bool gui_button(button _btn) {

  DrawTexturePro(
    get_texture_by_enum(BUTTON_TEXTURE), 
    _btn.source, 
    _btn.dest, 
    (Vector2){0}, 
    0, 
    WHITE
  );

  return _btn.state == BTN_STATE_PRESSED;
}

void register_button(const char* _text, u16 _x, u16 _y, button_type _btn_type, texture_type _tex_type, Vector2 source_dim) {
  if(_btn_type == BTN_TYPE_UNDEFINED || !b_user_interface_system_initialized) return;

  button btn = {
    .id = ui_system_state->button_count,
    .text = _text,
    .btn_type = _btn_type,
    .tex_type = _tex_type,
    .state = BTN_STATE_UP,
    .dest = (Rectangle) {
      .x = _x - BTN_MENU_DIM_X_DIV2,
      .y = _y - BTN_MENU_DIM_Y_DIV2,
      .width = BTN_MENU_DIM_X,
      .height = BTN_MENU_DIM_Y
    },
    .source = (Rectangle) {
      .x = 0,
      .y = 0,
      .width = source_dim.x,
      .height = source_dim.y
    },
  };

  ui_system_state->buttons[ui_system_state->button_count] = btn;
  ui_system_state->button_count++;
}

void show_skill_up() {
  if(gui_button((button){0})) {
    ui_system_state->p_player->ability_system.abilities[FIREBALL].level++;
    ui_system_state->p_player->ability_system.is_dirty_ability_system = true;
    ui_system_state->p_player->player_have_skill_points = false;
  }
}

bool user_interface_on_event(u16 code, void *sender, void *listener_inst, event_context context) {
  switch (code) {
  case EVENT_CODE_UI_SHOW_PAUSE_SCREEN: { // DON'T FIRE GAME_ENGINE PAUSE EVENTS
                                          // HERE
    ui_system_state->b_show_pause_screen = true;
    return true;
    break;
  }
  case EVENT_CODE_UI_SHOW_UNPAUSE_SCREEN: { // DON'T FIRE GAME_ENGINE PAUSE
                                            // EVENTS HERE
    ui_system_state->b_show_pause_screen = false;
    return true;
    break;
  }
  };

  return false;
}
