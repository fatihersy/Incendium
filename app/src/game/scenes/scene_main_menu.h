

#ifndef SCENE_MAIN_MENU_H
#define SCENE_MAIN_MENU_H

#include "defines.h"

[[__nodiscard__]] bool initialize_scene_main_menu(const app_settings * _in_app_settings, bool fade_in);
bool begin_scene_main_menu(bool fade_in);
void end_scene_main_menu(void);

void update_scene_main_menu(void);
void render_scene_main_menu(void);
void render_interface_main_menu(void);

#endif
