
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
  i32 id;
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
  bool is_scrolling_active;
  
  data128 buffer;
  panel(void) {
    this->id            = 0;
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
    this->is_scrolling_active = false;
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
  panel(i32 id, button_state signal_state, atlas_texture_id bg_tex_id, atlas_texture_id frame_tex_id, Vector4 offsets, Color bg_tint, Color hover_tint = Color { 52, 64, 76, 245}) : panel() {
    this->id = id;
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
  Color forground_color_btn_state_up;
  Color forground_color_btn_state_hover;
  Color forground_color_btn_state_pressed;

  bool should_center;
  button_type(void) {
    this->id = BTN_TYPE_UNDEFINED;
    this->ss_type = SHEET_ID_SPRITESHEET_UNSPECIFIED;
    this->source_frame_dim = ZEROVEC2;
    this->dest_frame_dim = ZEROVEC2;
    this->scale = 0.f;
    this->should_center = false;
    this->forground_color_btn_state_up = WHITE;
    this->forground_color_btn_state_hover = WHITE;
    this->forground_color_btn_state_pressed = WHITE;
  }
  button_type(button_type_id id, spritesheet_id ss_type, Vector2 source_frame_dim, Vector2 dest_frame_dim, f32 scale, bool should_center, 
    Color _forground_color_btn_state_up = WHITE,
    Color _forground_color_btn_state_hover = WHITE,
    Color _forground_color_btn_state_pressed = WHITE
  ) {
    this->id = id;
    this->ss_type = ss_type;
    this->source_frame_dim = source_frame_dim;
    this->dest_frame_dim = dest_frame_dim;
    this->scale = scale;
    this->should_center = should_center;
    this->forground_color_btn_state_up = _forground_color_btn_state_up;
    this->forground_color_btn_state_hover = _forground_color_btn_state_hover;
    this->forground_color_btn_state_pressed = _forground_color_btn_state_pressed;
  }
} button_type;

typedef struct button {
  button_id id;
  button_type btn_type;
  button_state current_state;
  button_state signal_state;
  Rectangle dest;
  bool on_screen;
  bool is_registered;
  button(void) {
    this->id = BTN_ID_UNDEFINED;
    this->btn_type = button_type();
    this->current_state = BTN_STATE_UNDEFINED;
    this->signal_state  = BTN_STATE_UNDEFINED;
    this->dest = ZERORECT;
    this->on_screen = false;
    this->is_registered = false;
  }
  button(button_id id, button_type btn_type, button_state state, Rectangle dest) : button() {
    this->id = id;
    this->btn_type = btn_type;
    this->signal_state  = state;
    this->dest = dest;
    this->is_registered = true;
  }
} button;

typedef struct local_button {
  i32 id;
  button_type btn_type;
  button_state current_state;
  button_state signal_state;
  Rectangle dest;
  bool on_screen;
  bool is_active;

  local_button(void) {
    this->id = 0;
    this->btn_type = button_type();
    this->current_state = BTN_STATE_UNDEFINED;
    this->signal_state  = BTN_STATE_UNDEFINED;
    this->dest = ZERORECT;
    this->on_screen = false;
    this->is_active = false;
  }
  local_button(i32 id, button_type btn_type, button_state state, Rectangle dest) : local_button() {
    this->id = id;
    this->btn_type = btn_type;
    this->signal_state  = state;
    this->dest = dest;
    this->is_active = true;
  }
} local_button;
  
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
  progress_bar_type_id id;
  atlas_texture_id body_inside;
  atlas_texture_id body_outside;
  shader_id mask_shader_id;
  Rectangle body_inside_scissor;
  Rectangle strecth_part_inside;
  Rectangle strecth_part_outside;
  progress_bar_type(void) {
    this->id = PRG_BAR_TYPE_ID_UNDEFINED;
    this->body_inside = ATLAS_TEX_ID_UNSPECIFIED;
    this->body_outside = ATLAS_TEX_ID_UNSPECIFIED;
    this->mask_shader_id = SHADER_ID_UNSPECIFIED;
    this->body_inside_scissor = ZERORECT;
  }
  progress_bar_type(
    progress_bar_type_id _type_id, atlas_texture_id body_inside, atlas_texture_id body_outside, shader_id mask_shader_id, 
    Rectangle body_inside_scissor, Rectangle strecth_part_inside, Rectangle strecth_part_outside) : progress_bar_type() {
    this->id = _type_id;
    this->body_inside = body_inside;
    this->body_outside = body_outside;
    this->mask_shader_id = mask_shader_id;
    this->body_inside_scissor = body_inside_scissor;
    this->strecth_part_inside = strecth_part_inside;
    this->strecth_part_outside = strecth_part_outside;
  }
} progress_bar_type;

typedef struct progress_bar {
  progress_bar_id id;
  progress_bar_type type;
  Rectangle dest;
  f32 progress;
  bool is_initialized;
  progress_bar(void) {
    this->id = PRG_BAR_ID_UNDEFINED;
    this->type = progress_bar_type();
    this->dest = ZERORECT;
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

  data128 data;
  void (*on_change_complete)(data128);

  ui_fade_control_system(void) {
    this->fade_animation_duration = 0;
    this->fade_animation_timer = 0;
    this->fade_type = FADE_TYPE_UNDEFINED;
    this->fade_animation_playing = false;
    this->is_fade_animation_played = false;
    this->data = data128();
    this->on_change_complete = nullptr;
  }
} ui_fade_control_system;

typedef enum error_display_animation_state {
  ERROR_DISPLAY_ANIMATION_STATE_UNDEFINED,
  ERROR_DISPLAY_ANIMATION_STATE_MOVE_IN,
  ERROR_DISPLAY_ANIMATION_STATE_STAY,
  ERROR_DISPLAY_ANIMATION_STATE_MOVE_OUT,
  ERROR_DISPLAY_ANIMATION_STATE_MAX,
} error_display_animation_state;

typedef struct ui_error_display_control_system {
  error_display_animation_state display_state;
  std::string error_text;
  Vector2 location;
  f32 duration;
  f32 accumulator;
  panel bg_panel;
  ui_error_display_control_system(void) {
    this->display_state = ERROR_DISPLAY_ANIMATION_STATE_UNDEFINED;
    this->error_text = std::string("");
    this->location = ZEROVEC2;
    this->duration = 0.f; 
    this->accumulator = 0.f;
    this->bg_panel = panel();
  }
} ui_error_display_control_system;

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
Vector2 ui_measure_text(const char* in_str, font_type in_font_type, f32 in_font_size);
bool is_ui_fade_anim_complete(void);
void is_ui_fade_anim_reset(void);
void ui_refresh_setting_sliders_to_default(void);
void process_fade_effect(ui_fade_control_system* fade);

bool ui_set_slider_current_index(slider_id id, u16 index);
bool ui_set_slider_current_value(slider_id id, slider_option value);

bool gui_slider_add_option(slider_id _id, data_pack content, u32 _localization_symbol, std::string _no_localized_text);
const Rectangle * get_atlas_texture_source_rect(atlas_texture_id _id);
const std::array<button_type, BTN_TYPE_MAX>* get_button_types(void); 

bool gui_menu_button(const char* text, button_id _id, Vector2 grid, Vector2 grid_location, bool play_on_click_sound);
bool gui_mini_button(const char* text, button_id _id, Vector2 grid, bool play_on_click_sound);
bool gui_draw_local_button(const char* text, local_button* btn, font_type _font_type, i32 font_size_scale, Vector2 pos, text_alignment align_to, bool play_on_click_sound);
bool gui_slider_button(button_id _id, Vector2 pos);
void gui_checkbox_grid(checkbox_id _id, Vector2 grid, Vector2 grid_location);
void gui_slider(slider_id _id, Vector2 pos, Vector2 grid);
void gui_draw_texture_to_background(texture_id _id);
void gui_draw_spritesheet_to_background(spritesheet_id _id, Color _tint);
void gui_progress_bar(progress_bar_id bar_id, Rectangle dest, bool _should_center, Color tint = BLANK, Color outside_tint = WHITE);
void gui_panel(panel pan, Rectangle dest, bool _should_center);
bool gui_panel_active(panel* panel, Rectangle dest, bool _should_center);
void gui_label_box(const char* text, font_type type, i32 font_size, Rectangle dest, Color tint, text_alignment alignment);
void gui_label(const char* text, font_type type, i32 font_size, Vector2 position, Color tint, bool _center_h, bool _center_v);
void gui_label_shader(const char* text, shader_id sdr_id, font_type type, i32 font_size, Vector2 position, Color tint, bool _center_h, bool _center_v);
void gui_label_wrap(const char* text, font_type type, i32 font_size, Rectangle position, Color tint, bool _should_center);
void gui_label_grid(const char* text, font_type type, i32 font_size, Vector2 position, Color tint, bool _center_h, bool _center_v, Vector2 grid_coord);
void gui_label_wrap_grid(const char* text, font_type type, i32 font_size, Rectangle position, Color tint, bool _should_center, Vector2 grid_pos);
void gui_draw_atlas_texture_id_pro(atlas_texture_id _id, Rectangle src, Rectangle dest, bool should_center, i32 texture_wrap = TEXTURE_WRAP_REPEAT, Color tint = WHITE);
void gui_draw_atlas_texture_id(atlas_texture_id _id, Rectangle dest, Vector2 origin, f32 rotation, Color tint = WHITE); 
void gui_draw_atlas_texture_id_scale(atlas_texture_id _id, Vector2 position, f32 scale, Color tint, bool should_center); 
void gui_draw_atlas_texture_id_pro_grid(atlas_texture_id _id, Rectangle src, Rectangle dest, bool relative);
void gui_draw_texture_id_pro(texture_id _id, Rectangle src, Rectangle dest, Color tint = WHITE, Vector2 origin = ZEROVEC2);
void gui_draw_texture_id(const texture_id _id, const Rectangle dest, const Vector2 origin);
void gui_draw_spritesheet_id(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, u16 frame); 
void gui_draw_settings_screen(void);
void gui_draw_pause_screen(bool in_game_play_state);
void gui_fire_display_error(const char * text);
 
// Exposed
void ui_play_sprite_on_site(spritesheet *sheet, Color _tint, Rectangle dest);
void ui_set_sprite(spritesheet *sheet, bool _play_looped, bool _play_once);
void ui_update_sprite(spritesheet *sheet);

#define gui_label_box_format(FONT, FONT_SIZE, RECT, COLOR, ALIGN, TEXT, ...) gui_label_box(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, RECT, COLOR, ALIGN)
#define gui_label_format(FONT, FONT_SIZE, X,Y, COLOR, CENTER_H, CENTER_V, TEXT, ...) gui_label(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, Vector2{X,Y}, COLOR, CENTER_H, CENTER_V)
#define gui_label_format_v(FONT, FONT_SIZE, POS, COLOR, CENTER_H, CENTER_V, TEXT, ...) gui_label(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, POS, COLOR, CENTER_H, CENTER_V)

void user_interface_system_destroy(void);

#endif
