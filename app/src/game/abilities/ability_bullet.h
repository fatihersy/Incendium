
#ifndef ABILITY_BULLET_H
#define ABILITY_BULLET_H

#include "game/game_types.h"

[[__nodiscard__]] bool ability_bullet_initialize(const camera_metrics *const _camera_metrics,const app_settings *const settings,const ingame_info *const _ingame_info);

ability get_ability_bullet(void);
ability get_ability_bullet_next_level(ability abl);
void upgrade_ability_bullet(ability *const abl);
void refresh_ability_bullet(ability *const abl);

void update_ability_bullet(ability *const system);
void render_ability_bullet(ability *const system);

#endif
