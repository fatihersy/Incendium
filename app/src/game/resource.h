
#ifndef RESOURCE_H
#define RESOURCE_H

#include "defines.h"

bool resource_system_initialize();

const char *rs_path(const char *_path);
Texture2D* get_texture_by_id(unsigned int id);
Texture2D* get_texture_by_enum(texture_type type);
Image* get_image_by_enum(image_type type);
spritesheet get_spritesheet_by_enum(spritesheet_type type);
tilesheet* get_tilesheet_by_enum(tilesheet_type type);


#endif
