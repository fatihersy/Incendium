
#ifndef ABILITY_PENDULUM_H
#define ABILITY_PENDULUM_H

#include "game/game_types.h"

bool ability_pendulum_initialize(const camera_metrics *const _camera_metrics, const app_settings *const settings, const ingame_info *const _ingame_info);

ability get_ability_pendulum(void);
ability get_ability_pendulum_next_level(ability abl);

void update_ability_pendulum(ability& abl);
void render_ability_pendulum(ability& abl);

void refresh_ability_pendulum(ability& abl);
void upgrade_ability_pendulum(ability& abl);

#endif
