
#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include "defines.h"

void user_interface_system_initialize();

void update_user_interface();
void render_user_interface();

bool set_player_user_interface(player_state* player);

bool gui_button(button_id _id, bool play_crt);
void gui_draw_texture_to_background(texture_type _type);
void gui_draw_spritesheet_to_background(spritesheet_type _type);
void gui_healthbar(f32 percent);
void gui_draw_pause_screen();


void user_interface_system_destroy();

#endif
