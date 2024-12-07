#include "tilemap.h"

#include "defines.h"
#include "core/fmemory.h"

#include "raylib.h"
#include "resource.h"

static tilemap* state;

bool create_tilemap(Vector2 _position, u16 _grid_size, u16 _cell_size, Color _grid_color) {
    state = (tilemap*)allocate_memory_linear(sizeof(tilemap), true);
    
    state->tex = get_texture_by_enum(MAP_TILESET_TEXTURE);
    state->cell_size = _cell_size;
    state->grid_size = _grid_size;
    state->position = _position;
    state->grid_color = _grid_color;

    state->position = (Vector2){
        .x = state->position.x - (state->grid_size * state->cell_size) / 2.f,
        .y = state->position.y - (state->grid_size * state->cell_size) / 2.f,
    };

    return true;
}

void update_tilemap() {}

void render_tilemap() {
    FDrawGrid();

    for (int i = 0; i < state->grid_size; ++i) {
        int x = (i % 15) * state->cell_size; // X position in the grid
        int y = (i / 15) * state->cell_size; // Y position in the grid

        DrawTexturePro(
            *state->tex, 
            (Rectangle){16 * (i % 15), 16 * (i / 15), 16, 16}, // Source rectangle
            (Rectangle){x, y, state->cell_size, state->cell_size}, // Destination rectangle
            (Vector2){0, 0}, // Origin
            0.0f, // Rotation
            WHITE // Tint
        );
    }
}


void FDrawGrid() {

    for (int i = 0; i <= state->grid_size; i++) {
      // Draw vertical lines
      DrawLineV(
      (Vector2) {
        state->position.x + i * state->cell_size, 
        state->position.y }, 
      (Vector2) {
        state->position.x + i * state->cell_size, 
        state->position.y + state->grid_size * state->cell_size
      }, state->grid_color);

      // Draw horizontal lines
      DrawLineV(
      (Vector2) {
        state->position.x, 
        state->position.y + i * state->cell_size }, 
      (Vector2) {
        state->position.x + state->grid_size * state->cell_size, 
        state->position.y + i * state->cell_size
      }, state->grid_color);
    }

}
