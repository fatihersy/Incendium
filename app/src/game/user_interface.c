#include "user_interface.h"

#include "core/event.h"
#include "core/fmemory.h"

#include "defines.h"
#include "player.h"
#include "raylib.h"
#include "tilemap.h"
#include <stdbool.h>

#define BTN_MENU_DIM_X 250
#define BTN_MENU_DIM_Y 50
#define BTN_MENU_DIM_X_DIV2 BTN_MENU_DIM_X / 2.f
#define BTN_MENU_DIM_Y_DIV2 BTN_MENU_DIM_Y / 2.f
#define BTN_SQUARE_DIM 70
#define BTN_SQUARE_DIM_DIV2 BTN_SQUARE_DIM / 2.f
#define BTN_SPACE_BTW(i) (BTN_MENU_DIM_Y + 15) * i

#define PAUSE_MENU_TOTAL_WIDTH 850
#define PAUSE_MENU_TOTAL_HEIGHT 480

static user_interface_system_state *ui_system_state;

#define PSPRITESHEET_SYSTEM ui_system_state
#include "game/spritesheet.h"

Color theme_color_yellow = {237, 213, 0, 255};
bool b_user_interface_system_initialized = false;
bool user_interface_on_event(u16 code, void *sender, void *listener_inst,
                             event_context context);
bool gui_button(button_type _type);
void gui_healthbar(f32 percent);
void register_button(const char *_text, u16 _x, u16 _y, button_type _btn_type,
                     scene_type render_scene, texture_type _tex_type,
                     Vector2 source_dim);
void show_pause_screen();
void show_skill_up();

void user_interface_system_initialize() {
  if (b_user_interface_system_initialized) return;

  ui_system_state = (user_interface_system_state *)allocate_memory_linear(sizeof(user_interface_system_state), true);

  if(!tilemap_system_initialize()) {
    TraceLog(LOG_ERROR, "ERROR::user_interface::user_interface_system_initialize()::tilemap initialization failed");
  }

  ui_system_state->p_player = get_player_state();
  ui_system_state->p_player_exp = ui_system_state->p_player->exp_current;
  ui_system_state->p_player_health = ui_system_state->p_player->health_current;
  ui_system_state->ui_font = LoadFont(rs_path("QuanticoBold.ttf"));
  ui_system_state->b_show_pause_screen = false;

  b_user_interface_system_initialized = true;

  register_button("Play", SCREEN_WIDTH_DIV2,
                  SCREEN_HEIGHT_DIV2 + BTN_SPACE_BTW(0),
                  BTN_TYPE_MAINMENU_BUTTON_PLAY, SCENE_MAIN_MENU,
                  BUTTON_TEXTURE, (Vector2){80, 16});
  register_button("Edit", SCREEN_WIDTH_DIV2,
                  SCREEN_HEIGHT_DIV2 + BTN_SPACE_BTW(1),
                  BTN_TYPE_MAINMENU_BUTTON_EDIT, SCENE_MAIN_MENU,
                  BUTTON_TEXTURE, (Vector2){80, 16});
  register_button("Settings", SCREEN_WIDTH_DIV2,
                  SCREEN_HEIGHT_DIV2 + BTN_SPACE_BTW(2),
                  BTN_TYPE_MAINMENU_BUTTON_SETTINGS, SCENE_MAIN_MENU,
                  BUTTON_TEXTURE, (Vector2){80, 16});
  register_button("Extras", SCREEN_WIDTH_DIV2,
                  SCREEN_HEIGHT_DIV2 + BTN_SPACE_BTW(3),
                  BTN_TYPE_MAINMENU_BUTTON_EXTRAS, SCENE_MAIN_MENU,
                  BUTTON_TEXTURE, (Vector2){80, 16});
  register_button("Exit", SCREEN_WIDTH_DIV2,
                  SCREEN_HEIGHT_DIV2 + BTN_SPACE_BTW(4),
                  BTN_TYPE_MAINMENU_BUTTON_EXIT, SCENE_MAIN_MENU,
                  BUTTON_TEXTURE, (Vector2){80, 16});
  register_button("Resume", SCREEN_WIDTH_DIV2,
                  SCREEN_HEIGHT_DIV2 + BTN_SPACE_BTW(0),
                  BTN_TYPE_INGAME_PAUSEMENU_BUTTON_RESUME, SCENE_IN_GAME,
                  BUTTON_TEXTURE, (Vector2){80, 16});
  register_button("Settings", SCREEN_WIDTH_DIV2,
                  SCREEN_HEIGHT_DIV2 + BTN_SPACE_BTW(1),
                  BTN_TYPE_INGAME_PAUSEMENU_BUTTON_SETTINGS, SCENE_IN_GAME,
                  BUTTON_TEXTURE, (Vector2){80, 16});
  register_button("Main Menu", SCREEN_WIDTH_DIV2,
                  SCREEN_HEIGHT_DIV2 + BTN_SPACE_BTW(2),
                  BTN_TYPE_INGAME_PAUSEMENU_BUTTON_MAINMENU, SCENE_IN_GAME,
                  BUTTON_TEXTURE, (Vector2){80, 16});
  register_button("Exit", SCREEN_WIDTH_DIV2,
                  SCREEN_HEIGHT_DIV2 + BTN_SPACE_BTW(3),
                  BTN_TYPE_INGAME_PAUSEMENU_BUTTON_EXIT, SCENE_IN_GAME,
                  BUTTON_TEXTURE, (Vector2){80, 16});

  event_register(EVENT_CODE_UI_SHOW_PAUSE_SCREEN, 0, user_interface_on_event);
  event_register(EVENT_CODE_UI_SHOW_UNPAUSE_SCREEN, 0, user_interface_on_event);
}

void update_user_interface(Vector2 _offset, Vector2 _screen_half_size, scene_type _current_scene_type, Camera2D _camera) {
  ui_system_state->screen_center = _screen_half_size;
  ui_system_state->p_player_health = (float)ui_system_state->p_player->health_current;
  ui_system_state->p_player_health_max = (float)ui_system_state->p_player->health_max;
  ui_system_state->p_player_health_perc = ((float)
    (float)ui_system_state->p_player->health_current / (float)ui_system_state->p_player->health_max);
  ui_system_state->p_player_exp = (float)ui_system_state->p_player->exp_current;
  ui_system_state->p_player_exp_max = (float)ui_system_state->p_player->exp_to_next_level;
  ui_system_state->p_player_exp_perc = 
    (float)ui_system_state->p_player->exp_current / (float)ui_system_state->p_player->exp_to_next_level;
  ui_system_state->mouse_pos = GetMousePosition();
  ui_system_state->offset = _offset;
  ui_system_state->scene_data = _current_scene_type;
  update_sprite_renderqueue();
  
  if (IsKeyReleased(KEY_K)) {
    ui_system_state->b_show_tilemap_screen = !ui_system_state->b_show_tilemap_screen;
  }

  for (int i = 0; i < BTN_TYPE_MAX; ++i) {
    if (ui_system_state->buttons[i].btn_type == BTN_TYPE_UNDEFINED) {
      /*       TraceLog(
                LOG_WARNING,
                "user_interface::update_user_interface()::Button:%d was
         undefined", i); */
      continue;
    }
    if (ui_system_state->buttons[i].render_on_scene !=
        ui_system_state->scene_data) {
      continue;
    }

    button btn = ui_system_state->buttons[i];
    if (CheckCollisionPointRec(ui_system_state->mouse_pos, btn.dest)) {

      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        btn.state = BTN_STATE_PRESSED;
        btn.source.x = btn.source.width;
        stop_sprite(ui_system_state->buttons[btn.btn_type].reflection_render_index, true);
      } else {
        if (btn.state != BTN_STATE_HOVER) {
          btn.state = BTN_STATE_HOVER;
          btn.source.x = 0;
        }
      }
    } else {
      if (btn.state != BTN_STATE_UP) {
        stop_sprite(btn.reflection_render_index, true);
        btn.is_reflection_played = false;
        btn.state = BTN_STATE_UP;
        btn.source.x = 0;
      }
    }

    ui_system_state->buttons[i] = btn;
  }
}

void render_user_interface() {
  switch (ui_system_state->scene_data) {
  case SCENE_MAIN_MENU: {
    DrawTexturePro(
    *get_texture_by_enum(BACKGROUND),
    (Rectangle){
      .x = 0,
      .y = 0,
      .width = get_texture_by_enum(BACKGROUND)->width,
      .height = get_texture_by_enum(BACKGROUND)->height},
    (Rectangle){
      .x = 0,
      .y = 0,
      .width = GetScreenWidth(),
      .height = GetScreenHeight()},
    (Vector2){.x = 0, .y = 0}, 0,
    WHITE); // Draws the background to main menu

    render_sprite_renderqueue();

    if (gui_button(BTN_TYPE_MAINMENU_BUTTON_PLAY)) {
      event_fire(EVENT_CODE_SCENE_IN_GAME, 0, (event_context){0});
    };
    if (gui_button(BTN_TYPE_MAINMENU_BUTTON_EDIT)) {
      event_fire(EVENT_CODE_SCENE_IN_GAME_EDIT, 0, (event_context){0});
    };
    if (gui_button(BTN_TYPE_MAINMENU_BUTTON_SETTINGS)) {
      // TODO: Settings
    };
    if (gui_button(BTN_TYPE_MAINMENU_BUTTON_EXTRAS)) {
      
    };
    if (gui_button(BTN_TYPE_MAINMENU_BUTTON_EXIT)) {
      event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context){0});
    };
    break;
  }
  case SCENE_IN_GAME: {
    gui_healthbar(ui_system_state->p_player_health_perc);

    if (ui_system_state->p_player->player_have_skill_points) {
      show_skill_up();
    }
    
    #ifdef _DEBUG
    if (ui_system_state->b_show_tilemap_screen) {
      render_tilemap();
    }
    #endif

    break;
  }
  default:
    break;
  }

  if (ui_system_state->b_show_pause_screen) {
    render_sprite_renderqueue();
    show_pause_screen();
  }
}

void show_pause_screen() {

  DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                (Color){53, 59, 72, 200});

  if (gui_button(BTN_TYPE_INGAME_PAUSEMENU_BUTTON_RESUME)) {
    event_fire(EVENT_CODE_UNPAUSE_GAME, 0, (event_context){0});
  }
  if (gui_button(BTN_TYPE_INGAME_PAUSEMENU_BUTTON_SETTINGS)) {
    // TODO: Settings
  }
  if (gui_button(BTN_TYPE_INGAME_PAUSEMENU_BUTTON_MAINMENU)) {
    ui_system_state->b_show_pause_screen = false;
  }
  if (gui_button(BTN_TYPE_INGAME_PAUSEMENU_BUTTON_EXIT)) {
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context){0});
  }
}

bool gui_button(button_type _type) {
  if (_type == BTN_TYPE_UNDEFINED) {
    return false;
  }
  button _btn = ui_system_state->buttons[_type];
  DrawTexturePro(*get_texture_by_enum(BUTTON_TEXTURE), _btn.source, _btn.dest,
                 (Vector2){0}, 0, WHITE);
  if (!is_sprite_playing(_btn.crt_render_index)) {
    play_sprite_on_site(_btn.crt_render_index, _btn.dest);
  }

  if (_btn.state == BTN_STATE_PRESSED) {
    DrawTextEx(ui_system_state->ui_font, _btn.text,
               (Vector2){.x = _btn.text_pos.x, .y = _btn.text_pos.y + 3},
               ui_system_state->ui_font.baseSize, _btn.text_spacing,
               theme_color_yellow);
  } else {
    if (_btn.state == BTN_STATE_HOVER) {
      if (_btn.is_reflection_played == false) {
        play_sprite_on_site(_btn.reflection_render_index, _btn.dest);
        _btn.is_reflection_played = true;
      }
    }
    DrawTextEx(ui_system_state->ui_font, _btn.text,
               (Vector2){.x = _btn.text_pos.x, .y = _btn.text_pos.y - 3},
               ui_system_state->ui_font.baseSize, _btn.text_spacing,
               theme_color_yellow);
  }
  ui_system_state->buttons[_type] = _btn;
  return _btn.state == BTN_STATE_PRESSED;
}
void gui_healthbar(f32 percent) {
  const u16 iter = 10*percent;
  DrawTexturePro(
    *get_texture_by_enum(HEALTHBAR_TEXTURE),
    (Rectangle){.x = 0, .y = 0, .width = 72, .height = 12},
    (Rectangle){.x = 15, .y = 15, .width = 216, .height = 36},
    (Vector2){0, 0}, 0, WHITE
  );
  for (int i=0; i < iter; ++i) {
    DrawTexturePro(
    *get_texture_by_enum(HEALTH_PERC_TEXTURE),
    (Rectangle){.x = 0, .y = 0, .width = 5, .height = 7},
    (Rectangle){.x = 33+(15+3)*i, .y = 24, .width = 15, .height = 21},
    (Vector2){0, 0}, 0, WHITE
    );
  }
}

void register_button(const char *_text, u16 _x, u16 _y, button_type _btn_type,
                     scene_type render_scene, texture_type _tex_type,
                     Vector2 source_dim) {
  if (_btn_type == BTN_TYPE_UNDEFINED || !b_user_interface_system_initialized)
    return;
  Vector2 text_measure =
      MeasureTextEx(ui_system_state->ui_font, _text,
                    ui_system_state->ui_font.baseSize, UI_FONT_SPACING);
  button btn = {
      .id = _btn_type,
      .btn_type = _btn_type,
      .text = _text,
      .render_on_scene = render_scene,
      .crt_render_index =
          register_sprite(BUTTON_CRT_SHEET, render_scene, true, false),
      .reflection_render_index =
          register_sprite(BUTTON_REFLECTION_SHEET, render_scene, true, false),
      .is_reflection_played = false,
      .text_spacing = UI_FONT_SPACING,
      .tex_type = _tex_type,
      .text_pos.x = _x - text_measure.x / 2.f,
      .text_pos.y = _y - text_measure.y / 2.f,
      .state = BTN_STATE_UP,
      .dest = (Rectangle){.x = _x - BTN_MENU_DIM_X_DIV2,
                          .y = _y - BTN_MENU_DIM_Y_DIV2,
                          .width = BTN_MENU_DIM_X,
                          .height = BTN_MENU_DIM_Y},
      .source =
          (Rectangle){
              .x = 0, .y = 0, .width = source_dim.x, .height = source_dim.y},
  };

  ui_system_state->buttons[_btn_type] = btn;
}

void show_skill_up() {
  if (gui_button(BTN_TYPE_UNDEFINED)) {
    ui_system_state->p_player->ability_system.abilities[FIREBALL].level++;
    ui_system_state->p_player->ability_system.is_dirty_ability_system = true;
    ui_system_state->p_player->player_have_skill_points = false;
  }
}

bool user_interface_on_event(u16 code, void *sender, void *listener_inst,
                             event_context context) {
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

#undef PSPRITESHEET_SYSTEM
