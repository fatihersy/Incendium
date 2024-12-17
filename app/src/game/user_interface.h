
#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include "defines.h"

void user_interface_system_initialize();

void update_user_interface();
void render_user_interface();

bool set_player_user_interface(player_state* player);

void draw_to_background(texture_type _type);
bool gui_button(button_type _type);
void gui_healthbar(f32 percent);

void clear_interface_state();

#endif
