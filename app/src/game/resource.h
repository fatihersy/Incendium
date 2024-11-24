
#ifndef RESOURCE_H
#define RESOURCE_H

#include "defines.h"

bool resource_system_initialize();

unsigned int load_texture(const char* _path, bool resize, Vector2 new_size, texture_type type);
void load_spritesheet(const char* _path, spritesheet_type _type, u8 _fps, u8 _frame_width, u8 _frame_height, u8 _total_row, u8 _total_col);
const char *rs_path(const char *_path);
Texture2D* get_texture_by_id(unsigned int id);
Texture2D* get_texture_by_enum(texture_type type);
spritesheet get_spritesheet_by_enum(spritesheet_type type);


#endif
