
#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include "defines.h"

void user_interface_system_initialize();

void update_user_interface();
void render_user_interface();

Font* ui_get_font(font_type font);
panel get_default_panel();
data_pack* get_slider_current_value(slider_id id);

bool gui_slider_add_option(slider_id _id, const char* _display_text, data_pack content);

bool gui_menu_button(const char* text, button_id _id);
bool gui_mini_button(const char* text, button_id _id);
bool gui_slider_button(button_id _id);
void gui_slider(slider_id _id);
void gui_draw_texture_to_background(texture_id _id);
void gui_draw_spritesheet_to_background(spritesheet_type _type, Color _tint);
void gui_progress_bar(progress_bar_id bar_id, Vector2 pos, bool _should_center);
void gui_panel(panel pan, Rectangle dest, bool _should_center);
bool gui_panel_active(panel* panel, Rectangle dest, bool _should_center);
void gui_label(const char* text, Vector2 position, Color tint);

void gui_draw_pause_screen();
void gui_draw_texture_id_pro(texture_id _id, Rectangle src, Rectangle dest);

#define gui_panel_scissored(PANEL, CENTER, CODE)        \
    gui_panel(PANEL, PANEL.dest, CENTER);                           \
    BeginScissorMode(PANEL.dest.x, PANEL.dest.y, PANEL.dest.width, PANEL.dest.height);\
    CODE                                                      \
    EndScissorMode();

void user_interface_system_destroy();

#endif
