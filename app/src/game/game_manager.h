

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <defines.h>

bool game_manager_initialize(camera_metrics* _camera_metrics);

void update_game_manager();

player_state* get_player_state_if_available();
bool get_is_game_paused();
void set_is_game_paused(bool _is_game_paused);
void toggle_is_game_paused();

void damage_any_spawn(Character2D *projectile);
void damage_any_collider_by_type(Character2D *from_actor, actor_type to_type);
void add_collision(rectangle_collision rect);

bool _add_ability(ability_type _type);
Vector2 _get_player_position(bool centered);
bool _upgrade_ability(ability* abl);
u16 _spawn_character(Character2D _character);
void _set_player_position(Vector2 position);

void render_game();

#endif
