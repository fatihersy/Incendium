
#ifndef TILEMAP_H
#define TILEMAP_H

#include "game_types.h"

bool create_tilemap(const tilesheet_type _type,const Vector2 _position,const i32 _grid_size,const i32 _tile_size,tilemap* const out_tilemap);
void create_tilesheet(tilesheet_type _type, u16 _dest_tile_size, f32 _offset, tilesheet* const out_tilesheet);

void update_tilemap(tilemap* const _tilemap);
void render_tilemap(const tilemap* _tilemap, Rectangle camera_view);
void render_props_y_based_all(const tilemap* _tilemap, Rectangle camera_view, i32 start_y, i32 end_y);
void render_props_y_based_by_zindex(const tilemap* _tilemap, size_t index, Rectangle camera_view, i32 start_y, i32 end_y);
void render_tilesheet(const tilesheet* sheet, f32 zoom);
void render_tile(const tile_symbol* symbol,const Rectangle dest,const tilesheet* sheet);
void render_mainmenu(const tilemap* _tilemap, Rectangle camera_view, const app_settings * in_settings);

Vector2 get_tilesheet_dim(const tilesheet* sheet);
Rectangle calc_mainmenu_prop_dest(const tilemap * const _tilemap, Rectangle dest, f32 scale, const app_settings * in_settings);
tile get_tile_from_sheet_by_mouse_pos(const tilesheet* sheet, const Vector2 mouse_pos, const f32 zoom);
tile get_tile_from_map_by_mouse_pos(const tilemap* sheet, const Vector2 mouse_pos, const u16 layer);
bool save_map_data(tilemap* const map, tilemap_stringtify_package* const out_package);
bool load_map_data(tilemap* const map, tilemap_stringtify_package* const out_package);
bool load_or_create_map_data(tilemap* const map, tilemap_stringtify_package* const out_package);

#endif
