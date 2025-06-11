
#ifndef ABILITY_MANAGER_H
#define ABILITY_MANAGER_H

#include "game_types.h"

bool ability_system_initialize(camera_metrics* _camera_metrics, app_settings* settings, ingame_info* _ingame_info);

ability get_ability(ability_type _type);
ability get_next_level(ability abl);
void upgrade_ability(ability* abl);
void refresh_ability(ability* abl);

void update_abilities(ability_play_system* system);
void render_abilities(ability_play_system* system);

#endif
