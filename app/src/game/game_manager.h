

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <defines.h>

bool game_manager_initialize(camera_metrics* _camera_metrics);

void update_game_manager();

game_manager_system_state* get_game_manager();
float get_time_elapsed(elapse_time_type type);

player_state* get_player_state_if_available();


void damage_any_spawn(Character2D *projectile);
void damage_any_collider_by_type(Character2D *from_actor, actor_type to_type);

Vector2 _get_player_position(bool centered);
bool _add_ability(ability_type _type);
bool _upgrade_ability(ability* abl);
void _set_player_position(Vector2 position);
u16 _spawn_character(Character2D _character);
void _update_spawns();
void _update_player();
void _render_player();
void _render_spawns();
void _render_map();

#endif
