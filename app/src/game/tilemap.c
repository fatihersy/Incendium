#include "tilemap.h"

#include "defines.h"
#include "game/resource.h"
#include "raylib.h"

void FDrawGrid(tilemap* _tilemap);

void create_tilemap(tilesheet_type _type, Vector2 _position, u16 _grid_size, u16 _tile_size, Color _grid_color, tilemap* out_tilemap) {
  if (_grid_size * _grid_size > MAX_TILEMAP_TILESLOT && (_type >= TILESHEET_TYPE_MAX || _type <= 0)) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::create_tilemap()::grid_size out of bound");
    return;
  }

  tilesheet* sheet = get_tilesheet_by_enum(_type);

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

    out_tilemap->tiles[x][y].origin_tilesize = sheet->tile_size;
    out_tilemap->tiles[x][y].sheet_tex = sheet->tex_type;
    out_tilemap->tiles[x][y].type = _type;
  }

  out_tilemap->is_initialized = true;
  return;
}

void update_tilemap() {}

void render_tilemap(tilemap* _tilemap) {
  if (!_tilemap) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::render_tilemap()::Provided map was null");
    return;
  }

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

void render_tilesheet(tilesheet_type sheet_type, Vector2 _position, u16 _dest_tile_size, f32 offset) {
  if (sheet_type > TILESHEET_TYPE_MAX || sheet_type <= 0) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::render_tilesheet()::Provided sheet was null");
    return;
  }

  tilesheet* sheet = get_tilesheet_by_enum(sheet_type);

  for (u16 i = 0; i < sheet->tile_count; ++i) {

    u16 origin_x = i % sheet->tile_count_x * sheet->tile_size;  
    u16 origin_y = i / sheet->tile_count_y * sheet->tile_size;  
    f32 dest_x = (i % sheet->tile_count_x) * offset * _dest_tile_size;  
    f32 dest_y = (i / sheet->tile_count_y) * offset *  _dest_tile_size; 
    i16 x_pos = offset + _position.x + dest_x; 
    i16 y_pos = offset + _position.y + dest_y; 
    
    DrawTexturePro(
    *sheet->tex, 
    (Rectangle) {
      origin_x, origin_y, 
      sheet->tile_size, sheet->tile_size
    },
    (Rectangle) {
      x_pos, y_pos, 
      _dest_tile_size, _dest_tile_size 
    },
    (Vector2){ 0, 0 },
    0.0f, 
    WHITE
    );
  }
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

Vector2 get_tilesheet_dim(tilesheet_type sheet, u16 dest_size, f32 offset) {
  tilesheet* ts = get_tilesheet_by_enum(sheet);

  return (Vector2) {
    (ts->tile_count_x * offset) * dest_size,
    (ts->tile_count_y * offset) * dest_size,
  };
}
