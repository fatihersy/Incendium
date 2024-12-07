
#ifndef CAMERA_H
#define CAMERA_H

#include "defines.h"

Camera2D create_camera(Vector2 position, u8 rotation);

Camera2D* get_active_camera();

bool update_camera(Vector2 position);

#endif