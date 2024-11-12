
#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include "defines.h"

typedef enum button_type { UNDEFINED, STANDARD, SQUARE } button_type;

void user_interface_system_initialize();

void update_user_interface(Vector2 offset, Vector2 _screen_half_size,
                           scene_type _current_scene_type, Camera2D _camera);
void render_user_interface();

#endif
