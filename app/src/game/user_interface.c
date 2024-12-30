#include "user_interface.h"

#include "core/event.h"
#include "core/fmemory.h"

#include "defines.h"
#include "game/user_interface.h"
#include "raylib.h"
#include <stdbool.h>

#define DEFAULT_MENU_BUTTON_SCALE 3
#define SPACE_BTW_Y(i, DIM_Y) (DIM_Y + DIM_Y/3.f) * i
#define SPACE_BTW_X(i, DIM_X) (DIM_X + DIM_X/3.f) * i

typedef struct user_interface_system_state {
  spritesheet_play_system spritesheet_system;
  player_state *p_player;
  Vector2 offset;
  Vector2 mouse_pos;
  button buttons[BTN_ID_MAX];
  button_type button_types[BTN_TYPE_MAX];
  slider sliders[SDR_ID_MAX];
  slider_type slider_types[SDR_TYPE_MAX];
  Font ui_font;
  u16 ui_font_size_35div20;
  u16 ui_font_size_3div2;
  u16 ui_font_size_div2;
  spritesheet ss_to_draw_bg;
  bool b_show_pause_menu;
  bool b_show_settings_menu;
} user_interface_system_state;

static user_interface_system_state *state;

#define PSPRITESHEET_SYSTEM state // Don't forget to undef very bottom of the file
#include "game/spritesheet.h"

bool user_interface_on_event(u16 code, void *sender, void *listener_inst, event_context context);
void update_buttons();
void update_sliders();

void register_button(Vector2 _attached_position, Vector2 offset, button_id _btn_id, button_type_id _btn_type_id);
void register_button_type(
  button_type_id _btn_type_id, spritesheet_type _ss_type, Vector2 frame_dim, f32 _scale, 
  bool _play_reflection, bool _play_crt, bool _should_center
);
void register_slider(Vector2 _pos, Vector2 offset, slider_id _sdr_id, slider_type_id _sdr_type_id, button_id _left_btn_id, button_id _right_btn_id);
void register_slider_type(
  slider_type_id _sdr_type_id, spritesheet_type _ss_sdr_body_type, f32 _scale, u16 _width_multiply,
  button_type_id _left_btn_type_id, button_type_id _right_btn_type_id, bool _should_center, u16 _char_limit
);
void render_slider_body(slider* sdr);
void draw_texture_regular(Texture2D* tex, Rectangle dest);
void draw_texture_type_regular(texture_type _type, Rectangle dest);
void gui_draw_panel(Rectangle dest, bool should_center);
void gui_draw_settings_screen();
Rectangle get_texture_source_rect(texture_type _type);
const char* stringtify_options(data_pack option, const char* parser, u16 character_limit);

void user_interface_system_initialize() {
  if (state) return;

  state = (user_interface_system_state *)allocate_memory_linear(sizeof(user_interface_system_state), true);
  state->ui_font = LoadFont(rs_path("quantico_bold.ttf"));
  if (state->ui_font.baseSize == 0) { // If custom font load failed
    state->ui_font = GetFontDefault();
  }
  state->ui_font_size_35div20 = state->ui_font.baseSize / 1.75f;
  state->ui_font_size_3div2 = state->ui_font.baseSize / 1.5f;
  state->ui_font_size_div2 = state->ui_font.baseSize / 2.f;

  register_button_type(
    BTN_TYPE_MENU_BUTTON, MENU_BUTTON, 
    (Vector2){80, 16}, DEFAULT_MENU_BUTTON_SCALE, 
    true, true, true
  );
  register_button_type(
    BTN_TYPE_MENU_BUTTON_NO_CRT, MENU_BUTTON, 
    (Vector2){80, 16}, DEFAULT_MENU_BUTTON_SCALE, 
    true, false, true
  );
  register_button_type(
    BTN_TYPE_SLIDER_LEFT_BUTTON, SLIDER_LEFT_BUTTON, 
    (Vector2){10, 10}, DEFAULT_MENU_BUTTON_SCALE, 
    false, false, false
  );
  register_button_type(
    BTN_TYPE_SLIDER_RIGHT_BUTTON, SLIDER_RIGHT_BUTTON, 
    (Vector2){10, 10}, DEFAULT_MENU_BUTTON_SCALE, 
    false, false, false
  );
  register_slider_type(
    SDR_TYPE_PERCENT, SLIDER_PERCENT, 
    DEFAULT_MENU_BUTTON_SCALE, DEFAULT_MENU_BUTTON_SCALE,
    BTN_TYPE_SLIDER_LEFT_BUTTON, BTN_TYPE_SLIDER_RIGHT_BUTTON,
    true, 6
  );
  register_slider_type(
    SDR_TYPE_OPTION, SLIDER_OPTION, 
    DEFAULT_MENU_BUTTON_SCALE, 2,
    BTN_TYPE_SLIDER_LEFT_BUTTON, BTN_TYPE_SLIDER_RIGHT_BUTTON,
    true, 6
  );

  // MAIN MENU
  register_button(
    (Vector2) { SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2 }, (Vector2) {0,0 },
    BTN_ID_MAINMENU_BUTTON_PLAY, BTN_TYPE_MENU_BUTTON_NO_CRT);

  register_button(
    (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,1 },
    BTN_ID_MAINMENU_BUTTON_EDITOR, BTN_TYPE_MENU_BUTTON_NO_CRT);

  register_button(
    (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,2 },
    BTN_ID_MAINMENU_BUTTON_SETTINGS, BTN_TYPE_MENU_BUTTON_NO_CRT);

  register_button(
    (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,3 },
    BTN_ID_MAINMENU_BUTTON_EXTRAS, BTN_TYPE_MENU_BUTTON_NO_CRT);

  register_button(
    (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,4 },
    BTN_ID_MAINMENU_BUTTON_EXIT, BTN_TYPE_MENU_BUTTON_NO_CRT);
  // MAIN MENU

  // EDITOR
  register_button(
    (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,1 },
    BTN_ID_EDITOR_BUTTON_LOAD_MAP, BTN_TYPE_MENU_BUTTON);

  register_button(
    (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,2 },
    BTN_ID_EDITOR_BUTTON_SAVE_MAP, BTN_TYPE_MENU_BUTTON);
  // EDITOR
  
  // USER INTERFACE
  register_button(
    (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,1 },
    BTN_ID_PAUSEMENU_BUTTON_RESUME, BTN_TYPE_MENU_BUTTON);

  register_button(
    (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,2 },
    BTN_ID_PAUSEMENU_BUTTON_SETTINGS, BTN_TYPE_MENU_BUTTON);

  register_button(
    (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,3 },
    BTN_ID_PAUSEMENU_BUTTON_MAIN_MENU, BTN_TYPE_MENU_BUTTON);

  register_button(
    (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0,4 },
    BTN_ID_PAUSEMENU_BUTTON_EXIT, BTN_TYPE_MENU_BUTTON);
  // USER INTERFACE

  // SETTINGS
  register_slider(
    (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, (Vector2) {0}, 
    SDR_ID_SETTINGS_SOUND_SLIDER,  SDR_TYPE_PERCENT, 
    BTN_ID_SETTINGS_SLIDER_SOUND_LEFT_BUTTON, BTN_ID_SETTINGS_SLIDER_SOUND_RIGHT_BUTTON
  );
  register_slider(
    (Vector2) {SCREEN_WIDTH_DIV2, SCREEN_WIDTH_DIV2}, (Vector2) {0,1}, 
    SDR_ID_SETTINGS_RES_SLIDER,  SDR_TYPE_OPTION, 
    BTN_ID_SETTINGS_SLIDER_RES_LEFT_BUTTON, BTN_ID_SETTINGS_SLIDER_RES_RIGHT_BUTTON
  );
  // SETTINGS

  gui_slider_add_option(SDR_ID_SETTINGS_RES_SLIDER, (data_pack) {
    .data.u16[0] = 1920,
    .data.u16[1] = 1080,
    .array_lenght = 2,
    .type_flag = DATA_TYPE_U16
  }, "x");
  gui_slider_add_option(SDR_ID_SETTINGS_RES_SLIDER, (data_pack) {
    .data.u16[0] = 2560,
    .data.u16[1] = 1440,
    .array_lenght = 2,
    .type_flag = DATA_TYPE_U16
  }, "x");
  gui_slider_add_option(SDR_ID_SETTINGS_RES_SLIDER, (data_pack) {
    .data.u16[0] = 3840,
    .data.u16[1] = 2160,
    .array_lenght = 2,
    .type_flag = DATA_TYPE_U16
  }, "x");



  event_register(EVENT_CODE_UI_SHOW_PAUSE_MENU, 0, user_interface_on_event);
  event_register(EVENT_CODE_UI_SHOW_SETTINGS_MENU, 0, user_interface_on_event);
}

void update_user_interface() {
  state->mouse_pos = GetMousePosition();
  state->offset = (Vector2) { SCREEN_OFFSET, SCREEN_OFFSET};
  update_sprite_renderqueue();

  update_buttons();
  update_sliders();
}

void update_buttons() {
  for (int i = 0; i < BTN_ID_MAX; ++i) {
    if (state->buttons[i].id == BTN_ID_UNDEFINED) {
      continue;
    }
    if (!state->buttons[i].on_screen) { continue; }

    button* btn = &state->buttons[i];
    if (CheckCollisionPointRec(state->mouse_pos, btn->dest)) {
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        btn->state = BTN_STATE_PRESSED;
        if(state->buttons[btn->id].btn_type.play_reflection)  {
          stop_sprite(state->buttons[btn->id].reflection_render_index, true);
        }
      } else {
        if (btn->state == BTN_STATE_PRESSED) { 
          btn->state = BTN_STATE_RELEASED;
        }
        else if (btn->state != BTN_STATE_HOVER) {
          btn->state = BTN_STATE_HOVER;
        }
      }
    } else {
      if (btn->state != BTN_STATE_UP) { 
        if(state->buttons[btn->id].btn_type.play_reflection)  {
          reset_sprite(btn->reflection_render_index, true);
        }
        btn->state = BTN_STATE_UP;
      }
    }
    btn->on_screen = false;
  }
}
void update_sliders() {
  for (int i = 0; i < SDR_ID_MAX; ++i) {
    if (state->sliders[i].id == SDR_ID_UNDEFINED || !state->sliders[i].on_screen) continue;
      slider* sdr = &state->sliders[i];
      if (!sdr->is_registered) {
        TraceLog(LOG_WARNING, "WARNING::user_interface::update_sliders()::Using slider didn't registered");
        sdr->on_screen = false;
        continue;
      }
      if (sdr->sdr_type.id != SDR_TYPE_PERCENT) { continue; } // Only percent type sliders needs update every frame

      Rectangle sdr_rect = (Rectangle) {
        sdr->position.x, sdr->position.y, 
        sdr->sdr_type.body_width * sdr->sdr_type.width_multiply, sdr->sdr_type.body_height
      };
    
      if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(state->mouse_pos, sdr_rect)) {
          f32 relative = state->mouse_pos.x - sdr_rect.x;
          f32 ratio = relative / sdr_rect.width;

          sdr->current_value = ratio * sdr->max_value + 1;
          FCLAMP(sdr->current_value, sdr->min_value, sdr->max_value);
      }
    }
    
    sdr->on_screen = false;
  }  
}

void render_user_interface() {
  
  if (state->b_show_pause_menu) {
    gui_draw_pause_screen();
  }

  if (state->b_show_settings_menu) {
    gui_draw_settings_screen();
  }
}

bool gui_button(const char* text, button_id _id) {
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
  _btn->on_screen = true;

  Vector2 text_measure;
  Vector2 text_pos;

  if (!TextIsEqual(text, "")) {
    text_measure = MeasureTextEx(state->ui_font, text, state->ui_font.baseSize, UI_FONT_SPACING);
    text_pos = (Vector2) {
    _btn->dest.x + _btn->dest.width /2.f - text_measure.x / 2.f, 
    _btn->dest.y + _btn->dest.height/2.f - text_measure.y / 2.f
    };
  }

  _btn_type->play_crt ? play_sprite_on_site(_btn->crt_render_index, WHITE, _btn->dest) : 0;

  Vector2 pos = (Vector2) {_btn->dest.x, _btn->dest.y};
  Vector2 draw_sprite_scale = (Vector2) {_btn->btn_type.scale,_btn->btn_type.scale};

  if (_btn->state == BTN_STATE_PRESSED) {
    draw_sprite_on_site(_btn->btn_type.ss_type, WHITE, pos, draw_sprite_scale, 1, false);
    if (!TextIsEqual(text, "")) {
      DrawTextEx(state->ui_font, text,
        (Vector2){.x = text_pos.x, .y = text_pos.y + 3},
        state->ui_font.baseSize, UI_FONT_SPACING,
        MYYELLOW);
    }
  } else {
    draw_sprite_on_site(_btn->btn_type.ss_type, WHITE, pos, draw_sprite_scale, 0, false);
    if (_btn->state == BTN_STATE_HOVER) {
      _btn_type->play_reflection ? play_sprite_on_site(_btn->reflection_render_index, WHITE, _btn->dest) : 0;
    }
    if (!TextIsEqual(text, "")) {
      DrawTextEx(state->ui_font, text,
        (Vector2){.x = text_pos.x, .y = text_pos.y - 3},
        state->ui_font.baseSize, UI_FONT_SPACING,
        MYYELLOW);
    }
  }
  return _btn->state == BTN_STATE_RELEASED;
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

void gui_slider(slider_id _id) {
  if (_id >= SDR_ID_MAX || _id <= SDR_ID_UNDEFINED || !state) {
    TraceLog(LOG_WARNING, "WARNING::user_interface::gui_slider()::One of recieved ids was out of bound");
    return;
  }
  slider* sdr = &state->sliders[_id];
  slider_type sdr_type = sdr->sdr_type;
  if (!sdr->is_registered || (sdr_type.id == SDR_TYPE_OPTION && sdr->max_value <= 0)) return;
 
  sdr->on_screen = true;

  render_slider_body(sdr);

  if (gui_button("", sdr_type.left_btn_id)) {
    if (sdr->current_value > sdr->min_value) {
      sdr->current_value--;
      TraceLog(LOG_INFO, "Current: %d", sdr->current_value);
    }
  }
  if (gui_button("", sdr_type.right_btn_id)) {
    if (sdr->current_value < sdr->max_value-1) {
      sdr->current_value++;
      TraceLog(LOG_INFO, "Current: %d", sdr->current_value);
    }
  }
}

void render_slider_body(slider* sdr) {
  slider_type sdr_type = sdr->sdr_type;

  switch (sdr->sdr_type.id) {
    case SDR_TYPE_PERCENT: {
      u16 scaled_value = sdr->current_value * sdr_type.scale;
      u16 scaled_max   = sdr->max_value     * sdr_type.scale;
      Vector2 draw_sprite_scale = (Vector2) {sdr_type.scale, sdr_type.scale};

      for (int i = 0; i < sdr_type.width_multiply; ++i) {
        Vector2 _pos = sdr->position;
        _pos.x += i * sdr_type.body_width; 

        draw_sprite_on_site(
        sdr_type.ss_sdr_body, WHITE, _pos, draw_sprite_scale, 
        FMIN(scaled_value, sdr->max_value), false
        );

        scaled_value -= scaled_value < sdr->max_value ? scaled_value : sdr->max_value;
      }
      break;
    }
    case SDR_TYPE_OPTION: {
      u16 total_body_width = sdr_type.body_width * sdr_type.width_multiply;
      u16 each_body_width = (total_body_width - ((sdr->max_value) * SCREEN_OFFSET)) / (sdr->max_value-1);
      f32 each_body_scale = (float)each_body_width / sdr_type.origin_body_width;
      Vector2 draw_sprite_scale = (Vector2) {each_body_scale, sdr_type.scale};
      Vector2 _pos_temp = (Vector2) {sdr->position.x + SCREEN_OFFSET, sdr->position.y};
      const char* text = TextFormat("%s", sdr->options[sdr->current_value].text);
      Vector2 text_measure = MeasureTextEx(state->ui_font, text, state->ui_font_size_35div20, UI_FONT_SPACING);

      for (int i = 1; i < sdr->max_value; ++i) {
        Vector2 _pos = _pos_temp;
        _pos.x += (each_body_width + SCREEN_OFFSET) * (i-1); 

        draw_sprite_on_site(
          sdr_type.ss_sdr_body, WHITE, 
          _pos, draw_sprite_scale, 
          sdr->current_value, false
        );
      }

      Vector2 text_pos = (Vector2) {
        sdr->position.x + total_body_width/2.f - text_measure.x / 2.f,
        sdr->position.y + sdr_type.body_height/2.f - text_measure.y / 2.f
      };

      DrawTextEx(state->ui_font, text, text_pos, state->ui_font_size_35div20, UI_FONT_SPACING, MYYELLOW);
      break;
    }

    default: TraceLog(LOG_WARNING, "WARNING::user_interface::render_slider_body()::Unsupported slider type");
    break;
  }
}

void gui_draw_settings_screen() {

  gui_slider(SDR_ID_SETTINGS_SOUND_SLIDER);

  gui_slider(SDR_ID_SETTINGS_RES_SLIDER);
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
    .x = SCREEN_OFFSET, .y = SCREEN_OFFSET, 
    .width = SCREEN_WIDTH - SCREEN_OFFSET, .height = SCREEN_HEIGHT - SCREEN_OFFSET}, false
  );

  if (gui_button("Resume", BTN_ID_PAUSEMENU_BUTTON_RESUME)) {
    state->b_show_pause_menu = !state->b_show_pause_menu;
  }
  if (gui_button("Settings", BTN_ID_PAUSEMENU_BUTTON_SETTINGS)) {
    
  }
  if (gui_button("Main Menu", BTN_ID_PAUSEMENU_BUTTON_MAIN_MENU)) {
    state->b_show_pause_menu = false;
    event_fire(EVENT_CODE_SCENE_MAIN_MENU, 0, (event_context) {0});
  }
  if (gui_button("Exit", BTN_ID_PAUSEMENU_BUTTON_EXIT)) {
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context) {0});
  }
}


bool set_player_user_interface(player_state* player) {
  if (player->initialized) {
    state->p_player = player;
    return true;
  }

  return false;
}

bool gui_slider_add_option(slider_id _id, data_pack content, const char* _parser) {
  if (_id >= SDR_ID_MAX || _id <= SDR_ID_UNDEFINED || !state) {
    TraceLog(LOG_WARNING, "WARNING::user_interface::gui_slider_add_option()::Slider ids was out of bound");
    return false;
  }
  slider* sdr = &state->sliders[_id];
  if (!sdr->is_registered) {
    TraceLog(LOG_WARNING, "WARNING::user_interface::gui_slider_add_option()::Given slider didn't registered");
    return false;
  }
  sdr->options[sdr->max_value] = (slider_option) {
    .array_lenght = content.array_lenght,
    .type_flag = content.type_flag,
    .parser = *_parser,
    .text = 0
  };
  TextCopy(
    sdr->options[sdr->max_value].text, 
    stringtify_options(content, _parser, sdr->sdr_type.width_multiply * sdr->sdr_type.char_limit)
  );

  sdr->max_value++;

  return true;
}

const char* stringtify_options(data_pack content, const char* parser, u16 character_limit) {
  u16 length = content.array_lenght;
  char _temp[MAX_SLIDER_OPTION_TEXT_SLOT] = "";
  u64 total_written_size = 0;
  switch (content.type_flag) {
    case DATA_TYPE_I32: { 
      for (int i=0; i<length; ++i) {
        if(i != length-1) { // Unless the last
          const char* c = TextFormat("%u", content.data.i32[i]);
          copy_memory(_temp + total_written_size, c, TextLength(c)); 
          total_written_size += TextLength(c);
          copy_memory(_temp + total_written_size, parser, TextLength(parser)); 
          total_written_size += TextLength(parser);
        } 
        else { // Last
          const char* c = TextFormat("%u", content.data.i32[i]);
          copy_memory(_temp + total_written_size, c, TextLength(c)); 
        }
      }
      break;
    }
    case DATA_TYPE_U64: { 
      const char* c = TextFormat("%u", content.data.u64);
      copy_memory(_temp + total_written_size, c, TextLength(c)); 
    }
    case DATA_TYPE_U32: { 
      for (int i=0; i<length; ++i) {
        if(i != length-1) { // Unless the last
          const char* c = TextFormat("%u", content.data.u32[i]);
          copy_memory(_temp + total_written_size, c, TextLength(c)); 
          total_written_size += TextLength(c);
          copy_memory(_temp + total_written_size, parser, TextLength(parser)); 
          total_written_size += TextLength(parser);
        } 
        else { // Last
          const char* c = TextFormat("%u", content.data.u32[i]);
          copy_memory(_temp + total_written_size, c, TextLength(c)); 
        }
      }
      break;
    }
    case DATA_TYPE_U16: { 
      for (int i=0; i<length; ++i) {
        if(i != length-1) { // Unless the last
          const char* c = TextFormat("%u", content.data.u16[i]);
          copy_memory(_temp + total_written_size, c, TextLength(c)); 
          total_written_size += TextLength(c);
          copy_memory(_temp + total_written_size, parser, TextLength(parser)); 
          total_written_size += TextLength(parser);
        } 
        else { // Last
          const char* c = TextFormat("%u", content.data.u16[i]);
          copy_memory(_temp + total_written_size, c, TextLength(c)); 
        }
      }
      break;
    }
    case DATA_TYPE_F32: { 
      for (int i=0; i<length; ++i) {
        if(i != length-1) { // Unless the last
          const char* c = TextFormat("%u", content.data.f32[i]);
          copy_memory(_temp + total_written_size, c, TextLength(c)); 
          total_written_size += TextLength(c);
          copy_memory(_temp + total_written_size, parser, TextLength(parser)); 
          total_written_size += TextLength(parser);
        } 
        else { // Last
          const char* c = TextFormat("%u", content.data.f32[i]);
          copy_memory(_temp + total_written_size, c, TextLength(c)); 
        }
      }
      break;
    }
    case DATA_TYPE_C: { 
      copy_memory(_temp, content.data.c, TextLength(content.data.c)); 
    }
    default: TraceLog(
      LOG_WARNING, "WARNING::user_interface::stringtify_options()::Unsupported data type");
  }
  const char* text = _temp;
  return text;
}

void register_button_type(button_type_id _btn_type_id, spritesheet_type _ss_type, Vector2 frame_dim, f32 _scale, bool _play_reflection, bool _play_crt, bool _should_center) {
  if (_ss_type     >= SPRITESHEET_TYPE_MAX || _ss_type <= SPRITESHEET_UNSPECIFIED || 
      _btn_type_id >= BTN_TYPE_MAX         || _btn_type_id <= BTN_TYPE_UNDEFINED  ||
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
    .play_crt = _play_crt,
    .play_reflection = _play_reflection,
    .should_center = _should_center
  };

  state->button_types[_btn_type_id] = btn_type;
}

void register_button(Vector2 _attached_position, Vector2 offset, button_id _btn_id, button_type_id _btn_type_id) {
  if (_btn_id      >= BTN_ID_MAX   || _btn_id      <= BTN_ID_UNDEFINED   || 
      _btn_type_id >= BTN_TYPE_MAX || _btn_type_id <= BTN_TYPE_UNDEFINED || !state) 
  {
    TraceLog(LOG_WARNING, "WARNING::user_interface::register_button()::One of recieved ids was out of bound");
    return;
  }

  button_type* _btn_type = &state->button_types[_btn_type_id];

  u16 width = _btn_type->source_frame_dim.x * _btn_type->scale;
  u16 height = _btn_type->source_frame_dim.y * _btn_type->scale;
  f32 pos_x = _attached_position.x + SPACE_BTW_X(offset.x, width);
  f32 pos_y = _attached_position.y + SPACE_BTW_Y(offset.y, height);
  if (_btn_type->should_center) {
    pos_x -= width/2.f;
    pos_y -= height/2.f;
  }

  button btn = {
    .id = _btn_id,
    .btn_type = state->button_types[_btn_type_id],
    .dest = (Rectangle) {
      .x = pos_x, .y = pos_y,
      .width = width, .height = height
    },
    .state = BTN_STATE_UP,
    .crt_render_index = _btn_type->play_crt 
      ? register_sprite(BUTTON_CRT_SHEET, true, false, false)
      : 0,
    .reflection_render_index = _btn_type->play_reflection 
      ? register_sprite(BUTTON_REFLECTION_SHEET, false, true, false)
      : 0,
    .on_screen = false,
    .is_registered = true,
  };

  state->buttons[_btn_id] = btn;
}

void register_slider_type(
  slider_type_id _sdr_type_id, spritesheet_type _ss_sdr_body_type, f32 _scale, u16 _width_multiply,
  button_type_id _left_btn_type_id, button_type_id _right_btn_type_id, bool _should_center, u16 _char_limit) {
    if (_ss_sdr_body_type >= SPRITESHEET_TYPE_MAX || _ss_sdr_body_type <= SPRITESHEET_UNSPECIFIED || 
        _sdr_type_id      >= SDR_TYPE_MAX         || _sdr_type_id      <= SDR_TYPE_UNDEFINED      ||
        _left_btn_type_id >= BTN_TYPE_MAX         || _left_btn_type_id <= BTN_TYPE_UNDEFINED      ||
        _right_btn_type_id>= BTN_TYPE_MAX         || _right_btn_type_id<= BTN_TYPE_UNDEFINED      ||
        !state) {
      TraceLog(LOG_WARNING, "WARNING::user_interface::register_slider_type()::One of recieved ids was out of bound");
      return;
    }

  spritesheet ss_body = get_spritesheet_by_enum(_ss_sdr_body_type);
  button_type* left_btn_type = &state->button_types[_left_btn_type_id]; 
  button_type* right_btn_type = &state->button_types[_right_btn_type_id]; 

  slider_type sdr_type = {
    .id = _sdr_type_id,
    .scale = _scale,
    .ss_sdr_body = _ss_sdr_body_type,
    .source_frame_dim = (Vector2) {
      .x = ss_body.current_frame_rect.width, .y = ss_body.current_frame_rect.height
    },
    .left_btn_type_id = _left_btn_type_id,
    .right_btn_type_id = _right_btn_type_id,
    .left_btn_width = left_btn_type->source_frame_dim.x * left_btn_type->scale,
    .right_btn_width = right_btn_type->source_frame_dim.x * right_btn_type->scale,
    .origin_body_width = ss_body.current_frame_rect.width,
    .body_width = ss_body.current_frame_rect.width * _scale,
    .body_height = ss_body.current_frame_rect.height * _scale,
    .width_multiply = _width_multiply,
    .char_limit = _scale * _char_limit,
    .should_center = _should_center
  };

  state->slider_types[_sdr_type_id] = sdr_type;
}

void register_slider(Vector2 _pos, Vector2 offset, slider_id _sdr_id, slider_type_id _sdr_type_id, button_id _left_btn_id, button_id _right_btn_id) {
  if (_sdr_id       >= SDR_ID_MAX   || _sdr_id      <= SDR_ID_UNDEFINED  || 
      _left_btn_id  >= BTN_ID_MAX   || _left_btn_id <= BTN_ID_UNDEFINED  || 
      _right_btn_id >= BTN_ID_MAX   || _right_btn_id<= BTN_ID_UNDEFINED  || 
      _sdr_type_id  >= SDR_TYPE_MAX || _sdr_type_id <= SDR_TYPE_UNDEFINED||
      !state) 
  {
    TraceLog(LOG_WARNING, "WARNING::user_interface::register_slider()::One of recieved ids was out of bound");
    return;
  }

  slider_type* _sdr_type = &state->slider_types[_sdr_type_id];

  u16 width = _sdr_type->source_frame_dim.x * _sdr_type->scale;
  u16 height = _sdr_type->source_frame_dim.y * _sdr_type->scale;
  u16 pos_x = _pos.x + SPACE_BTW_X(offset.x, width);
  u16 pos_y = _pos.y + SPACE_BTW_Y(offset.y, height);
  if (_sdr_type->should_center) {
    pos_x -= (width*_sdr_type->width_multiply)/2.f;
    pos_y -= height/2.f;
  }

  slider sdr = {
    .id = _sdr_id,
    .sdr_type = *_sdr_type,
    .position = (Vector2) {
      .x = pos_x, .y = pos_y
    },
    .current_value = _sdr_type_id == SDR_TYPE_PERCENT ? 7 : 1,
    .max_value = _sdr_type_id == SDR_TYPE_PERCENT ? 10 : 1,
    .min_value = 1,
    .on_screen = false,
    .is_registered = true,
  };
  sdr.sdr_type.left_btn_id = _left_btn_id;
  sdr.sdr_type.right_btn_id = _right_btn_id;

  register_button(
    (Vector2) { sdr.position.x - sdr.sdr_type.left_btn_width, sdr.position.y}, (Vector2) {0},
    sdr.sdr_type.left_btn_id, sdr.sdr_type.left_btn_type_id);

  register_button(
    (Vector2) { sdr.position.x + width * sdr.sdr_type.width_multiply, sdr.position.y}, (Vector2) {0},
    sdr.sdr_type.right_btn_id, sdr.sdr_type.right_btn_type_id);

  state->sliders[_sdr_id] = sdr;
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
      break;
    }
    case EVENT_CODE_UI_SHOW_SETTINGS_MENU: {
      state->b_show_settings_menu = !state->b_show_settings_menu;
      break;
    }
  };

  return false;
}

#undef PSPRITESHEET_SYSTEM
