
#ifndef RESOURCE_H
#define RESOURCE_H

#include "game_types.h"

bool resource_system_initialize(void);

const char* rs_path(const char *filename);
const char* map_layer_path(const char *filename);
const atlas_texture* get_atlas_texture_by_enum(atlas_texture_id _id);
const Texture2D* get_texture_by_enum(texture_id _id);
const Image* get_image_by_enum(image_type type);
const spritesheet* get_spritesheet_by_enum(spritesheet_id type);
const tilesheet* get_tilesheet_by_enum(tilesheet_type type);
file_data _get_asset_file_data(i32 index);

std::vector<tilemap_prop_static> * resource_get_tilemap_props_static(tilemap_prop_types type);
std::vector<tilemap_prop_sprite> * resource_get_tilemap_props_sprite(void);
tilemap_prop_address resource_get_map_prop_by_prop_id(i32 id, tilemap_prop_types type);

constexpr void add_prop(texture_id source_tex, tilemap_prop_types type, Rectangle source, f32 scale = 1.f, Color tint = WHITE);
constexpr void add_prop(tilemap_prop_types type, spritesheet_id sprite_id, f32 scale = 1.f, Color tint = WHITE);
#define add_prop_tree(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_TREE __VA_OPT__(,) __VA_ARGS__)
#define add_prop_tombstone(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_TOMBSTONE, __VA_ARGS__)
#define add_prop_stone(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_STONE __VA_OPT__(,) __VA_ARGS__)
#define add_prop_spike(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_SPIKE __VA_OPT__(,) __VA_ARGS__)
#define add_prop_skull(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_SKULL __VA_OPT__(,) __VA_ARGS__)
#define add_prop_pillar(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_PILLAR __VA_OPT__(,) __VA_ARGS__)
#define add_prop_lamp(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_LAMP __VA_OPT__(,) __VA_ARGS__)
#define add_prop_fence(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_FENCE __VA_OPT__(,) __VA_ARGS__)
#define add_prop_detail(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_DETAIL __VA_OPT__(,) __VA_ARGS__)
#define add_prop_candle(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_CANDLE __VA_OPT__(,) __VA_ARGS__)
#define add_prop_building(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_BUILDING __VA_OPT__(,) __VA_ARGS__)
#define add_prop_sprite(SPRITE_ID, ...) add_prop(TILEMAP_PROP_TYPE_SPRITE, SPRITE_ID __VA_OPT__(,) __VA_ARGS__)

#endif
