

#ifndef SPAWN_H
#define SPAWN_H

#include "defines.h"

bool spawn_system_initialize();
bool spawn_character(Character2D character, actor_type type);

spawn_system_state* get_spawn_system();
void clean_up_spawn_system();

void kill_spawn(u16 _id);

bool update_spawns(Vector2 player_position);
bool render_spawns();


#endif
