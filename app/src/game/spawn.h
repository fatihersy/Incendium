

#ifndef SPAWN_H
#define SPAWN_H

#include "defines.h"



bool spawn_system_initialize(camera_metrics* _camera_metrics);
bool spawn_character(Character2D _character);
Character2D *get_spawns(void);
u32 *get_spawn_count(void);

void clean_up_spawn_state(void);

i32 damage_spawn(u32 _id, i32 damage);

bool update_spawns(Vector2 player_position);
bool render_spawns(void);


#endif
