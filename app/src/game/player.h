

#ifndef PLAYER_H
#define PLAYER_H

#include "defines.h"

bool player_system_initialize();

player_state* get_player_state();

bool update_player();
bool render_player();

#endif