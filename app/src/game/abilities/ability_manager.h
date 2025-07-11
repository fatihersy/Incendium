
#ifndef ABILITY_MANAGER_H
#define ABILITY_MANAGER_H

#include "game/game_types.h"

[[__nodiscard__]] bool ability_system_initialize(const camera_metrics* _camera_metrics,const app_settings* settings,const ingame_info* _ingame_info);

ability get_ability(ability_type _type);
ability get_next_level(ability abl);
void upgrade_ability(ability* abl);
void refresh_ability(ability* abl);

void update_abilities(ability_play_system* system);
void render_abilities(ability_play_system* system);

#endif
