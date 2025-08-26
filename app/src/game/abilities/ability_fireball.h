
#ifndef ABILITY_FIREBALL_H
#define ABILITY_FIREBALL_H

#include "game/game_types.h"

[[__nodiscard__]] bool ability_fireball_initialize(const camera_metrics *const _camera_metrics, const app_settings *const settings, const ingame_info *const _ingame_info);

ability get_ability_fireball(void);
ability get_ability_fireball_next_level(ability abl);
void upgrade_ability_fireball(ability *const abl);
void refresh_ability_fireball(ability *const abl);

void update_ability_fireball(ability *const system);
void render_ability_fireball(ability *const system);

#endif
