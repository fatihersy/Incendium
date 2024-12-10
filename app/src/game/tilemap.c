#include "tilemap.h"

#include "defines.h"
#include "resource.h"

void FDrawGrid(tilemap* _tilemap);

void create_tilemap(tilesheet_type _type, Vector2 _position, u16 _grid_size, u16 _tile_size, Color _grid_color, tilemap* out_tilemap) {
  if (_grid_size * _grid_size > MAX_TILEMAP_TILESLOT) {
    TraceLog(LOG_ERROR, "grid_size out of bound");
    return;
  }

  out_tilemap->render_grid = true;
  out_tilemap->position = _position;
  out_tilemap->tile_size = _tile_size;
  out_tilemap->grid_size = _grid_size;
  out_tilemap->grid_color = _grid_color;

  out_tilemap->position = (Vector2){
    .x = out_tilemap->position.x - (out_tilemap->grid_size * out_tilemap->tile_size) / 2.f,
    .y = out_tilemap->position.y - (out_tilemap->grid_size * out_tilemap->tile_size) / 2.f,
  };

  for (u16 i = 0; i < out_tilemap->grid_size * out_tilemap->grid_size; ++i) {
    u16 x = i % out_tilemap->grid_size;  
    u16 y = i / out_tilemap->grid_size;  

    out_tilemap->tiles[x][y].origin_tilesize = 16;
    out_tilemap->tiles[x][y].sheet_tex = MAP_TILESET_TEXTURE;
    out_tilemap->tiles[x][y].type = TILESHEET_TYPE_MAP;
  }

  out_tilemap->is_initialized = true;
  return;
}

void update_tilemap() {}

void render_tilemap(tilemap* _tilemap) {

  for (u16 i = 0; i < _tilemap->grid_size * _tilemap->grid_size; ++i) {

    u16 x = i % _tilemap->grid_size;  
    u16 y = i / _tilemap->grid_size;  
    i16 x_pos = _tilemap->position.x + x * _tilemap->tile_size; 
    i16 y_pos = _tilemap->position.y + y * _tilemap->tile_size; 
    
    Texture2D tex = *get_texture_by_enum(_tilemap->tiles[x][y].sheet_tex);


    DrawTexturePro(
    tex, 
    (Rectangle) {
        _tilemap->tiles[x][y].x,        _tilemap->tiles[x][y].y, 
    _tilemap->tiles[x][y].origin_tilesize, _tilemap->tiles[x][y].origin_tilesize
    },
    (Rectangle) {
        x_pos, y_pos, 
    _tilemap->tile_size, _tilemap->tile_size 
    },
    (Vector2){ 0, 0 },
    0.0f, 
    WHITE
    );
  }

  
  if(_tilemap->render_grid) FDrawGrid(_tilemap);
}


void FDrawGrid(tilemap* _tilemap) {

  for (int i = 0; i <= _tilemap->grid_size; i++) {
    // Draw vertical lines
    DrawLineV(
    (Vector2) {
      _tilemap->position.x + i * _tilemap->tile_size, 
      _tilemap->position.y }, 
    (Vector2) {
     _tilemap->position.x + i * _tilemap->tile_size, 
      _tilemap->position.y + _tilemap->grid_size * _tilemap->tile_size
    }, _tilemap->grid_color);

    // Draw horizontal lines
    DrawLineV(
    (Vector2) {
      _tilemap->position.x, 
      _tilemap->position.y + i * _tilemap->tile_size }, 
    (Vector2) {
      _tilemap->position.x + _tilemap->grid_size * _tilemap->tile_size, 
      _tilemap->position.y + i * _tilemap->tile_size
    }, _tilemap->grid_color);
  }
}

