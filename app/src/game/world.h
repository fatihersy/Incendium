
#ifndef WORLD_H
#define WORLD_H

#include "game_types.h"

[[__nodiscard__]] bool world_system_initialize(const app_settings* _in_app_settings);
[[__nodiscard__]] bool world_system_begin(const camera_metrics* _in_camera_metrics);

void set_worldmap_location(i32 id);
const worldmap_stage* get_worldmap_locations(void);
const worldmap_stage* get_active_worldmap(void);
const tilemap* get_active_map(void);
tilemap ** get_active_map_ptr(void);
void set_map_tile(i32 layer, tile src, tile dst);
tilemap_prop_address get_map_prop_by_pos(Vector2 pos);
map_collision* get_map_collision_by_pos(Vector2 pos);
tilemap_prop_static* get_map_prop_static_by_id(i32 map_id);
tilemap_prop_sprite* get_map_prop_sprite_by_id(i32 map_id);
const map_collision* get_map_collision_by_id(i32 coll_id);

void save_current_map(void);
void load_current_map(void);

tile _get_tile_from_sheet_by_mouse_pos(Vector2 _mouse_pos);
tile _get_tile_from_map_by_mouse_pos(i32 from_layer, Vector2 _mouse_pos);
void _render_props_y_based(i32 start_y, i32 end_y);
void _sort_render_y_based_queue(void);
Rectangle wld_calc_mainmenu_prop_dest(const tilemap * const _tilemap, Rectangle dest, f32 scale);

bool add_prop_curr_map(tilemap_prop_static prop_static);
bool add_prop_curr_map(tilemap_prop_sprite prop_sprite);
bool add_map_coll_curr_map(Rectangle map_coll);
bool remove_prop_cur_map_by_id(i32 map_id, tilemap_prop_types type);
bool remove_map_collision_by_id(i32 coll_id);
void update_map(void);
void drag_tilesheet(Vector2 vec);
void _render_tile_on_pos(const tile* _tile, Vector2 pos,const tilesheet* sheet);
void render_map(void);
void render_map_palette(f32 zoom);
void refresh_render_queue(i32 id);

#define _remove_prop_cur_map_by_id(PROP) remove_prop_cur_map_by_id(PROP->map_id, PROP->prop_type)

#endif
