
#ifndef CAMERA_H
#define CAMERA_H

#include <game/game_types.h>

bool create_camera(i32 width, i32 height);

camera_metrics* get_in_game_camera(void);

void set_camera_position(Vector2 pos);

bool update_camera(Vector2 position, Vector2 offset);

#endif
