
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
      this->frame_tex_id  = ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL;
      this->bg_tex_id     = ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG;
      this->bg_tint       = Color { 30, 39, 46, 245};
      this->bg_hover_tint = Color { 52, 64, 76, 245};
      this->offsets       = Vector4 {6, 6, 6, 6};
      this->zoom          = 1.f;
      this->scroll        = 0;
      this->current_state = BTN_STATE_UP;
      this->signal_state  = BTN_STATE_UNDEFINED;
      this->dest          = Rectangle {};
      this->scroll_handle = Rectangle {};
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
  } button_type;
  
  typedef struct button {
    button_id id;
    button_type btn_type;
    button_state state;
    Rectangle dest;
    bool on_screen;
    bool is_registered;
  } button;
  
  typedef struct slider_option {
    std::string no_localized_text;
    u32 localization_symbol;
    data_pack content;
    slider_option() {
      this->no_localized_text = "NULL";
      this-> localization_symbol = 0;
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
      this->no_localized_text = "::ERROR";
      this->localization_symbol = symbol;
      this->content = content;
    }
  } slider_option;
  
  typedef struct slider_type {
    slider_type_id id;
    spritesheet_id ss_sdr_body;
    Vector2 source_frame_dim;
    f32 scale;
    u16 width_multiply;
    u16 char_limit;
    button_id left_btn_id = BTN_ID_UNDEFINED;
    button_id right_btn_id = BTN_ID_UNDEFINED;
    button_type_id left_btn_type_id;
    button_type_id right_btn_type_id;
    u16 left_btn_width;
    u16 right_btn_width;
    u16 origin_body_width;
    u16 body_width;
    u16 body_height;
    Vector2 whole_body_width = Vector2 {};
  } slider_type;
  
  typedef struct slider {
    slider_id id;
    slider_type sdr_type;
    Vector2 position;
    bool (*on_click)();
    bool (*on_left_button_trigger)();
    bool (*on_right_button_trigger)();
  
    std::array<slider_option, MAX_SLIDER_OPTION_SLOT> options = {};
    u16 current_value;
    u16 max_value;
    u16 min_value;
  
    bool localize_text; // Flag for sliders displaying only numbers, 
    bool is_clickable;
    bool on_screen;
    bool is_registered;  
  } slider;
  
  typedef struct progress_bar_type {
    //texture_id body_repetitive;
    atlas_texture_id body_inside;
    atlas_texture_id body_outside;
  
  
    shader_id mask_shader_id;
  } progress_bar_type;
  
  typedef struct progress_bar {
    progress_bar_id id;
    progress_bar_type type;
  
    f32 width_multiply;
    Vector2 scale;
  
    f32 progress;
    bool is_initialized;
  } progress_bar;

void user_interface_system_initialize(void);

void update_user_interface(void);
void render_user_interface(void);

Vector2 position_element(Vector2 grid, Vector2 pos, Vector2 dim, f32 f);

void register_slider(slider_id _sdr_id, slider_type_id _sdr_type_id, button_id _left_btn_id, button_id _right_btn_id, bool _is_clickable, bool _localize_text);

panel get_default_panel(void);
data_pack* get_slider_current_value(slider_id id);
Vector2* ui_get_mouse_pos(void);
Font* ui_get_font(font_type font);
slider* get_slider_by_id(slider_id sdr_id);
bool is_ui_fade_anim_complete(void);
bool is_ui_fade_anim_about_to_complete(void);

bool ui_set_slider_current_index(slider_id id, u16 index);
bool ui_set_slider_current_value(slider_id id, slider_option value);

bool gui_slider_add_option(slider_id _id, data_pack content, u32 _localization_symbol, std::string _no_localized_text);
Rectangle get_atlas_texture_source_rect(atlas_texture_id _id);

bool gui_menu_button(const char* text, button_id _id, Vector2 grid, f32 grid_scale, bool play_on_click_sound);
bool gui_mini_button(const char* text, button_id _id, Vector2 grid, f32 grid_scale, bool play_on_click_sound);
bool gui_slider_button(button_id _id, Vector2 pos);
void gui_slider(slider_id _id, Vector2 pos, Vector2 grid, f32 grid_scale);
void gui_draw_texture_to_background(texture_id _id);
void gui_draw_spritesheet_to_background(spritesheet_id _id, Color _tint);
void gui_progress_bar(progress_bar_id bar_id, Vector2 pos, bool _should_center);
void gui_panel(panel pan, Rectangle dest, bool _should_center);
bool gui_panel_active(panel* panel, Rectangle dest, bool _should_center);
void gui_label(const char* text, font_type type, i32 font_size, Vector2 position, Color tint, bool _center_h, bool _center_v);
void gui_label_wrap(const char* text, font_type type, i32 font_size, Rectangle position, Color tint, bool _should_center);
void gui_label_grid(const char* text, font_type type, i32 font_size, Vector2 position, Color tint, bool _center_h, bool _center_v, f32 grid_scale);
void gui_label_wrap_grid(const char* text, font_type type, i32 font_size, Rectangle position, Color tint, bool _should_center, f32 grid_scale);

void gui_draw_atlas_texture_id_pro(atlas_texture_id _id, Rectangle src, Rectangle dest, bool relative, bool should_center);
void gui_draw_atlas_texture_id(atlas_texture_id _id, Rectangle dest); 
void gui_draw_atlas_texture_id_scale(atlas_texture_id _id, Vector2 position, f32 scale, Color tint, bool should_center); 
void gui_draw_atlas_texture_id_pro_grid(atlas_texture_id _id, Rectangle src, Rectangle dest, bool relative, f32 grid_scale); 
void gui_draw_atlas_texture_id_grid(atlas_texture_id _id, Rectangle dest, f32 grid_scale); 
void gui_draw_atlas_texture_id_center(atlas_texture_id _id, Vector2 pos, Vector2 dim, bool should_center);
void gui_draw_texture_id_pro(texture_id _id, Rectangle src, Rectangle dest, bool should_center);
void gui_draw_texture_id(texture_id _id, Rectangle dest); 
void gui_draw_map_stage_pin(bool have_hovered, Vector2 screen_loc);
void gui_draw_spritesheet_id(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, u16 frame, bool _should_center); 
void gui_draw_settings_screen(void);
void gui_draw_pause_screen(bool in_game_play_state);

#define gui_label_format(FONT, FONT_SIZE, X,Y, COLOR, CENTER_H, CENTER_V, TEXT, ...) gui_label(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, Vector2{X,Y}, COLOR, CENTER_H, CENTER_V)
#define gui_label_format_v(FONT, FONT_SIZE, POS, COLOR, CENTER_H, CENTER_V, TEXT, ...) gui_label(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, POS, COLOR, CENTER_H, CENTER_V)
#define gui_label_format_grid(FONT, FONT_SIZE, X,Y, GRID_SCALE, COLOR, CENTER_H, CENTER_V, TEXT, ...) gui_label_grid(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, Vector2{X,Y}, COLOR, CENTER_H, CENTER_V, GRID_SCALE)
#define gui_label_format_v_grid(FONT, FONT_SIZE, POS, GRID_SCALE, COLOR, CENTER_H, CENTER_V, TEXT, ...) gui_label_grid(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, POS, COLOR, CENTER_H, CENTER_V, GRID_SCALE)

#define gui_panel_scissored(PANEL, CENTER, CODE)                                      \
    gui_panel(PANEL, PANEL.dest, CENTER);                                             \
    BeginScissorMode(PANEL.dest.x, PANEL.dest.y, PANEL.dest.width, PANEL.dest.height);\
    CODE                                                                              \
    EndScissorMode();

void user_interface_system_destroy(void);

#endif
