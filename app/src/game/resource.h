
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
spritesheet get_spritesheet_by_enum(spritesheet_type type);
void flip_texture_spritesheet(spritesheet_type _type, world_direction _w_direction);
u16 register_sprite(spritesheet_type _type, scene_type _scene, bool _play_once, bool center_sprite);
void play_sprite_on_player(u16 _id);
void play_sprite_on_site(u16 _id, Rectangle dest);
void update_sprite(u16 queue_index);
void queue_sprite_change_location(u16 queue_index, Rectangle dest);
void stop_sprite(i16 i, bool reset);
bool is_sprite_playing(u16 i);
void load_spritesheet(const char* _path, spritesheet_type _type, u8 _fps, u8 _frame_width, u8 _frame_height, u8 _total_row, u8 _total_col);

#endif
