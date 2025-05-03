
#ifndef SPRITESHEET_H
#define SPRITESHEET_H

#include "game_types.h"
#include "raylib.h"

void set_sprite(spritesheet *sheet, bool _play_looped, bool _play_once, bool _center_sprite);
void update_sprite(spritesheet *sheet);
void play_sprite_on_site(spritesheet *sheet, Color _tint, Rectangle dest);
void draw_sprite_on_site(spritesheet *sheet, Color _tint, Vector2 pos, Vector2 scale, u16 frame, bool _should_center);
void draw_sprite_on_site_by_id(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, u16 frame, bool _should_center);
void stop_sprite(spritesheet *sheet, bool reset);
void reset_sprite(spritesheet *sheet, bool _retrospective);

// Exposed functions from resource.h
atlas_texture* _get_atlas_texture_by_enum(atlas_texture_id _id);
Texture2D* _get_texture_by_enum(texture_id _id);
spritesheet* _get_spritesheet_by_enum(spritesheet_id _id);
const char *_rs_path(const char *_path);

#endif
