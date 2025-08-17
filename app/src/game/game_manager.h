

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <game/game_types.h>

[[__nodiscard__]] bool game_manager_initialize(const camera_metrics* _camera_metrics, const app_settings * in_app_settings,tilemap ** const in_active_map_ptr);

void update_game_manager(void);
void update_game_manager_debug(void);

const i32* get_currency_souls_total(void);
bool get_b_player_have_upgrade_points(void);
void set_dynamic_player_have_ability_upgrade_points(bool _b);

const character_stat* get_static_player_state_stat(character_stat_id stat);
const ingame_info* gm_get_ingame_info(void);
const std::vector<character_trait>* gm_get_character_traits(void);

[[nodiscard]] bool gm_start_game(worldmap_stage stage);
void gm_end_game(bool is_win);
void gm_save_game(void);
void gm_load_game(void);
void gm_damage_spawn_if_collide(data128 coll_data, i32 damage, collision_type coll_check);
void gm_damage_player_if_collide(data128 coll_data, i32 damage, collision_type coll_check);
void game_manager_set_stat_value_by_level(character_stat* stat, i32 level);
void set_static_player_state_stat(character_stat_id stat_id, i32 level);
void currency_souls_add(i32 value);
void set_starter_ability(ability_id _id);

bool    _add_ability(ability_id _type);
bool    _upgrade_ability(ability* abl);
void    _set_player_position(Vector2 position);
ability _get_next_level(ability abl);
const ability * _get_ability(ability_id _id);
const std::array<ability, ABILITY_ID_MAX> * _get_all_abilities(void);
const Character2D * _get_spawn_by_id(i32 _id);

void render_game(void);

#endif
