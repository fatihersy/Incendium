
#ifndef WORLD_H
#define WORLD_H

#include "game_types.h"

bool world_system_initialize(camera_metrics* _in_camera_metrics);

void set_worldmap_location(u16 id);
worldmap_stage* get_active_worldmap(void);
void set_map_tile(i32 layer, tile src, tile dst);
worldmap_stage* get_worldmap_locations(void);
tilemap* get_active_map(void);
tilemap_prop get_map_prop_by_pos(Vector2 pos);
tilemap_prop_static* get_map_prop_static_by_id(u16 id);
tilemap_prop_sprite* get_map_prop_sprite_by_id(u16 id);

void save_current_map(void);
void load_current_map(void);

tile _get_tile_from_sheet_by_mouse_pos(Vector2 _mouse_pos);
tile _get_tile_from_map_by_mouse_pos(u16 from_layer, Vector2 _mouse_pos);

bool add_prop_curr_map(tilemap_prop prop);
bool remove_prop_cur_map_by_id(u16 id, tilemap_prop_types type);
void update_map(void);
void drag_tilesheet(Vector2 vec);
void _render_tile_on_pos(tile* _tile, Vector2 pos, tilesheet* sheet);
void render_map(void);
void render_map_view_on(Vector2 pos, f32 zoom);
void render_map_palette(f32 zoom);

#define _remove_prop_cur_map_by_id(PROP) remove_prop_cur_map_by_id(PROP->id, PROP->prop_type)

#endif
