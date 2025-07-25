
#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include "game_types.h"

typedef enum button_state {
  BTN_STATE_UNDEFINED,
  BTN_STATE_UP,
  BTN_STATE_HOVER,
  BTN_STATE_PRESSED,
  BTN_STATE_RELEASED,
} button_state;

typedef enum checkbox_state {
  CHECKBOX_STATE_UNDEFINED,
  CHECKBOX_STATE_CHECKED,
  CHECKBOX_STATE_UNCHECKED,
  CHECKBOX_STATE_MAX,
} checkbox_state;

typedef struct panel {
  atlas_texture_id frame_tex_id;
  atlas_texture_id bg_tex_id;
  Color bg_tint;
  Color bg_hover_tint;
  Vector4 offsets;
  f32 zoom;
  f32 scroll;
  button_state current_state;
  button_state signal_state;
  Rectangle dest;
  Rectangle scroll_handle;
  bool draggable;
  bool is_dragging_scroll;
  
  data128 buffer;
  panel() {
      this->frame_tex_id  = ATLAS_TEX_ID_DARK_FANTASY_PANEL;
      this->bg_tex_id     = ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG;
      this->bg_tint       = Color { 30, 39, 46, 245};
      this->bg_hover_tint = Color { 52, 64, 76, 245};
      this->offsets       = Vector4 {6, 6, 6, 6};
      this->zoom          = 1.f;
      this->scroll        = 0;
      this->current_state = BTN_STATE_UP;
      this->signal_state  = BTN_STATE_UNDEFINED;
      this->dest          = ZERORECT;
      this->scroll_handle = ZERORECT;
      this->draggable     = false;
      this->is_dragging_scroll = false;
      this->buffer = data128();
  };
  panel(button_state signal_state, atlas_texture_id bg_tex_id, atlas_texture_id frame_tex_id, Vector4 offsets, Color bg_tint, Color hover_tint = Color { 52, 64, 76, 245}) : panel() {
      this->signal_state = signal_state;
      this->bg_tex_id = bg_tex_id;
      this->frame_tex_id = frame_tex_id;
      this->bg_tint = bg_tint;
      this->offsets = offsets;
      this->bg_hover_tint = hover_tint;
  }
} panel;
  
typedef struct button_type {
  button_type_id id;
  spritesheet_id ss_type;
  Vector2 source_frame_dim;
  Vector2 dest_frame_dim;
  f32 scale;
  bool should_center;
  button_type(void) {
    this->id = BTN_TYPE_UNDEFINED;
    this->ss_type = SHEET_ID_SPRITESHEET_UNSPECIFIED;
    this->source_frame_dim = ZEROVEC2;
    this->dest_frame_dim = ZEROVEC2;
    this->scale = 0.f;
    this->should_center = false;
  }
  button_type(button_type_id id, spritesheet_id ss_type, Vector2 source_frame_dim, Vector2 dest_frame_dim, f32 scale, bool should_center) {
    this->id = id;
    this->ss_type = ss_type;
    this->source_frame_dim = source_frame_dim;
    this->dest_frame_dim = dest_frame_dim;
    this->scale = scale;
    this->should_center = should_center;
  }
} button_type;
  
typedef struct button {
  button_id id;
  button_type btn_type;
  button_state state;
  Rectangle dest;
  bool on_screen;
  bool is_registered;
  button(void) {
    this->id = BTN_ID_UNDEFINED;
    this->btn_type = button_type();
    this->state = BTN_STATE_UNDEFINED;
    this->dest = ZERORECT;
    this->on_screen = false;
    this->is_registered = false;
  }
  button(button_id id, button_type btn_type, button_state state, Rectangle dest) : button() {
    this->id = id;
    this->btn_type = btn_type;
    this->state = state;
    this->dest = dest;
    this->is_registered = true;
  }
} button;
  
typedef struct checkbox_type {
  checkbox_type_id id;
  spritesheet_id ss_type;
  Vector2 source_frame_dim;
  Vector2 dest_frame_dim;
  f32 scale;
  bool should_center;
  checkbox_type(void) {
    this->id = CHECKBOX_TYPE_UNDEFINED;
    this->ss_type = SHEET_ID_SPRITESHEET_UNSPECIFIED;
    this->source_frame_dim = ZEROVEC2;
    this->dest_frame_dim = ZEROVEC2;
    this->scale = 0.f;
    this->should_center = false;
  }
  checkbox_type(checkbox_type_id id, spritesheet_id ss_type, Vector2 source_frame_dim, Vector2 dest_frame_dim, f32 scale, bool should_center) {
    this->id = id;
    this->ss_type = ss_type;
    this->source_frame_dim = source_frame_dim;
    this->dest_frame_dim = dest_frame_dim;
    this->scale = scale;
    this->should_center = should_center;
  }
} checkbox_type;

typedef struct checkbox {
  checkbox_id id;
  checkbox_type cb_type;
  checkbox_state state;
  
  bool (*pfn_on_change)(void);

  Rectangle dest;
  bool on_screen;
  bool is_registered;
  
  checkbox(void) {
    this->id = CHECKBOX_ID_UNDEFINED;
    this->cb_type = checkbox_type();
    this->state = CHECKBOX_STATE_UNCHECKED;
    this->dest = ZERORECT;
    this->on_screen = false;
    this->is_registered = false;
  }
  checkbox(checkbox_id id, checkbox_type btn_type, Rectangle dest) : checkbox() {
    this->id = id;
    this->cb_type = btn_type;
    this->dest = dest;
    this->is_registered = true;
  }
} checkbox;

typedef struct slider_option {
  std::string no_localized_text;
  u32 localization_symbol;
  data_pack content;
  slider_option(void) {
      this->no_localized_text = "::NULL";
      this->localization_symbol = 0;
      this->content = data_pack();
  }
  slider_option(const char* _str, u32 symbol, data_pack content) : slider_option() {
      this->no_localized_text = _str;
      this->localization_symbol = symbol;
      this->content = content;
  }
  slider_option(const char* _str, data_pack content) : slider_option() {
      this->localization_symbol = 0;
      this->no_localized_text = _str;
      this->content = content;
  }
  slider_option(u32 symbol, data_pack content) : slider_option() {
      this->no_localized_text = "::localized";
      this->localization_symbol = symbol;
      this->content = content;
  }
} slider_option;

typedef struct slider_type {
  slider_type_id id;
  spritesheet_id ss_sdr_body;
  Vector2 source_frame_dim;
  Vector2 dest_frame_dim;
  f32 scale;
  u16 width_multiply;
  u16 char_limit;
  button_id left_btn_id;
  button_id right_btn_id;
  button_type_id left_btn_type_id;
  button_type_id right_btn_type_id;
  u16 left_btn_width;
  u16 right_btn_width;
  u16 origin_body_width;
  u16 body_width;
  slider_type(void) {
    this->id = SDR_TYPE_UNDEFINED;
    this->ss_sdr_body = SHEET_ID_SPRITESHEET_UNSPECIFIED;
    this->source_frame_dim = ZEROVEC2;
    this->dest_frame_dim = ZEROVEC2;
    this->scale = 0.f;
    this->width_multiply = 0u;
    this->char_limit = 0u;
    this->left_btn_id = BTN_ID_UNDEFINED;
    this->right_btn_id = BTN_ID_UNDEFINED;
    this->left_btn_type_id = BTN_TYPE_UNDEFINED;
    this->right_btn_type_id = BTN_TYPE_UNDEFINED;
    this->left_btn_width = 0u;
    this->right_btn_width = 0u;
    this->origin_body_width = 0u;
    this->body_width = 0u;
  }
  slider_type(
    slider_type_id id, spritesheet_id ss_sdr_body, Vector2 source_frame_dim, f32 scale, u16 width_multiply, u16 char_limit, 
    button_type_id left_btn_type_id, button_type_id right_btn_type_id, u16 left_btn_width, u16 right_btn_width, u16 origin_body_width, u16 body_width) : slider_type() {
    this->id = id;
    this->ss_sdr_body = ss_sdr_body;
    this->source_frame_dim = source_frame_dim;
    this->scale = scale;
    this->width_multiply = width_multiply;
    this->char_limit = char_limit;
    this->left_btn_type_id = left_btn_type_id;
    this->right_btn_type_id = right_btn_type_id;
    this->left_btn_width =  left_btn_width;
    this->right_btn_width =  right_btn_width;
    this->origin_body_width =  origin_body_width;
    this->body_width = body_width;
  }
} slider_type;

typedef struct slider {
  slider_id id;
  slider_type sdr_type;
  Vector2 position;
  bool (*on_click)(void);
  bool (*on_left_button_trigger)(void);
  bool (*on_right_button_trigger)(void);

  std::vector<slider_option> options;
  i32 current_value;

  bool localize_text; // Flag for sliders displaying only numbers, 
  bool is_clickable;
  bool on_screen;
  bool is_registered;
  slider(void) {
    this->id = SDR_ID_UNDEFINED;
    this->sdr_type = slider_type();
    this->options.clear();
    this->position = ZEROVEC2;
    this->on_click = nullptr;
    this->on_left_button_trigger = nullptr;
    this->on_right_button_trigger = nullptr;
    this->current_value = 0u;
    this->localize_text = false; 
    this->is_clickable = false;
    this->on_screen = false;
    this->is_registered = false;
  }
  slider(slider_id _id, slider_type _type, u16 current_value, bool _localized_text, bool _is_clickable) : slider() {
    this->id = _id;
    this->sdr_type = _type;
    this->current_value = current_value;
    this->localize_text = _localized_text;
    this->is_clickable = _is_clickable;
    this->is_registered = true;
  }
} slider;

typedef struct progress_bar_type {
  //texture_id body_repetitive;
  atlas_texture_id body_inside;
  atlas_texture_id body_outside;
  shader_id mask_shader_id;
  progress_bar_type(void) {
    this->body_inside = ATLAS_TEX_ID_UNSPECIFIED;
    this->body_outside = ATLAS_TEX_ID_UNSPECIFIED;
    this->mask_shader_id = SHADER_ID_UNSPECIFIED;
  }
} progress_bar_type;

typedef struct progress_bar {
  progress_bar_id id;
  progress_bar_type type;

  f32 width_multiply;
  Vector2 scale;

  f32 progress;
  bool is_initialized;
  progress_bar(void) {
    this->id = PRG_BAR_ID_UNDEFINED;
    this->type = progress_bar_type();
    this->width_multiply = 0.f;
    this->scale = ZEROVEC2;
    this->progress = 0.f;
    this->is_initialized = false;
  }
} progress_bar;

typedef enum ui_fade_type {
  FADE_TYPE_UNDEFINED,
  FADE_TYPE_FADEIN,
  FADE_TYPE_FADEOUT,
  FADE_TYPE_MAX,
} ui_fade_type;

typedef struct ui_fade_control_system {
  i32 fade_animation_duration;
  i32 fade_animation_timer; // By frame
  ui_fade_type fade_type;
  bool fade_animation_playing;
  bool is_fade_animation_played;

  data64 data;
  void (*on_change_complete)(data64 data);

  ui_fade_control_system(void) {
    this->fade_animation_duration = 0;
    this->fade_animation_timer = 0;
    this->fade_type = FADE_TYPE_UNDEFINED;
    this->fade_animation_playing = false;
    this->is_fade_animation_played = false;
    this->data = data64();
    this->on_change_complete = nullptr;
  }
} ui_fade_control_system;

[[__nodiscard__]] bool user_interface_system_initialize(void);

void update_user_interface(void);
void render_user_interface(void);

Vector2 position_element_by_grid(Vector2 grid_location, Vector2 grid, Vector2 grid_dim);

void register_slider(slider_id _sdr_id, slider_type_id _sdr_type_id, button_id _left_btn_id, button_id _right_btn_id, bool _is_clickable, bool _localize_text);

data_pack* get_slider_current_value(slider_id id);
const Vector2* ui_get_mouse_pos_screen(void);
const Font* ui_get_font(font_type font);
slider* get_slider_by_id(slider_id sdr_id);
checkbox* get_checkbox_by_id(checkbox_id cb_id);
bool is_ui_fade_anim_complete(void);
void is_ui_fade_anim_reset(void);
void ui_refresh_setting_sliders_to_default(void);
void process_fade_effect(ui_fade_control_system* fade);

bool ui_set_slider_current_index(slider_id id, u16 index);
bool ui_set_slider_current_value(slider_id id, slider_option value);

bool gui_slider_add_option(slider_id _id, data_pack content, u32 _localization_symbol, std::string _no_localized_text);
Rectangle get_atlas_texture_source_rect(atlas_texture_id _id);

bool gui_menu_button(const char* text, button_id _id, Vector2 grid, Vector2 grid_location, bool play_on_click_sound);
bool gui_mini_button(const char* text, button_id _id, Vector2 grid, bool play_on_click_sound);
bool gui_slider_button(button_id _id, Vector2 pos);
void gui_checkbox_grid(checkbox_id _id, Vector2 grid, Vector2 grid_location);
void gui_slider(slider_id _id, Vector2 pos, Vector2 grid);
void gui_draw_texture_to_background(texture_id _id);
void gui_draw_spritesheet_to_background(spritesheet_id _id, Color _tint);
void gui_progress_bar(progress_bar_id bar_id, Vector2 pos, bool _should_center);
void gui_panel(panel pan, Rectangle dest, bool _should_center);
bool gui_panel_active(panel* panel, Rectangle dest, bool _should_center);
void gui_label(const char* text, font_type type, i32 font_size, Vector2 position, Color tint, bool _center_h, bool _center_v);
void gui_label_shader(const char* text, shader_id sdr_id, font_type type, i32 font_size, Vector2 position, Color tint, bool _center_h, bool _center_v);
void gui_label_wrap(const char* text, font_type type, i32 font_size, Rectangle position, Color tint, bool _should_center);
void gui_label_grid(const char* text, font_type type, i32 font_size, Vector2 position, Color tint, bool _center_h, bool _center_v, Vector2 grid_coord);
void gui_label_wrap_grid(const char* text, font_type type, i32 font_size, Rectangle position, Color tint, bool _should_center, Vector2 grid_pos);
void gui_draw_atlas_texture_id_pro(atlas_texture_id _id, Rectangle src, Rectangle dest, bool relative, bool should_center);
void gui_draw_atlas_texture_id(atlas_texture_id _id, Rectangle dest, Vector2 origin, f32 rotation); 
void gui_draw_atlas_texture_id_scale(atlas_texture_id _id, Vector2 position, f32 scale, Color tint, bool should_center); 
void gui_draw_atlas_texture_id_pro_grid(atlas_texture_id _id, Rectangle src, Rectangle dest, bool relative); 
void gui_draw_atlas_texture_id_grid(atlas_texture_id _id, Rectangle dest); 
void gui_draw_texture_id_pro(texture_id _id, Rectangle src, Rectangle dest, Color tint = WHITE, Vector2 origin = {0.f, 0.f});
void gui_draw_texture_id(texture_id _id, Rectangle dest);
void gui_draw_map_stage_pin(bool have_hovered, Vector2 screen_loc);
void gui_draw_spritesheet_id(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, u16 frame); 
void gui_draw_settings_screen(void);
void gui_draw_pause_screen(bool in_game_play_state);
 
// Exposed
void ui_play_sprite_on_site(spritesheet *sheet, Color _tint, Rectangle dest);
void ui_set_sprite(spritesheet *sheet, bool _play_looped, bool _play_once);
void ui_update_sprite(spritesheet *sheet);

#define gui_label_format(FONT, FONT_SIZE, X,Y, COLOR, CENTER_H, CENTER_V, TEXT, ...) gui_label(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, Vector2{X,Y}, COLOR, CENTER_H, CENTER_V)
#define gui_label_format_v(FONT, FONT_SIZE, POS, COLOR, CENTER_H, CENTER_V, TEXT, ...) gui_label(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, POS, COLOR, CENTER_H, CENTER_V)
#define gui_label_format_grid(FONT, FONT_SIZE, X,Y, GRID_SCALE, COLOR, CENTER_H, CENTER_V, TEXT, ...) gui_label_grid(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, Vector2{X,Y}, COLOR, CENTER_H, CENTER_V, GRID_SCALE)
#define gui_label_format_v_grid(FONT, FONT_SIZE, POS, GRID_SCALE, COLOR, CENTER_H, CENTER_V, TEXT, ...) gui_label_grid(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, POS, COLOR, CENTER_H, CENTER_V, GRID_SCALE)

void user_interface_system_destroy(void);

#endif
