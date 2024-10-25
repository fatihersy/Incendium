

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include "defines.h"

bool game_manager_initialize();

void set_player_position(i16 x, i16 y);

Character2D* get_actor_by_id(u32 ID);


#endif