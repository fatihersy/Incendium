
#ifndef ABILITY_MANAGER_H
#define ABILITY_MANAGER_H

#include "game/game_types.h"

[[__nodiscard__]] bool ability_system_initialize(const camera_metrics *const _camera_metrics, const app_settings *const settings, const ingame_info *const _ingame_info);

const ability& get_ability(ability_id _type);
const std::array<ability, ABILITY_ID_MAX>& get_all_abilities(void);
void get_next_level(ability& abl);
void upgrade_ability(ability& abl);
void refresh_ability(ability& abl);

void update_abilities(ability_play_system& system);
void render_abilities(ability_play_system& system);

#endif
