
#ifndef ABILITY_CODEX_H
#define ABILITY_CODEX_H

#include "game/game_types.h"

[[__nodiscard__]] bool ability_codex_initialize(const camera_metrics* _camera_metrics,const app_settings* settings,const ingame_info* _ingame_info);

ability get_ability_codex(void);
ability get_ability_codex_next_level(ability abl);
void upgrade_ability_codex(ability* abl);
void refresh_ability_codex(ability* abl);

void update_ability_codex(ability* system);
void render_ability_codex(ability* system);

#endif
