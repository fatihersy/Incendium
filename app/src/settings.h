
#ifndef SETTINGS_H
#define SETTINGS_H

#include "defines.h"

void settings_initialize(void);

bool set_settings_from_ini_file(const char* file_name);
bool save_ini_file(void);

void set_resolution(u32 width, u32 height);
bool set_master_sound(u32 volume);
void set_language(const char* lang);

app_settings* get_app_settings(void);
i32 get_window_state(void);
std::vector<f32> get_screen_offset(void);

#endif 
