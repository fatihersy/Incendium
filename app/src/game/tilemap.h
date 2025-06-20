
#ifndef TILEMAP_H
#define TILEMAP_H

#include "game_types.h"

void create_tilemap(tilesheet_type _type, Vector2 _position, u16 _grid_size, u16 _tile_size, tilemap* out_tilemap);
void create_tilesheet(tilesheet_type _type, u16 _dest_tile_size, f32 _offset, tilesheet* out_tilesheet);

void update_tilemap(tilemap* _tilemap);
void render_tilemap(tilemap* _tilemap, Rectangle camera_view);
void render_tilesheet(tilesheet* sheet, f32 zoom);
void render_tile(tile_symbol* symbol, Rectangle dest, tilesheet* sheet);

Vector2 get_tilesheet_dim(tilesheet* sheet);
tile get_tile_from_sheet_by_mouse_pos(tilesheet* sheet, Vector2 mouse_pos, f32 zoom);
tile get_tile_from_map_by_mouse_pos(tilemap* sheet, Vector2 mouse_pos, u16 layer);
bool save_map_data(tilemap* map, tilemap_stringtify_package* out_package);
bool load_map_data(tilemap * map, tilemap_stringtify_package * out_package);
bool load_or_create_map_data(tilemap * map, tilemap_stringtify_package * out_package);

#endif
