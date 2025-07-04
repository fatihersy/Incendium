
#ifndef SCENE_EDITOR_H
#define SCENE_EDITOR_H

#include <game/game_types.h>

[[__nodiscard__]] bool initialize_scene_editor(camera_metrics* _camera);
void end_scene_editor(void);

void update_scene_editor(void);
void render_scene_editor(void);
void render_interface_editor(void);

#endif
