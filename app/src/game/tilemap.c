#include "tilemap.h"

#include "defines.h"
#include "core/fmemory.h"

#include "raylib.h"
#include "resource.h"

static tilemap* state;

bool create_tilemap(u16 _origin_tilesize, Vector2 _position, u16 _grid_size, u16 _cell_size, Color _grid_color) {
  if (_grid_size * _grid_size > MAX_TILEMAP_TILESLOT) {
    TraceLog(LOG_ERROR, "grid_size out of bound");
    return false;
  }
  
  state = (tilemap*)allocate_memory_linear(sizeof(tilemap), true);
    
  state->tex = get_texture_by_enum(MAP_TILESET_TEXTURE);
  state->origin_tilesize = _origin_tilesize;
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

  for (u16 i = 0; i < MAX_TILEMAP_TILESLOT; ++i) {

    u16 x = i % MAX_TILEMAP_TILESLOT_X;  
    u16 y = i / MAX_TILEMAP_TILESLOT_X;  
    i16 x_pos = state->position.x + x * state->cell_size; 
    i16 y_pos = state->position.y + y * state->cell_size; 

    DrawTexturePro(
    *state->tex, 
    (Rectangle) {
        state->tiles[x][y].x,        state->tiles[x][y].y, 
    state->origin_tilesize, state->origin_tilesize 
    },
    (Rectangle) {
        x_pos, y_pos, 
    state->cell_size, state->cell_size 
    },
    (Vector2){ 0, 0 },
    0.0f, 
    WHITE
    );
  }

  DrawPixel(0, 0, RED);
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
