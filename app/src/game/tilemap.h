
#ifndef TILEMAP_H
#define TILEMAP_H

#include <defines.h>

void create_tilemap(tilesheet_type _type, Vector2 _position, u16 _grid_size, u16 _tile_size, Color _grid_color, tilemap* out_tilemap);

void update_tilemap();
void render_tilemap(tilemap* _tilemap);
void render_tilesheet(tilesheet_type sheet, Vector2 position, u16 _dest_tile_size, f32 offset);

Vector2 get_tilesheet_dim(tilesheet_type sheet, u16 dest_size, f32 offset);

#endif