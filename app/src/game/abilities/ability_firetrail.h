
#ifndef ABILITY_FIRETRAIL_H
#define ABILITY_FIRETRAIL_H

#include "game/game_types.h"

[[__nodiscard__]] bool ability_firetrail_initialize(camera_metrics* _camera_metrics, app_settings* settings, ingame_info* _ingame_info);

ability get_ability_firetrail(void);
ability get_ability_firetrail_next_level(ability abl);
void upgrade_ability_firetrail(ability* abl);
void refresh_ability_firetrail(ability* abl);

void update_ability_firetrail(ability* system);
void render_ability_firetrail(ability* system);

#endif
