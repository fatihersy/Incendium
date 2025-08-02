

#ifndef SCENE_IN_GAME_H
#define SCENE_IN_GAME_H

#include <game/game_types.h>

[[__nodiscard__]] bool initialize_scene_in_game(const app_settings * _in_app_settings, bool fade_in);
[[__nodiscard__]] bool begin_scene_in_game(bool fade_in);
void end_scene_in_game(void);

void update_scene_in_game(void);
void render_scene_in_game(void);
void render_interface_in_game(void);

#endif

