

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include "defines.h"
#include "raylib.h"

bool game_manager_initialize(Vector2 screen_size, scene_type _scene_data);

void update_game_manager(scene_type _scene_data);

game_manager_system_state* get_game_manager();
float get_time_elapsed(elapse_time_type type);

player_state* get_player_state_if_available();

void damage_any_spawn(Character2D *projectile);
void damage_any_collider_by_type(Character2D *from_actor, actor_type to_type);

Vector2 _get_player_position(bool centered);
Vector2 _get_screen_half_size();
void _set_player_position(Vector2 position);
u16 _spawn_character(Character2D _character);
void _update_spawns();
void _update_player();
void _render_player();
void _render_spawns();

#endif
