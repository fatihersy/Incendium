
#ifndef COLLECTIBLE_MANAGER_H
#define COLLECTIBLE_MANAGER_H

#include <game/game_types.h>

[[__nodiscard__]] bool collectible_manager_initialize(
    const camera_metrics* _camera_metrics, 
    const app_settings * in_app_settings, 
    const tilemap ** const in_active_map_ptr, 
    const ingame_info * in_ingame_info
);

bool update_collectible_manager(void);
bool render_collectible_manager(void);
//void update_collectible_manager_debug(void);

const loot_item * get_loot_by_id(i32 id);
const std::vector<loot_item> * get_loots_pointer(void);

loot_item * create_loot_item(item_type type, Vector2 position);

bool remove_item(i32 id);

#endif
