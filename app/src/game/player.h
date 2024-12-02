

#ifndef PLAYER_H
#define PLAYER_H

#include "defines.h"

bool player_system_initialize();

player_state* get_player_state();
Vector2 get_player_position(bool centered);

void add_exp_to_player(u32 exp);

bool update_player(scene_type _scene_data);
bool render_player();

#endif