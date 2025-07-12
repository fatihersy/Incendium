
#ifndef SPRITESHEET_H
#define SPRITESHEET_H

#include "game_types.h"

void set_sprite(spritesheet *const sheet, bool _play_looped, bool _play_once);
void update_sprite(spritesheet *const sheet);
void play_sprite_on_site(spritesheet *const sheet, Color _tint, const Rectangle dest);
void play_sprite_on_site_pro(spritesheet *const sheet, const Rectangle dest, const Vector2 origin, const f32 rotation, const Color _tint);
void draw_sprite_on_site(spritesheet *const sheet, Color _tint, Vector2 pos, Vector2 scale, u16 frame);
void draw_sprite_on_site_by_id(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, u16 frame);
void stop_sprite(spritesheet *const sheet, bool reset);
void reset_sprite(spritesheet *const sheet, bool _retrospective);

// Exposed functions from resource.h
const Texture2D* ss_get_texture_by_enum(texture_id _id);
const atlas_texture* ss_get_atlas_texture_by_enum(atlas_texture_id _id);
const spritesheet* ss_get_spritesheet_by_enum(spritesheet_id _id);
const char *ss_rs_path(const char *_path);

#endif
