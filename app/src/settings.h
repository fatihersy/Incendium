
#ifndef SETTINGS_H
#define SETTINGS_H

#include "defines.h"

bool settings_initialize(void);

bool update_app_settings_state(void);

bool set_settings_from_ini_file(const char* file_name);
bool save_ini_file(void);

void set_resolution(i32 width, i32 height);
void set_window_size(i32 width, i32 height);
bool set_master_sound(i32 volume, bool save = false);
void set_language(const char* lang);
void set_active_save_slot(save_slot_id id);

app_settings * get_app_settings(void);
app_settings get_default_ini_file(void);
const app_settings * get_initializer_settings(void);
i32 get_window_state(void);
const std::vector<std::pair<i32, i32>> * get_supported_render_resolutions(void);
std::pair<i32, i32> get_optimum_render_resolution(aspect_ratio ratio);
std::pair<i32, i32> get_optimum_render_resolution(i32 width, i32 height);
aspect_ratio get_aspect_ratio(i32 width, i32 height);
aspect_ratio get_window_aspect_ratio(void);
aspect_ratio get_monitor_aspect_ratio(void);

#endif 
