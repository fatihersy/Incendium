
#ifndef RESOURCE_H
#define RESOURCE_H

#include "defines.h"

typedef struct resource_system_state 
{
    i16 texture_amouth;

} resource_system_state;

bool resource_system_initialize();

bool update_resource_system();

unsigned int load_texture(const char* path, bool resize, Vector2 new_size);
Texture2D get_texture_by_id(unsigned int id);

#endif
