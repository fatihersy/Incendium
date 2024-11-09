#include "resource.h"

#include "core/fmemory.h"

static resource_system_state* resource_system;

bool resource_system_initialized = false;

bool resource_system_initialize() {
    if (resource_system_initialized) return false;

    resource_system = (resource_system_state*)allocate_memory_linear(sizeof(resource_system_state), true);

    resource_system->texture_amouth = -1;

    load_texture("D:\\Workspace\\Resources\\fudesumi.png", true, (Vector2){64, 64}, ENEMY_TEXTURE);
    load_texture("D:\\Workspace\\Resources\\space_bg.png", true, (Vector2){3840 , 2160}, BACKGROUND);
    load_texture("D:\\Workspace\\Resources\\wabbit_alpha.png", true, (Vector2){32, 32}, PLAYER_TEXTURE);

    resource_system_initialized = true;

    return true;
}

bool update_resource_system() {
    return true;
}

Texture2D get_texture_by_id(unsigned int id) {
    for (i16 i = 0; i < resource_system->texture_amouth + 1; i++) {
        if (resource_system->textures[i].id == id) return resource_system->textures[i];
    }

    return (Texture2D){.id = INVALID_ID32};
}

Texture2D get_texture_by_enum(resource_type type) {

    switch (type) {
        case UNSPECIFIED: {
            return (Texture2D) { .id = INVALID_ID32 };
        }
        case PLAYER_TEXTURE: {
            return resource_system->textures[PLAYER_TEXTURE];
        }
        case ENEMY_TEXTURE: {
            return resource_system->textures[ENEMY_TEXTURE];
        }
        case BACKGROUND: {
            return resource_system->textures[BACKGROUND];
        }

        default: break;
    }

    return (Texture2D) { .id = INVALID_ID32 };
}

unsigned int load_texture(const char* path, bool resize, Vector2 new_size, resource_type type) {

    if (!FileExists(path)) return INVALID_ID32;

    Texture2D tex = LoadTexture(path);

    if (resize) {
        Image img = LoadImage(path);
        ImageResize(&img, new_size.x, new_size.y);
        tex = LoadTextureFromImage(img);
    } else {
        tex = LoadTexture(path);
    }

    switch (type) {
        case UNSPECIFIED: {
            resource_system->textures[++resource_system->texture_amouth] = tex;

            return resource_system->textures[resource_system->texture_amouth].id;
        }
        case PLAYER_TEXTURE: {
            resource_system->textures[PLAYER_TEXTURE] = tex;

            return resource_system->textures[PLAYER_TEXTURE].id;
        }
        case ENEMY_TEXTURE: {
            resource_system->textures[ENEMY_TEXTURE] = tex;

            return resource_system->textures[ENEMY_TEXTURE].id;
        }
        case BACKGROUND: {
            resource_system->textures[BACKGROUND] = tex;

            return resource_system->textures[BACKGROUND].id;
        }

        default: break;
    }

    return 0;
}
