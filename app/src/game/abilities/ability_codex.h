
#ifndef ABILITY_CODEX_H
#define ABILITY_CODEX_H

#include "game/game_types.h"

/** Initialize Codex module and bind runtime inputs. */
[[__nodiscard__]] bool ability_codex_initialize(const camera_metrics *const _camera_metrics, const app_settings *const settings, const ingame_info *const _ingame_info);

/** Create Codex ability configured as Lightning Bolt. */
ability get_ability_codex(void);

/** Return a copy reflecting next-level stats after upgrade. */
ability get_ability_codex_next_level(ability abl);

/** Apply upgrade rules to Codex ability. */
void upgrade_ability_codex(ability& abl);

/** Rebuild Codex projectiles and spritesheets. */
void refresh_ability_codex(ability& abl);

void update_ability_codex(ability& system);
void render_ability_codex(ability& system);

#endif
