
#ifndef CAMERA_H
#define CAMERA_H

#include <game/game_types.h>

bool create_camera(i32 width, i32 height, i32 render_width, i32 render_height);
bool update_camera(f32 delta_time);

const camera_metrics* get_in_game_camera(void);

#endif
