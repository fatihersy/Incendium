

#ifndef SCENE_IN_GAME_H
#define SCENE_IN_GAME_H

#include <defines.h>

bool initialize_scene_in_game(camera_metrics* _camera_metrics);

void end_scene_in_game(void);

void update_scene_in_game(void);
void render_scene_in_game(void);
void render_interface_in_game(void);

#endif

