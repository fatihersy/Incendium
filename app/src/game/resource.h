
#ifndef RESOURCE_H
#define RESOURCE_H

#include "defines.h"
#include "raylib.h"

bool resource_system_initialize();

void update_resource_system();
void render_resource_system();

unsigned int load_texture(const char* _path, bool resize, Vector2 new_size, texture_type type);
Texture2D get_texture_by_id(unsigned int id);
Texture2D get_texture_by_enum(texture_type type);
void play_sprite(spritesheet_type _type, spritesheet_playmod _mod, bool _play_once, Vector2 _coord, u16 id);
void stop_sprite(spritesheet_type _type);
void load_spritesheet(const char* _path, spritesheet_type _type, u8 _fps, u8 _frame_width, u8 _frame_height, u8 _total_row, u8 _total_col);

#endif
