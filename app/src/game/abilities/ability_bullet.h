
#ifndef ABILITY_BULLET_H
#define ABILITY_BULLET_H

#include "game/game_types.h"

[[__nodiscard__]] bool ability_bullet_initialize(const camera_metrics* _camera_metrics,const app_settings* settings,const ingame_info* _ingame_info);

ability get_ability_bullet(void);
ability get_ability_bullet_next_level(ability abl);
void upgrade_ability_bullet(ability* abl);
void refresh_ability_bullet(ability* abl);

void update_ability_bullet(ability* system);
void render_ability_bullet(ability* system);

#endif
