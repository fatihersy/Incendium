
#ifndef CAMERA_H
#define CAMERA_H

#include <game/game_types.h>

bool create_camera(i32 width, i32 height);
bool update_camera(void);

const camera_metrics* get_in_game_camera(void);

#endif
