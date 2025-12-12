

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <game/game_types.h>

#define GM_SIGIL_HEAD_RAD_SCALE_BY_VIEWPORT_SIZE 0.0816f
#define GM_SIGIL_ARCH_RAD_SCALE_BY_VIEWPORT_SIZE 0.06885f
#define GM_SIGIL_COMMON_RAD_SCALE_BY_VIEWPORT_SIZE 0.0595f

#define GM_ITEM_COUNT_CHESTS_SCROLL 20

struct sigil_upgrade_result {
    enum E_result {
        UNDEFINED,
        IDLE,
        ERROR_LHS_TYPE_INCOMPATIBLE,
        ERROR_RHS_TYPE_INCOMPATIBLE,
        ERROR_LHS_LEVEL_OUT_OF_BOUND,
        ERROR_INSUFFICIENT,
        SUCCESS
    } type = UNDEFINED;
    bool success = false;
    i32 soul_requirement {};
    sigil_upgrade_result(i32 _soul_requirement) {
        this->soul_requirement = _soul_requirement;
    }
    sigil_upgrade_result(E_result _result) {
        this->type = _result;
        this->success = _result == E_result::SUCCESS;
    }
    sigil_upgrade_result(E_result _result, i32 _soul_requirement) {
        this->type = _result;
        this->success = _result == E_result::SUCCESS;
        this->soul_requirement = _soul_requirement;
    }
};

struct inventory_remove_amouth_result {
    bool found {};
    i32 remaining_amouth {};
};

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
const std::array<sigil_slot, SIGIL_SLOT_MAX>& gm_get_sigil_slots(void);
f32 gm_get_ingame_chance(ingame_chance chance_type, data128 context);
sigil_upgrade_result gm_get_sigil_upgrade_requirements(item_data& lhs, item_data& rhs);
sigil_upgrade_result gm_upgrade_sigil(item_data& lhs, item_data& rhs);

[[nodiscard]] bool gm_init_game(worldmap_stage stage, std::vector<character_trait>& _chosen_traits, ability_id starter_ability);
void gm_start_game(void);
void gm_end_game(bool is_win);
void gm_save_game(void);
void gm_load_game(save_slot_id slot_id);
void gm_refresh_save_slot(void);
void gm_damage_spawn_if_collide(data128 coll_data, i32 damage, collision_type coll_check);
void gm_damage_player_if_collide(data128 coll_data, i32 damage, collision_type coll_check);
bool gm_refresh_game_rule_by_level(game_rule * rule, i32 level);
bool gm_refresh_sigil(item_data * sigil);
void gm_refresh_item(item_data& item);
void gm_set_game_rule_level_by_id(game_rule_id rule_id, i32 level);
void currency_coins_add(i32 value);
void gm_add_to_inventory(item_type _item_type, data128 ig_buffer, data128 ui_buffer = data128(), i32 amouth = 1, i32 level = 1);
void gm_set_sigil_slot(sigil_slot sigil);
void gm_clear_sigil_slot(sigil_slot_id id);
void gm_set_sigil_ui_context(sigil_slot_id id, data128 ui_buffer, bool draw_item);
void gm_remove_from_inventory_by_id(i32 id);
inventory_remove_amouth_result gm_remove_from_inventory_by_amouth(i32 id, i32 amouth);
void gm_update_inventory_item_by_id(item_data data, i32 id);
void gm_set_chosen_traits(std::vector<character_trait> traits);
bool upgrade_ability_by_id(ability_id abl_id);
bool set_inventory_ui_ex(i32 slot_id, data128 data);

bool    _add_ability(ability_id _type);
void    _set_player_position(Vector2 position);
void    _get_next_level(ability& abl);
const ability& _get_ability(ability_id _id);
const std::array<ability, ABILITY_ID_MAX>& _get_all_abilities(void);
const Character2D * _get_spawn_by_id(i32 _id);
const player_state * gm_get_player_state(void);
f32 gm_get_player_sprite_scale(void);
const std::vector<player_inventory_slot>& gm_get_inventory(void);
const save_data& gm_get_save_data(save_slot_id id);
void gm_save_to_slot(save_slot_id id);

void render_game(void);
void gm_draw_sigil(item_type type, Vector2 position, bool should_center);

#endif
