

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <game/game_types.h>

[[__nodiscard__]] bool game_manager_initialize(const camera_metrics* _camera_metrics, const app_settings * in_app_settings,tilemap ** const in_active_map_ptr);
void game_manager_cleanup_state(void);

void update_game_manager(void);
void update_game_manager_debug(void);

i32 get_currency_coins_total(void);
bool get_b_player_have_upgrade_points(void);
void set_dynamic_player_have_ability_upgrade_points(bool _b);

const ingame_info* gm_get_ingame_info(void);
const std::vector<character_trait>& gm_get_character_traits_all(void);
const std::array<item_data, ITEM_TYPE_MAX>& gm_get_default_items(void);
f32 gm_get_ingame_chance(ingame_chance chance_type, data128 context);
bool gm_upgrade_item_by_slot_ref(const player_inventory_slot& slot);

[[nodiscard]] bool gm_init_game(worldmap_stage stage, std::vector<character_trait>& _chosen_traits, ability_id starter_ability);
void gm_start_game(void);
void gm_end_game(bool is_win);
void gm_save_game(void);
void gm_load_game(void);
void gm_refresh_save_slot(void);
void gm_damage_spawn_if_collide(data128 coll_data, i32 damage, collision_type coll_check);
void gm_damage_player_if_collide(data128 coll_data, i32 damage, collision_type coll_check);
bool gm_refresh_game_rule_by_level(game_rule* rule, i32 level);
void gm_set_game_rule_level_by_id(game_rule_id rule_id, i32 level);
void currency_coins_add(i32 value);
void gm_add_to_inventory(item_type _item_type);
void gm_remove_from_inventory(item_type _item_type, i32 amouth);
void gm_set_chosen_traits(std::vector<character_trait> traits);
bool upgrade_ability_by_id(ability_id abl_id);
bool set_inventory_ui_ex(i32 slot_id, data128 data);

bool    _add_ability(ability_id _type);
void    _set_player_position(Vector2 position);
ability _get_next_level(ability abl);
const ability * _get_ability(ability_id _id);
const std::array<ability, ABILITY_ID_MAX> * _get_all_abilities(void);
const Character2D * _get_spawn_by_id(i32 _id);
const player_state * gm_get_player_state(void);

void render_game(void);

#endif
