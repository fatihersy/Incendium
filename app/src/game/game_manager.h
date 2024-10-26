

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include "defines.h"

bool game_manager_initialize();

void set_player_position(i16 x, i16 y);
Vector2 get_player_position();
Character2D* get_actor_by_id(u16 ID);

void damage_any_collade(Character2D* _character);



#endif