
#ifndef CAMERA_H
#define CAMERA_H

#include <defines.h>

void create_camera(Vector2 position);

camera_metrics* get_in_game_camera(void);

void set_camera_position(Vector2 pos);

bool update_camera(Vector2 position);

#endif
