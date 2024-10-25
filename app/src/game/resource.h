
#ifndef RESOURCE_H
#define RESOURCE_H

#include "defines.h"

bool resource_system_initialize();

bool update_resource_system();

unsigned int load_texture(const char* path, bool resize, Vector2 new_size);
Texture2D get_texture_by_id(unsigned int id);

#endif
