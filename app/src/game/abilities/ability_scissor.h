#ifndef ABILITY_SCISSOR_H
#define ABILITY_SCISSOR_H

#include "game/game_types.h"

bool ability_scissor_initialize(const camera_metrics *const _camera_metrics, const app_settings *const settings, const ingame_info *const _ingame_info);

ability get_ability_scissor(void);
ability get_ability_scissor_next_level(ability abl);

void update_ability_scissor(ability& abl);
void render_ability_scissor(ability& abl);

void refresh_ability_scissor(ability& abl);
void upgrade_ability_scissor(ability& abl);

#endif
