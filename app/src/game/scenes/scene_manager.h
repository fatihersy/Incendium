
#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "defines.h"

bool scene_manager_initialize(Vector2 _screen_size, Vector2 _screen_half_size);

void update_scene_scene();
void render_scene_world();
void render_scene_interface();

void set_current_scene_type(scene_type type);
scene_type get_current_scene_type();
Vector2 get_spectator_position();

#endif