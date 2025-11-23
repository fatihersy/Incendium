
#ifndef ABILITY_RADIENCE_H
#define ABILITY_RADIENCE_H

#include "game/game_types.h"

[[__nodiscard__]] bool ability_radience_initialize(const camera_metrics *const _camera_metrics, const app_settings *const settings, const ingame_info *const _ingame_info);

ability get_ability_radience(void);
ability get_ability_radience_next_level(ability abl);
void upgrade_ability_radience(ability& abl);
void refresh_ability_radience(ability& abl);

void update_ability_radience(ability& system);
void render_ability_radience(ability& system);

#endif
