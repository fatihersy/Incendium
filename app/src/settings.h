
#ifndef SETTINGS_H
#define SETTINGS_H

#include "defines.h"

void settings_initialize(void);

bool set_settings_from_ini_file(const char* file_name);
bool save_ini_file(void);

bool set_resolution(u32 width, u32 height);
bool set_master_sound(u32 volume);

app_settings* get_app_settings(void);
i32 get_window_state(void);
Vector2* get_resolution_div2(void);
Vector2* get_resolution_div3(void);
Vector2* get_resolution_div4(void);
Vector2* get_resolution_3div2(void);
Vector2* get_resolution_5div4(void);
Vector2* get_resolution_35div20(void);
Vector2* get_resolution_38div20(void);
Vector2 get_screen_offset(void);

#endif 
