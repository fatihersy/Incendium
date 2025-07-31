

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <game/game_types.h>

[[__nodiscard__]] bool game_manager_initialize(const camera_metrics* _camera_metrics, const app_settings * in_app_settings,tilemap ** const in_active_map_ptr);

void update_game_manager(void);
void update_game_manager_debug(void);

i32 get_currency_souls(void);
void set_currency_souls(i32 value);
bool get_b_player_have_upgrade_points(void);
void set_dynamic_player_have_ability_upgrade_points(bool _b);
void toggle_is_game_paused(void);

const character_stat* get_static_player_state_stat(character_stats stat);
const ingame_info* gm_get_ingame_info(void);
const std::vector<character_trait>* gm_get_character_traits(void);

[[nodiscard]] bool gm_start_game(worldmap_stage stage);
void gm_end_game(bool is_win);
void gm_save_game(void);
void gm_load_game(void);
void gm_damage_spawn_if_collide(data128 coll_data, i32 damage, collision_type coll_check);
void gm_damage_player_if_collide(data128 coll_data, i32 damage, collision_type coll_check);
void upgrade_dynamic_player_stat(character_stats stat_id, u16 level = 0);
void upgrade_static_player_stat(character_stats stat_id, u16 level = 0);
void refresh_player_stats(bool refresh_dynamic_state, bool refresh_static_state);
void upgrade_stat_pseudo(character_stat* stat);
void currency_souls_add(i32 value);

bool    _add_ability(ability_type _type);
bool    _upgrade_ability(ability* abl);
u16     _spawn_character(Character2D _character);
void    _set_player_position(Vector2 position);
ability _get_next_level(ability abl);
ability _get_ability(ability_type type);

void render_game(void);

#endif
