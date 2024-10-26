#include "resource.h"

resource_system_state* resource_system;

#define MAX_TEXTURE_SLOTS 50

 Texture2D* textures;

bool resource_system_initialized = false;

bool resource_system_initialize() {
    if (resource_system_initialized) return false;
    
    resource_system = (resource_system_state*)malloc(sizeof(resource_system_state));

    textures = (Texture2D*)malloc(MAX_TEXTURE_SLOTS * sizeof(Texture2D));

    resource_system->texture_amouth = -1;

    resource_system_initialized = true;

    return true;
}

bool update_resource_system() {

    return true;
}

Texture2D get_texture_by_id(unsigned int id) 
{
    for (size_t i = 0; i < resource_system->texture_amouth+1; i++)
    {
        if(textures[i].id == id) return textures[i];
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

    textures[++resource_system->texture_amouth] = tex;

    return textures[resource_system->texture_amouth].id;
}
