

#ifndef PLAYER_H
#define PLAYER_H

#include "defines.h"


bool player_system_initialize();

player_state* get_player_state();
Vector2 get_player_position(bool centered);

bool update_player();
bool render_player();

#endif