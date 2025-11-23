
#ifndef ABILITY_FIRETRAIL_H
#define ABILITY_FIRETRAIL_H

#include "game/game_types.h"

[[__nodiscard__]] bool ability_firetrail_initialize(const camera_metrics *const _camera_metrics, const app_settings *const settings, const ingame_info *const _ingame_info);

ability get_ability_firetrail(void);
ability get_ability_firetrail_next_level(ability abl);
void upgrade_ability_firetrail(ability& abl);
void refresh_ability_firetrail(ability& abl);

void update_ability_firetrail(ability& system);
void render_ability_firetrail(ability& system);

#endif
