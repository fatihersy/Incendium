
#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include "defines.h"

void user_interface_system_initialize();

void update_user_interface();
void render_user_interface();

bool gui_slider_add_option(slider_id _id, const char* _display_text, data_pack content);

bool gui_button(const char* text, button_id _id);
void gui_slider(slider_id _id);
void gui_draw_texture_to_background(texture_id _id);
void gui_draw_spritesheet_to_background(spritesheet_type _type, Color _tint);
void gui_progress_bar(progress_bar_id bar_id, Vector2 pos, bool _should_center);

void gui_draw_pause_screen();

void user_interface_system_destroy();

#endif
