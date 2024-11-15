
#ifndef RESOURCE_H
#define RESOURCE_H

#include "defines.h"

bool resource_system_initialize();

void update_resource_system();
void render_resource_system();

unsigned int load_texture(const char* _path, bool resize, Vector2 new_size, texture_type type);
Texture2D get_texture_by_id(unsigned int id);
Texture2D get_texture_by_enum(texture_type type);
spritesheet get_spritesheet_by_enum(spritesheet_type type);
void flip_texture_spritesheet(spritesheet_type _type, world_direction _w_direction);
void play_sprite(spritesheet_type _type, spritesheet_playmod _mod, bool _play_once, Rectangle dest, bool center_sprite, u16 id);
void stop_sprite(spritesheet_type _type, bool should_reset);
void load_spritesheet(const char* _path, spritesheet_type _type, u8 _fps, u8 _frame_width, u8 _frame_height, u8 _total_row, u8 _total_col);

#endif
