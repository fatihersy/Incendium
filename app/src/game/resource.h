
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


#endif
