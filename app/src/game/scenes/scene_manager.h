
#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "defines.h"

[[__nodiscard__]] bool scene_manager_initialize(app_settings * _in_app_settings);

void update_scene_scene(void);
void render_scene_world(void);
void render_scene_interface(void);


#endif
