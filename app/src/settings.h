
#ifndef SETTINGS_H
#define SETTINGS_H

#include "defines.h"

void settings_initialize();

bool set_settings_from_ini_file(const char* file_name);

bool set_resolution(u32 width, u32 height);
bool set_master_sound(u32 volume);

app_settings* get_app_settings();
i32 get_window_state();
Vector2* get_resolution_div2();
Vector2* get_resolution_div3();
Vector2* get_resolution_div4();
Vector2* get_resolution_3div2();
Vector2* get_resolution_5div4();
Vector2* get_resolution_35div20();
Vector2* get_resolution_38div20();
u16 get_screen_offset();

#endif 