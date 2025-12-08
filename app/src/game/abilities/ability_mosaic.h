#ifndef ABILITY_MOSAIC_H
#define ABILITY_MOSAIC_H

#include "game/game_types.h"

bool ability_mosaic_initialize(const camera_metrics *const _camera_metrics, const app_settings *const settings, const ingame_info *const _ingame_info);

ability get_ability_mosaic(void);
ability get_ability_mosaic_next_level(ability abl);

void update_ability_mosaic(ability& abl);
void render_ability_mosaic(ability& abl);

void refresh_ability_mosaic(ability& abl);
void upgrade_ability_mosaic(ability& abl);

#endif
