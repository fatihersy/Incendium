
#ifndef SPRITESHEET_H
#define SPRITESHEET_H

#include "defines.h"

#if defined(PSPRITESHEET_SYSTEM) 
#define register_sprite(_type, _scene, _play_once, center_sprite) \
    _register_sprite(&PSPRITESHEET_SYSTEM->spritesheet_system, _type, _scene, _play_once, center_sprite)
#define update_sprite_renderqueue() \
    _update_sprite_renderqueue(&PSPRITESHEET_SYSTEM->spritesheet_system, PSPRITESHEET_SYSTEM->scene_data)
#define render_sprite_renderqueue() \
    _render_sprite_renderqueue(&PSPRITESHEET_SYSTEM->spritesheet_system)
#define play_sprite_on_site(_id, dest) \
    _play_sprite_on_site(&PSPRITESHEET_SYSTEM->spritesheet_system, _id, dest)
#define queue_sprite_change_location(queue_index, _location) \
    _queue_sprite_change_location(&PSPRITESHEET_SYSTEM->spritesheet_system, queue_index, _location)
#define stop_sprite(index, reset) \
    _stop_sprite(&PSPRITESHEET_SYSTEM->spritesheet_system, index, reset)
#define is_sprite_playing(index) \
    _is_sprite_playing(&PSPRITESHEET_SYSTEM->spritesheet_system, index)
#define get_texture_by_enum(_type) \
    _get_texture_by_enum(_type)
#define get_spritesheet_by_enum(_type) \
    _get_spritesheet_by_enum(_type)
#define rs_path(_path) \
    _rs_path(_path)
#endif

u16 _register_sprite(spritesheet_play_system *system, spritesheet_type _type, scene_type _scene, bool _play_once, bool center_sprite) ;
void _update_sprite_renderqueue(spritesheet_play_system *system, scene_type scene_data);
Texture2D* _get_texture_by_enum(texture_type _type);
spritesheet _get_spritesheet_by_enum(spritesheet_type _type);
const char *_rs_path(const char *_path);
void _render_sprite_renderqueue(spritesheet_play_system *system);
void _play_sprite_on_site(spritesheet_play_system *system, u16 _id, Rectangle dest);
void _queue_sprite_change_location(spritesheet_play_system *system, u16 queue_index, Rectangle _location);
void _stop_sprite(spritesheet_play_system *system, u16 index, bool reset);
bool _is_sprite_playing(spritesheet_play_system *system, u16 index);

#endif