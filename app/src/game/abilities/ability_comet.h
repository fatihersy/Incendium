
#ifndef ABILITY_COMET_H
#define ABILITY_COMET_H

#include "game/game_types.h"

[[__nodiscard__]] bool ability_comet_initialize(const camera_metrics *const _camera_metrics, const app_settings *const settings, const ingame_info *const _ingame_info);

ability get_ability_comet(void);
ability get_ability_comet_next_level(ability abl);
void upgrade_ability_comet(ability *const abl);
void refresh_ability_comet(ability *const abl);
 
void update_ability_comet(ability *const system);
void render_ability_comet(ability *const system);

#endif
