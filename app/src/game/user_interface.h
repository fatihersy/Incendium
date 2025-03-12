
#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include "defines.h"

typedef struct panel {
    texture_id frame_tex_id;
    texture_id bg_tex_id;
    Color bg_tint;
    Color bg_hover_tint;
    Vector4 offsets;
    f32 zoom;
    f32 scroll;
    button_state current_state;
    button_state signal_state;
    Rectangle dest;
    Rectangle scroll_handle;
    data_pack buffer[2];
    bool draggable;
    bool is_dragging_scroll;
  } panel;
  
  typedef struct button_type {
    button_type_id id;
    spritesheet_id ss_type;
    Vector2 source_frame_dim;
    Vector2 dest_frame_dim;
    f32 scale;
    Vector2 text_offset_on_click;
    bool play_reflection;
    bool play_crt;
    bool should_center;
  } button_type;
  
  typedef struct button {
    button_id id;
    button_type btn_type;
    button_state state;
    Rectangle dest;
  
    u16 crt_render_index;
    u16 reflection_render_index;
  
    bool on_screen;
    bool is_registered;
  } button;
  
  typedef struct slider_option {
    char display_text[MAX_SLIDER_OPTION_TEXT_SLOT];
    data_pack content;
  } slider_option;
  
  typedef struct slider_type {
    slider_type_id id;
    spritesheet_id ss_sdr_body;
    Vector2 source_frame_dim;
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
    u16 body_height;
    Vector2 whole_body_width;
  } slider_type;
  
  typedef struct slider {
    slider_id id;
    slider_type sdr_type;
    Vector2 position;
  
    slider_option options[MAX_SLIDER_OPTION_SLOT];
    u16 current_value;
    u16 max_value;
    u16 min_value;
  
    bool is_clickable;
    bool on_screen;
    bool is_registered;  
  } slider;
  
  typedef struct progress_bar_type {
    //texture_id body_repetitive;
    texture_id body_inside;
    texture_id body_outside;
  
  
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

Font* ui_get_font(font_type font);
panel get_default_panel(void);
data_pack* get_slider_current_value(slider_id id);
bool is_ui_fade_anim_complete();
bool is_ui_fade_anim_about_to_complete();

bool gui_slider_add_option(slider_id _id, const char* _display_text, data_pack content);

bool gui_menu_button(const char* text, button_id _id, Vector2 offset);
bool gui_mini_button(const char* text, button_id _id, Vector2 offset, f32 offset_scale);
bool gui_slider_button(button_id _id, Vector2 pos);
void gui_slider(slider_id _id, Vector2 pos, Vector2 offset, f32 offset_scale);
void gui_draw_texture_to_background(texture_id _id);
void gui_draw_spritesheet_to_background(spritesheet_id _id, Color _tint);
void gui_progress_bar(progress_bar_id bar_id, Vector2 pos, bool _should_center);
void gui_panel(panel pan, Rectangle dest, bool _should_center);
bool gui_panel_active(panel* panel, Rectangle dest, bool _should_center);
void gui_label(const char* text, font_type type, i32 font_size, Vector2 position, Color tint, bool _should_center);

void gui_draw_pause_screen(void);
void gui_draw_texture_id_pro(texture_id _id, Rectangle src, Rectangle dest); 
void gui_draw_texture_id(texture_id _id, Rectangle dest); 
void gui_draw_spritesheet_id(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, u16 frame, bool _should_center); 
void gui_draw_texture_id_center(texture_id _id, Vector2 pos, Vector2 dim, bool should_center);
void gui_draw_map_stage_pin(bool have_hovered, Vector2 screen_loc);

#define gui_label_format(FONT, FONT_SIZE, X,Y, COLOR, CENTER, TEXT, ...) gui_label(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, (Vector2){X,Y}, COLOR, CENTER)
#define gui_label_format_v(FONT, FONT_SIZE, VEC, COLOR, CENTER, TEXT, ...) gui_label(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, VEC, COLOR, CENTER)

#define gui_panel_scissored(PANEL, CENTER, CODE)        \
    gui_panel(PANEL, PANEL.dest, CENTER);                           \
    BeginScissorMode(PANEL.dest.x, PANEL.dest.y, PANEL.dest.width, PANEL.dest.height);\
    CODE                                                      \
    EndScissorMode();

void user_interface_system_destroy(void);

#endif
