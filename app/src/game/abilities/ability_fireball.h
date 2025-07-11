
#ifndef ABILITY_FIREBALL_H
#define ABILITY_FIREBALL_H

#include "game/game_types.h"

[[__nodiscard__]] bool ability_fireball_initialize(const camera_metrics* _camera_metrics,const app_settings* settings,const ingame_info* _ingame_info);

ability get_ability_fireball(void);
ability get_ability_fireball_next_level(ability abl);
void upgrade_ability_fireball(ability* abl);
void refresh_ability_fireball(ability* abl);

void update_ability_fireball(ability* system);
void render_ability_fireball(ability* system);

#endif
