
#ifndef CAMERA_H
#define CAMERA_H

#include <raylib.h>

void create_camera(Vector2 position);

Camera2D* get_active_camera();

void set_camera_position(Vector2 pos);

bool update_camera(Vector2 position);

#endif