
#ifndef SCENE_EDITOR_H
#define SCENE_EDITOR_H

#include <game/game_types.h>

[[__nodiscard__]] bool initialize_scene_editor(const app_settings *const _in_app_settings, bool fade_in);
void end_scene_editor(void);

void update_scene_editor(void);
void render_scene_editor(void);
void render_interface_editor(void);

#endif
