

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <defines.h>

bool game_manager_initialize(camera_metrics* _camera_metrics);

void update_game_manager(void);

u16 get_remaining_enemies(void);
u32 get_currency_souls(void);
void set_currency_souls(i32 value);
bool get_b_player_have_upgrade_points(void);
ability* get_player_ability(ability_type type);
character_stat* get_player_stat(character_stats stat);
void set_player_have_ability_upgrade_points(bool _b);
bool get_is_game_paused(void);
void set_is_game_paused(bool _is_game_paused);
void toggle_is_game_paused(void);
bool get_is_game_end(void);
void set_is_game_end(bool _is_game_end);
void toggle_is_game_end(void);

void gm_start_game(worldmap_stage stage);
void gm_reset_game(void);
void gm_save_game(void);
void gm_load_game(void);
void damage_any_spawn(Character2D *projectile);
void damage_any_collider_by_type(Character2D from_actor, actor_type to_type);
void upgrade_player_stat(character_stat* stat);
void refresh_player_stats(void);
void currency_souls_add(i32 value);

bool _add_ability(ability_type _type);
Vector2 _get_player_position(bool centered);
bool _upgrade_ability(ability* abl);
u16 _spawn_character(Character2D _character);
void _set_player_position(Vector2 position);
ability _get_next_level(ability abl);
ability _get_ability(ability_type type);
player_state* _get_player_state(void);

void render_game(void);

#endif
