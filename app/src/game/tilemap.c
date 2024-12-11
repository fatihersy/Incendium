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

    out_tilemap->tiles[x][y].tile_size = sheet->tile_size;
    out_tilemap->tiles[x][y].tex_type = sheet->tex_type;
    out_tilemap->tiles[x][y].sheet_type = _type;
    out_tilemap->tiles[x][y].is_initialized = true;
  }

  out_tilemap->is_initialized = true;
  return;
}


void create_tilesheet(tilesheet_type _type, u16 _dest_tile_size, f32 _offset, tilesheet* out_tilesheet) {
  if (_type >= TILESHEET_TYPE_MAX || _type <= 0) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::create_tilesheet()::Sheet type out of bound");
    return;
  }

  tilesheet sheet = *get_tilesheet_by_enum(_type);

  *out_tilesheet = sheet;
  out_tilesheet->dest_tile_size = _dest_tile_size;
  out_tilesheet->offset = _offset;
  out_tilesheet->is_initialized = true;
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

    if (!_tilemap->tiles[x][y].is_initialized) continue;

    Texture2D tex = *get_texture_by_enum(_tilemap->tiles[x][y].tex_type);

    DrawTexturePro(
    tex, 
    (Rectangle) {
        _tilemap->tiles[x][y].x,                    _tilemap->tiles[x][y].y, 
    _tilemap->tiles[x][y].tile_size, _tilemap->tiles[x][y].tile_size
    },
      (Rectangle) {
        x_pos,                    y_pos, 
    _tilemap->tile_size, _tilemap->tile_size 
    },
    (Vector2){ 0, 0 },
    0.0f, 
    WHITE
    );
  }

  
  if(_tilemap->render_grid) FDrawGrid(_tilemap);
}

void render_tilesheet(tilesheet* sheet) {
  if (!sheet) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::render_tilesheet()::Provided sheet was null");
    return;
  }

  for (u16 i = 0; i < sheet->tile_count; ++i) {

    u16 origin_x = i % sheet->tile_count_x * sheet->tile_size;  
    u16 origin_y = i / sheet->tile_count_y * sheet->tile_size;  
    f32 dest_x = (i % sheet->tile_count_x) * sheet->offset * sheet->dest_tile_size;  
    f32 dest_y = (i / sheet->tile_count_y) * sheet->offset * sheet->dest_tile_size; 
    i16 x_pos = sheet->offset + sheet->position.x + dest_x; 
    i16 y_pos = sheet->offset + sheet->position.y + dest_y; 
    
    DrawTexturePro(
    *sheet->tex, 
    (Rectangle) {
      origin_x, origin_y, 
      sheet->tile_size, sheet->tile_size
    },
    (Rectangle) {
      x_pos, y_pos, 
      sheet->dest_tile_size, sheet->dest_tile_size 
    },
    (Vector2){ 0, 0 },
    0.0f, 
    WHITE
    );
  }
}


void FDrawGrid(tilemap* _tilemap) {
  if (!_tilemap) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::FDrawGrid()::Provided map was null");
    return;
  }

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


void render_tile(tilemap_tile origin, tilemap_tile dest) {

  tilesheet* ts = get_tilesheet_by_enum(origin.sheet_type);
  Texture2D* tx = get_texture_by_enum(origin.tex_type);

  DrawTexturePro(
  *tx, 
  (Rectangle) {
    origin.x, origin.y, 
    origin.tile_size, origin.tile_size
  },
  (Rectangle) {
    dest.x, dest.y, 
    dest.tile_size, dest.tile_size 
  },
  (Vector2){ 0, 0 },
  0.0f, 
  WHITE
  );
}

Vector2 get_tilesheet_dim(tilesheet* sheet) {
  return (Vector2) {
    (sheet->tile_count_x * sheet->offset) * sheet->dest_tile_size,
    (sheet->tile_count_y * sheet->offset) * sheet->dest_tile_size,
  };
}

tilemap_tile get_tile_from_mouse_pos(tilesheet* sheet, Vector2 mouse_pos) {
  if (!sheet) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::get_tile_from_mouse_pos()::Provided sheet was null");
    return (tilemap_tile) { .is_initialized = false };
  }
  tilemap_tile tile = {0};

  i16 relative_x = mouse_pos.x - sheet->position.x - sheet->offset;
  i16 relative_y = mouse_pos.y - sheet->position.y - sheet->offset;

  i16 x = relative_x / (sheet->offset * sheet->dest_tile_size);
  i16 y = relative_y / (sheet->offset * sheet->dest_tile_size);

  if (x < 0 && x > sheet->tile_count_x && y < 0 && y > sheet->tile_count_y) {
    TraceLog(LOG_WARNING, "WARNING::tilemap::get_tile_from_mouse_pos()::No tile found");
    return (tilemap_tile) { .is_initialized = false };
  }

  tile.x = x;
  tile.y = y;
  tile.tile_size = sheet->tile_size;
  tile.sheet_type = sheet->sheet_type;
  tile.tex_type = sheet->tex_type;
  tile.is_initialized = true;

  return tile;
}


