
#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "defines.h"

bool scene_manager_initialize(void);

void update_scene_scene(void);
void render_scene_world(void);
void render_scene_interface(void);

void set_current_scene_type(scene_type type);
scene_type get_current_scene_type(void);
Vector2 get_spectator_position(void);

#endif
