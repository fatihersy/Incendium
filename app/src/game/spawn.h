

#ifndef SPAWN_H
#define SPAWN_H

#include "defines.h"

bool spawn_system_initialize();
bool spawn_character(Character2D character, actor_type type);

Character2D* get_spawn_data();
bool update_spawns();
bool render_spawns();


#endif
