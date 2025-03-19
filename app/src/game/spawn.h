

#ifndef SPAWN_H
#define SPAWN_H

#include "defines.h"

bool spawn_system_initialize(void);
u16 spawn_character(Character2D _character);
Character2D *get_spawns();
u16 *get_spawn_count();

void clean_up_spawn_system(void);

u16 damage_spawn(u16 _id, u16 damage);

bool update_spawns(Vector2 player_position);
bool render_spawns(void);


#endif
