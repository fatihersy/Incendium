#include "resource.h"

#include "core/fmemory.h"

static resource_system_state* resource_system;


bool resource_system_initialized = false;

bool resource_system_initialize() {
    if (resource_system_initialized) return false;
    
    resource_system = (resource_system_state*)allocate_memory_linear(sizeof(resource_system_state), true);

    resource_system->texture_amouth = -1;

    resource_system_initialized = true;

    return true;
}

bool update_resource_system() {

    return true;
}

Texture2D get_texture_by_id(unsigned int id) 
{
    for (i16 i = 0; i < resource_system->texture_amouth+1; i++)
    {
        if(resource_system->textures[i].id == id) return resource_system->textures[i];
    }
    
    return (Texture2D) { .id = INVALID_ID32 };
}

unsigned int load_texture(const char* path, bool resize, Vector2 new_size) 
{
    if(!FileExists(path)) return INVALID_ID32;

    Texture2D tex = LoadTexture(path);

    if (resize) 
    {
        Image img = LoadImage(path);
        ImageResize(&img, new_size.x, new_size.y);
        tex = LoadTextureFromImage(img);
    } 
    else 
    {
        tex = LoadTexture(path);
    }

    resource_system->textures[++resource_system->texture_amouth] = tex;

    return resource_system->textures[resource_system->texture_amouth].id;
}
