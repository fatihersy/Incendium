

#ifndef SCENE_MAIN_MENU_H
#define SCENE_MAIN_MENU_H

#include "game/game_types.h"

[[__nodiscard__]] bool initialize_scene_main_menu(camera_metrics* _camera_metrics);
void end_scene_main_menu(void);

void update_scene_main_menu(void);
void render_scene_main_menu(void);
void render_interface_main_menu(void);

#endif
