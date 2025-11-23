#ifndef ABILITY_HARVESTER_H
#define ABILITY_HARVESTER_H

#include "/game/game_types.h"

bool ability_harvester_initialize(const camera_metrics *const _camera_metrics, const app_settings *const _settings, const ingame_info *const _ingame_info);

void upgrade_ability_harvester(ability& abl);

ability get_ability_harvester(void);
ability get_ability_harvester_next_level(ability abl);

void update_ability_harvester(ability& abl);
void render_ability_harvester(ability& abl);

void refresh_ability_harvester(ability& abl);

#endif 