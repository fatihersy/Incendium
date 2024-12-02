

#ifndef SCENE_IN_GAME_H
#define SCENE_IN_GAME_H

#include <stdbool.h>
#include <raylib.h>

bool initialize_scene_in_game(Vector2 _screen_size, Vector2 _screen_half_size);

void update_scene_in_game();
void render_scene_in_game();

#endif

