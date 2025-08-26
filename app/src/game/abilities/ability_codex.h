
#ifndef ABILITY_CODEX_H
#define ABILITY_CODEX_H

#include "game/game_types.h"

[[__nodiscard__]] bool ability_codex_initialize(const camera_metrics *const _camera_metrics, const app_settings *const settings, const ingame_info *const _ingame_info);

ability get_ability_codex(void);
ability get_ability_codex_next_level(ability abl);
void upgrade_ability_codex(ability *const abl);
void refresh_ability_codex(ability *const abl);

void update_ability_codex(ability *const system);
void render_ability_codex(ability *const system);

#endif
