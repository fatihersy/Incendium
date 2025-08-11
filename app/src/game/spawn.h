#ifndef SPAWN_H
#define SPAWN_H

#include "game_types.h"

[[nodiscard]] bool spawn_system_initialize(const camera_metrics* _camera_metrics, const ingame_info* _ingame_info);

bool update_spawns(Vector2 player_position);
bool render_spawns(void);

std::vector<Character2D>* get_spawns(void);

i32 spawn_character(Character2D _character);
i32 damage_spawn(i32 _id, i32 damage);
void clean_up_spawn_state(void);


#endif
