#include "tilemap.h"

#include "defines.h"
#include "core/fmemory.h"

#include "raylib.h"
#include "resource.h"

static tilemap_system_state* tilemap_state;

bool tilemap_system_initialize() {
    tilemap_state = (tilemap_system_state*)allocate_memory_linear(sizeof(tilemap_system_state), true);

    tilemap_state->tilemap = get_texture_by_enum(MAP_TILESET_TEXTURE);

    for (int i = 0; i < 15 * 15; ++i) {
        Image img = LoadImageFromTexture(*tilemap_state->tilemap);
        ImageCrop(&img, (Rectangle) {15*(i % 15), 15*(u16)(i / 15.f), 16, 16});
        tilemap_state->tileset[i] = LoadTextureFromImage(img);
    }

    return true;
}

void update_tilemap() {}

void render_tilemap() {
    DrawRectangle(5, 5, 15*16, 15*16, WHITE);
    for (int i = 0; i < 15 * 15; ++i) {
        DrawTexture(tilemap_state->tileset[i], (i%15)*16, (i/15)*16, WHITE);
    }
}
