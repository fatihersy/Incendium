
#ifndef TILEMAP_H
#define TILEMAP_H

#include "raylib.h"
#include <defines.h>

void create_tilemap(tilesheet_type _type, Vector2 _position, u16 _grid_size, u16 _tile_size, Color _grid_color, tilemap* out_tilemap);
void create_tilesheet(tilesheet_type _type, u16 _dest_tile_size, f32 _offset, tilesheet* out_tilesheet);

void update_tilemap();
void render_tilemap(tilemap* _tilemap);
void render_tilesheet(tilesheet* sheet);
void render_tile(tilemap_tile origin, tilemap_tile dest);

Vector2 get_tilesheet_dim(tilesheet* sheet);
tilemap_tile get_tile_from_sheet_by_mouse_pos(tilesheet* sheet, Vector2 mouse_pos);
tilemap_tile get_tile_from_map_by_mouse_pos(tilemap* sheet, Vector2 mouse_pos);

#endif