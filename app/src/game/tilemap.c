#include "tilemap.h"

#include "defines.h"
#include "core/fmemory.h"

#include "raylib.h"
#include "resource.h"

static tilemap_system_state* tilemap_state;

bool tilemap_system_initialize() {
    tilemap_state = (tilemap_system_state*)allocate_memory_linear(sizeof(tilemap_system_state), true);

    tilemap_state->tilemap = get_texture_by_enum(MAP_TILESET_TEXTURE);

    return true;
}

void update_tilemap() {}

void render_tilemap() {
    //DrawRectangle(5, 5, 15*16, 15*16, WHITE);
    for (int i = 0; i < 15 * 15; ++i) {
        DrawTexturePro(*tilemap_state->tilemap, 
        (Rectangle) {16*(i/15), 16*(i%15), 16, 16}, 
        (Rectangle) {5, 5, 16*3, 16*3},
        (Vector2) {0, 0},
        0,
        WHITE);
    }
}
