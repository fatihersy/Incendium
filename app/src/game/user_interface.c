#include "user_interface.h"

#include "core/event.h"
#include "core/fmemory.h"

#include "defines.h"
#include "game/user_interface.h"
#include "raylib.h"
#include <stdbool.h>

#define DEFAULT_MENU_BUTTON_SCALE 3
#define BTN_SPACE_BTW_Y(i, DIM_Y) (DIM_Y + DIM_Y/3.f) * i
#define BTN_SPACE_BTW_X(i, DIM_X) (DIM_X + DIM_X/3.f) * i

typedef struct user_interface_system_state {
  spritesheet_play_system spritesheet_system;
  player_state *p_player;
  Vector2 offset;
  Vector2 mouse_pos;
  button buttons[BTN_ID_MAX];
  button_type button_types[BTN_TYPE_MAX];
  Font ui_font;
  spritesheet ss_to_draw_bg;
  bool b_show_pause_menu;

  u16 ssq_settings_slider_id;
} user_interface_system_state;

static user_interface_system_state *state;

#define PSPRITESHEET_SYSTEM state // Don't forget to undef very bottom of the file
#include "game/spritesheet.h"

bool      user_interface_on_event(u16 code, void *sender, void *listener_inst, event_context context);
void      register_button(const char *_text, Vector2 _attached_position, Vector2 offset, button_id _btn_id, button_type_id _btn_type_id);
void      register_button_type(button_type_id _btn_type_id, spritesheet_type _ss_type, Vector2 frame_dim, f32 _scale);
void      draw_texture_regular(Texture2D* tex, Rectangle dest);
void      draw_texture_type_regular(texture_type _type, Rectangle dest);
void      gui_draw_panel(Rectangle dest, bool should_center);
Rectangle get_texture_source_rect(texture_type _type);

void user_interface_system_initialize() {
  if (state) return;

  state = (user_interface_system_state *)allocate_memory_linear(sizeof(user_interface_system_state), true);
  state->ui_font = LoadFont(rs_path("quantico_bold.ttf"));

  register_button_type(BTN_TYPE_MENU_BUTTON, MENU_BUTTON, (Vector2){80, 16}, DEFAULT_MENU_BUTTON_SCALE);
  register_button_type(BTN_TYPE_SLIDER_LEFT_BUTTON, SETTINGS_SLIDER_LEFT_BUTTON, (Vector2){10, 10}, DEFAULT_MENU_BUTTON_SCALE);
  register_button_type(BTN_TYPE_SLIDER_RIGHT_BUTTON, SETTINGS_SLIDER_RIGHT_BUTTON, (Vector2){10, 10}, DEFAULT_MENU_BUTTON_SCALE);

  // MAIN MENU
  register_button(
    "Play", (Vector2) { SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2 }, (Vector2) {0,0 },
    BTN_ID_MAINMENU_BUTTON_PLAY, BTN_TYPE_MENU_BUTTON);

  register_button(
    "Editor", (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,1 },
    BTN_ID_MAINMENU_BUTTON_EDITOR, BTN_TYPE_MENU_BUTTON);

  register_button(
    "Settings", (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,2 },
    BTN_ID_MAINMENU_BUTTON_SETTINGS, BTN_TYPE_MENU_BUTTON);

  register_button(
    "Extras", (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,3 },
    BTN_ID_MAINMENU_BUTTON_EXTRAS, BTN_TYPE_MENU_BUTTON);

  register_button(
    "Exit", (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,4 },
    BTN_ID_MAINMENU_BUTTON_EXIT, BTN_TYPE_MENU_BUTTON);
  // MAIN MENU

  // EDITOR
  register_button(
    "Load Map", (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,1 },
    BTN_ID_EDITOR_BUTTON_LOAD_MAP, BTN_TYPE_MENU_BUTTON);

  register_button(
    "Save Map", (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,2 },
    BTN_ID_EDITOR_BUTTON_SAVE_MAP, BTN_TYPE_MENU_BUTTON);
  // EDITOR
  
  // USER INTERFACE
  register_button(
    "Resume", (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,1 },
    BTN_ID_PAUSEMENU_BUTTON_RESUME, BTN_TYPE_MENU_BUTTON);

  register_button(
    "Settings", (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,2 },
    BTN_ID_PAUSEMENU_BUTTON_SETTINGS, BTN_TYPE_MENU_BUTTON);

  register_button(
    "Main Menu", (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,3 },
    BTN_ID_PAUSEMENU_BUTTON_MAIN_MENU, BTN_TYPE_MENU_BUTTON);

  register_button(
    "Exit", (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,4 },
    BTN_ID_PAUSEMENU_BUTTON_EXIT, BTN_TYPE_MENU_BUTTON);
  // USER INTERFACE

  state->ssq_settings_slider_id = register_sprite(SETTINGS_SLIDER_PERCENT, false, false, false);

  register_button(
    "Main Menu", (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {-1,0 },
    BTN_ID_SETTINGS_SLIDER_LEFT_BUTTON, BTN_TYPE_SLIDER_LEFT_BUTTON);

  register_button(
    "Exit", (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {1,0 },
    BTN_ID_SETTINGS_SLIDER_RIGHT_BUTTON, BTN_TYPE_SLIDER_RIGHT_BUTTON);

  event_register(EVENT_CODE_UI_SHOW_PAUSE_MENU, 0, user_interface_on_event);
}

bool set_player_user_interface(player_state* player) {
  if (player->initialized) {
    state->p_player = player;
    return true;
  }

  return false;
}

void update_user_interface() {
  state->mouse_pos = GetMousePosition();
  state->offset = (Vector2) { SCREEN_OFFSET, SCREEN_OFFSET};
  update_sprite_renderqueue();

  for (int i = 0; i < BTN_ID_MAX; ++i) {
    if (state->buttons[i].id == BTN_ID_UNDEFINED) {
      continue;
    }
    if (!state->buttons[i].show) { continue; }

    button btn = state->buttons[i];
    if (CheckCollisionPointRec(state->mouse_pos, btn.dest)) {

      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        btn.state = BTN_STATE_PRESSED;
        //btn.btn_type.source_rect.x = btn.btn_type.source_rect.width;
        stop_sprite(state->buttons[btn.id].reflection_render_index, true);
      } else {
        if (btn.state != BTN_STATE_HOVER) {
          btn.state = BTN_STATE_HOVER;
          //btn.btn_type.source_rect.x = 0;
        }
      }
    } else {
      if (btn.state != BTN_STATE_UP) {
        reset_sprite(btn.reflection_render_index, true);
        btn.state = BTN_STATE_UP;
        //btn.btn_type.source_rect.x = 0;
      }
    }

    state->buttons[i] = btn;
  }
}

void render_user_interface() {
  
  if (state->b_show_pause_menu) {
    gui_draw_pause_screen();
  }
}

bool gui_button(const char* text, button_id _id, bool play_crt) {
  if (_id >= BTN_ID_MAX || _id <= BTN_ID_UNDEFINED) {
    TraceLog(LOG_WARNING, "WARNING::user_interface::gui_button()::Recieved button type out of bound");
    return false;
  }

  button* _btn = &state->buttons[_id];
  if (!_btn->is_registered) {
    TraceLog(LOG_WARNING, "WARNING::user_interface::gui_button()::The button is not registered");
    return false;
  }
  button_type* _btn_type = &_btn->btn_type;
  _btn->show = true;

  Vector2 text_measure;
  Vector2 text_pos;

  if (!TextIsEqual(text, "")) {
    text_measure = MeasureTextEx(state->ui_font, text, state->ui_font.baseSize, UI_FONT_SPACING);
    text_pos = (Vector2) {
    _btn->dest.x + _btn->dest.width /2.f - text_measure.x / 2.f, 
    _btn->dest.y + _btn->dest.height/2.f - text_measure.y / 2.f
    };
  }

  if(play_crt) play_sprite_on_site(_btn->crt_render_index, WHITE, _btn->dest);
  Vector2 pos = (Vector2) {_btn->dest.x, _btn->dest.y};

  if (_btn->state == BTN_STATE_PRESSED) {
    draw_sprite_on_site(_btn->btn_type.ss_type, WHITE, pos, _btn->btn_type.scale, 1, false);
    if (!TextIsEqual(text, "")) {
      DrawTextEx(state->ui_font, text,
        (Vector2){.x = text_pos.x, .y = text_pos.y + 3},
        state->ui_font.baseSize, UI_FONT_SPACING,
        MYYELLOW);
    }
  } else {
    draw_sprite_on_site(_btn->btn_type.ss_type, WHITE, pos, _btn->btn_type.scale, 0, false);
    if (_btn->state == BTN_STATE_HOVER) {
      play_sprite_on_site(_btn->reflection_render_index, WHITE, _btn->dest);
    }
    if (!TextIsEqual(text, "")) {
      DrawTextEx(state->ui_font, text,
        (Vector2){.x = text_pos.x, .y = text_pos.y - 3},
        state->ui_font.baseSize, UI_FONT_SPACING,
        MYYELLOW);
    }
  }
  return _btn->state == BTN_STATE_PRESSED;
}

void gui_healthbar(f32 percent) {
  const u16 iter = 10*percent;
  DrawTexturePro(
    *get_texture_by_enum(TEX_HEALTHBAR_TEXTURE),
    (Rectangle){.x = 0, .y = 0, .width = 72, .height = 12},
    (Rectangle){.x = 15, .y = 15, .width = 216, .height = 36},
    (Vector2){0, 0}, 0, WHITE
  );
  for (int i=0; i < iter; ++i) {
    DrawTexturePro(
    *get_texture_by_enum(TEX_HEALTH_PERC_TEXTURE),
    (Rectangle){.x = 0, .y = 0, .width = 5, .height = 7},
    (Rectangle){.x = 33+(15+3)*i, .y = 24, .width = 15, .height = 21},
    (Vector2){0, 0}, 0, WHITE
    );
  }
}

void gui_draw_panel(Rectangle dest, bool should_center) {
  Texture2D* tex_panel = get_texture_by_enum(TEX_PANEL);
  if (!tex_panel) {
    TraceLog(LOG_WARNING, "WARNING::user_interface::gui_draw_panel()::Panel textures return null");
    return;
  }

  if (should_center) {
    dest.x -= dest.width/2.f;
    dest.y -= dest.height/2.f;
  }

  draw_texture_regular(tex_panel, dest);
}

void gui_draw_pause_screen() {
  gui_draw_panel((Rectangle) {
    .x = SCREEN_OFFSET,     .y = SCREEN_OFFSET, 
    .width = SCREEN_WIDTH - SCREEN_OFFSET, .height = SCREEN_HEIGHT - SCREEN_OFFSET}, false
  );

  if (gui_button("Resume", BTN_ID_PAUSEMENU_BUTTON_RESUME, true)) {
    state->b_show_pause_menu = !state->b_show_pause_menu;
  }
  if (gui_button("Settings", BTN_ID_PAUSEMENU_BUTTON_SETTINGS, true)) {
    
  }
  if (gui_button("Main Menu", BTN_ID_PAUSEMENU_BUTTON_MAIN_MENU, true)) {
    event_fire(EVENT_CODE_SCENE_MAIN_MENU, 0, (event_context) {0});
  }
  if (gui_button("Exit", BTN_ID_PAUSEMENU_BUTTON_EXIT, true)) {
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context) {0});
  }
}

void gui_draw_options_screen() {
  if (gui_button("", BTN_ID_SETTINGS_SLIDER_LEFT_BUTTON, true)) {
    
  }
  Vector2 pos = (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2};
  
  draw_sprite_on_site(SETTINGS_SLIDER_PERCENT, WHITE, pos, DEFAULT_MENU_BUTTON_SCALE, 0, false);

  if (gui_button("", BTN_ID_SETTINGS_SLIDER_RIGHT_BUTTON, true)) {
    
  }
}

void register_button_type(button_type_id _btn_type_id, spritesheet_type _ss_type, Vector2 frame_dim, f32 _scale) {
  if (_btn_type_id >= BTN_TYPE_MAX || _btn_type_id <= BTN_TYPE_UNDEFINED ||
      !state) {
    TraceLog(LOG_WARNING, "WARNING::user_interface::register_button_type()::Recieved id was out of bound");
    return;
  }

  button_type btn_type = {
      .id = _btn_type_id,
      .scale = _scale,
      .ss_type = _ss_type,
      .source_frame_dim = (Vector2) {
        .x = frame_dim.x, .y = frame_dim.y
      },
  };

  state->button_types[_btn_type_id] = btn_type;
}

void register_button(const char *_text, Vector2 _attached_position, Vector2 offset, button_id _btn_id, button_type_id _btn_type_id) {
  if (_btn_id      >= BTN_ID_MAX   || _btn_id      <= BTN_ID_UNDEFINED   || 
      _btn_type_id >= BTN_TYPE_MAX || _btn_type_id <= BTN_TYPE_UNDEFINED ||
      !state) 
  {
    TraceLog(LOG_WARNING, "WARNING::user_interface::register_button()::One of recieved ids was out of bound");
    return;
  }
  //+ 
  button_type* _btn_type = &state->button_types[_btn_type_id];

  u16 width = _btn_type->source_frame_dim.x * _btn_type->scale;
  u16 height = _btn_type->source_frame_dim.y * _btn_type->scale;
  f32 pos_x = _attached_position.x + BTN_SPACE_BTW_X(offset.x, width) - width/2.f;
  f32 pos_y = _attached_position.y + BTN_SPACE_BTW_Y(offset.y, height) - height/2.f;

  button btn = {
      .id = _btn_id,
      .btn_type = state->button_types[_btn_type_id],
      .dest = (Rectangle) {
        .x = pos_x, .y = pos_y,
        .width = width, .height = height
      },
      .state = BTN_STATE_UP,
      .crt_render_index = register_sprite(BUTTON_CRT_SHEET, true, false, false),
      .reflection_render_index = register_sprite(BUTTON_REFLECTION_SHEET, false, true, false),
      .show = false,
      .is_registered = true,
  };

  state->buttons[_btn_id] = btn;
}

void gui_draw_texture_to_background(texture_type _type) {
  draw_texture_type_regular(_type, (Rectangle) {
    0, 0, GetScreenWidth(), GetScreenHeight()
  });
}

void gui_draw_spritesheet_to_background(spritesheet_type _type, Color _tint) {
  if (_type >= SPRITESHEET_TYPE_MAX || _type <= SPRITESHEET_UNSPECIFIED || !state) {
    TraceLog(LOG_WARNING, "WARNING::user_interface::gui_draw_spritesheet_to_background()::Sprite type out of bound");
    return;
  }
  if (state->ss_to_draw_bg.type != _type) {
    state->ss_to_draw_bg = get_spritesheet_by_enum(_type);
    state->ss_to_draw_bg.render_queue_index = register_sprite(_type, true, false, false);
  }
  Rectangle dest = (Rectangle) {0, 0, GetScreenWidth(), GetScreenHeight()};
  play_sprite_on_site(state->ss_to_draw_bg.render_queue_index, _tint, dest);
}

/**
 * @note inline function, returns "(Rectangle) {0}" if texture type returns null pointer
 * @return (Rectangle) { .x = 0, .y = 0, .width = tex->width, .height = tex->height}; 
 */
inline Rectangle get_texture_source_rect(texture_type _type) {
  Texture2D* tex = get_texture_by_enum(_type);
  if (!tex) { 
    TraceLog(LOG_WARNING, "WARNING::user_interface::get_texture_source_rect()::Requested type was null");
    return (Rectangle) {0}; 
  }
  
  return (Rectangle) { .x = 0, .y = 0, .width = tex->width, .height = tex->height};
}

inline void draw_texture_regular(Texture2D* tex, Rectangle dest) {
  if (!tex) { TraceLog(
  LOG_WARNING, "WARNING::user_interface::draw_texture_regular()::Tex was null");
    return; }

  DrawTexturePro(*tex, 
  (Rectangle) { 0, 0, tex->width, tex->height}, 
  dest, 
  (Vector2) {0}, 0, WHITE);
}

inline void draw_texture_type_regular(texture_type _type, Rectangle dest) {
  Texture2D* tex = get_texture_by_enum(_type);

  if (!tex || tex->id == 0) { TraceLog(
  LOG_WARNING, "WARNING::user_interface::draw_texture_regular()::Tex was null");
    return; }

  DrawTexturePro(*tex, 
  (Rectangle) { 0, 0, tex->width, tex->height}, 
  dest, 
  (Vector2) {0}, 0, WHITE); 
}


void user_interface_system_destroy() {

}

bool user_interface_on_event(u16 code, void *sender, void *listener_inst, event_context context) {
  switch (code) {
    case EVENT_CODE_UI_SHOW_PAUSE_MENU: {
      state->b_show_pause_menu = !state->b_show_pause_menu;
    }
  };

  return false;
}

#undef PSPRITESHEET_SYSTEM
