
#ifndef ABILITY_COMET_H
#define ABILITY_COMET_H

#include "game/game_types.h"

[[__nodiscard__]] bool ability_comet_initialize(const camera_metrics* _camera_metrics,const app_settings* settings,const ingame_info* _ingame_info);

ability get_ability_comet(void);
ability get_ability_comet_next_level(ability abl);
void upgrade_ability_comet(ability* abl);
void refresh_ability_comet(ability* abl);

void update_ability_comet(ability* system);
void render_ability_comet(ability* system);

#endif
