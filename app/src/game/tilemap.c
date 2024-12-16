#include "tilemap.h"

#include "core/fmemory.h"

#include "defines.h"
#include "game/resource.h"
#include "raylib.h"


void FDrawGrid(tilemap* _tilemap);
void map_to_str(tilemap* map, tilemap_stringtify_package* out_package);
void str_to_map(tilemap* map, tilemap_stringtify_package* out_package);

void create_tilemap(tilesheet_type _type, Vector2 _position, u16 _grid_size, u16 _tile_size, Color _grid_color, tilemap* out_tilemap) {
  if (_grid_size * _grid_size > MAX_TILEMAP_TILESLOT && (_type >= TILESHEET_TYPE_MAX || _type <= 0)) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::create_tilemap()::grid_size out of bound");
    return;
  }

  tilesheet* sheet = get_tilesheet_by_enum(_type);

  out_tilemap->render_grid = false;
  out_tilemap->position = _position;
  out_tilemap->tile_size = _tile_size;
  out_tilemap->map_dim = _grid_size;
  out_tilemap->map_dim_total = _grid_size * _grid_size;
  out_tilemap->grid_color = _grid_color;

  out_tilemap->position = (Vector2){
    .x = out_tilemap->position.x - (out_tilemap->map_dim * out_tilemap->tile_size) / 2.f,
    .y = out_tilemap->position.y - (out_tilemap->map_dim * out_tilemap->tile_size) / 2.f,
  };

  for (u16 i = 0; i < out_tilemap->map_dim_total; ++i) {
    u16 x = i % out_tilemap->map_dim;  
    u16 y = i / out_tilemap->map_dim;  

    out_tilemap->tiles[x][y].tile_size = sheet->tile_size;
    out_tilemap->tiles[x][y].tex_type = sheet->tex_type;
    out_tilemap->tiles[x][y].sheet_type = _type;
    out_tilemap->tiles[x][y].tile_symbol = sheet->tile_symbols[0 / _tile_size][0 / _tile_size];

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

  for (u16 i = 0; i < _tilemap->map_dim_total; ++i) {

    u16 x = i % _tilemap->map_dim;  
    u16 y = i / _tilemap->map_dim;  
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
    u16 x        = i % sheet->tile_count_x;
    u16 y        = i / sheet->tile_count_x;
    u16 origin_x = x * sheet->tile_size;  
    u16 origin_y = y * sheet->tile_size;  
    f32 dest_x   = x * sheet->offset * sheet->dest_tile_size;  
    f32 dest_y   = y * sheet->offset * sheet->dest_tile_size; 
    i16 x_pos    = sheet->offset     + sheet->position.x + dest_x; 
    i16 y_pos    = sheet->offset     + sheet->position.y + dest_y; 
    
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

  for (int i = 0; i <= _tilemap->map_dim; i++) {
    // Draw vertical lines
    DrawLineV(
    (Vector2) {
      _tilemap->position.x + i * _tilemap->tile_size, 
      _tilemap->position.y }, 
    (Vector2) {
     _tilemap->position.x + i * _tilemap->tile_size, 
      _tilemap->position.y + _tilemap->map_dim * _tilemap->tile_size
    }, _tilemap->grid_color);

    // Draw horizontal lines
    DrawLineV(
    (Vector2) {
      _tilemap->position.x, 
      _tilemap->position.y + i * _tilemap->tile_size }, 
    (Vector2) {
      _tilemap->position.x + _tilemap->map_dim * _tilemap->tile_size, 
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

tilemap_tile get_tile_from_sheet_by_mouse_pos(tilesheet* sheet, Vector2 mouse_pos) {
  if (!sheet) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::get_tile_from_mouse_pos()::Provided sheet was null");
    return (tilemap_tile) { .is_initialized = false };
  }
  tilemap_tile tile = {0};

  i16 x_pos    = mouse_pos.x - sheet->offset          - sheet->position.x; 
  i16 y_pos    = mouse_pos.y - sheet->offset          - sheet->position.y; 
  i16 dest_x   = x_pos       / (sheet->dest_tile_size * sheet->offset);  
  i16 dest_y   = y_pos       / (sheet->dest_tile_size * sheet->offset);  

  if (dest_x < 0 || dest_x > sheet->tile_count_x || dest_y < 0 || dest_y > sheet->tile_count_y) {
    return (tilemap_tile) { .is_initialized = false };
  }

  tile.x           = dest_x  * sheet->tile_size;
  tile.y           = dest_y  * sheet->tile_size;
  tile.tile_size   = sheet->tile_size;
  tile.sheet_type  = sheet->sheet_type;
  tile.tex_type    = sheet->tex_type;
  tile.tile_symbol = get_tilesheet_by_enum(tile.sheet_type)->tile_symbols[dest_x][dest_y];

  tile.is_initialized = true;
  return tile;
}

tilemap_tile get_tile_from_map_by_mouse_pos(tilemap* map, Vector2 mouse_pos) {
  if (!map) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::get_tile_from_mouse_pos()::Provided sheet was null");
    return (tilemap_tile) { .is_initialized = false };
  }
  tilemap_tile tile = {0};

  tile.x = (mouse_pos.x - map->position.x) / map->tile_size;
  tile.y = (mouse_pos.y - map->position.y) / map->tile_size;

  if (tile.x < 0 || tile.x > map->map_dim || tile.y < 0 || tile.y > map->map_dim) {
    return (tilemap_tile) { .is_initialized = false };
  }

  tile.tile_size = map->tile_size;
  tile.sheet_type = map->tiles[tile.x][tile.y].sheet_type;
  tile.tex_type = map->tiles[tile.x][tile.y].tex_type;
  tile.tile_symbol = get_tilesheet_by_enum(tile.sheet_type)->tile_symbols[tile.x / tile.tile_size][tile.y/ tile.tile_size];

  tile.is_initialized = true;
  return tile;
}

void map_to_str(tilemap* map, tilemap_stringtify_package* out_package) {
  if (!map) {
    TraceLog(LOG_ERROR, "Recieved a NULL pointer");
    return;
  }
  out_package->size = sizeof(tile_symbol) * map->map_dim_total;

  for (u16 i=0; i < map->map_dim_total; ++i) {
    u16 map_x   = i % map->map_dim;
    u16 map_y   = i / map->map_dim;  
    tilesheet* sheet = get_tilesheet_by_enum(map->tiles[map_x][map_y].sheet_type);

    u16 origin_x = map->tiles[map_x][map_y].x;
    u16 origin_y = map->tiles[map_x][map_y].y;  
    u16 sheet_x = origin_x / sheet->tile_size;
    u16 sheet_y = origin_y / sheet->tile_size;

    tile_symbol symbol = sheet->tile_symbols[sheet_x][sheet_y];  

    copy_memory(out_package->str + (sizeof(tile_symbol) * i), symbol.c, sizeof(symbol));
  }

  out_package->is_success = true;
}

void str_to_map(tilemap* map, tilemap_stringtify_package* out_package) {
  if (!*out_package->str) {
    TraceLog(LOG_ERROR, "Recieved a NULL pointer");
    return;
  }

  for (u16 i=0; i < map->map_dim_total; ++i) {
    tile_symbol symbol = {0};

    u16 map_x   = i % map->map_dim;
    u16 map_y   = i / map->map_dim;  
    copy_memory(symbol.c, out_package->str + (sizeof(tile_symbol) * i), sizeof(symbol));

    map->tiles[map_x][map_y].x = symbol.c[0] - TILEMAP_TILE_START_SYMBOL;
    map->tiles[map_x][map_y].y = symbol.c[1] - TILEMAP_TILE_START_SYMBOL;
    map->tiles[map_x][map_y].sheet_type = symbol.c[2];

    tilesheet* sheet = get_tilesheet_by_enum(map->tiles[map_x][map_y].sheet_type);

    u16 origin_x = map->tiles[map_x][map_y].x;
    u16 origin_y = map->tiles[map_x][map_y].y;  
    u16 sheet_x = origin_x / sheet->tile_size;
    u16 sheet_y = origin_y / sheet->tile_size;
  }

  out_package->is_success = true;
}

bool save_map_data(tilemap* map, tilemap_stringtify_package* out_package) {
  map_to_str(map, out_package);
  if (out_package->is_success) {
    return SaveFileData(rs_path("map.txt"), out_package->str, out_package->size);
  }
  else {
    TraceLog(LOG_ERROR, "ERROR::tilemap::save_map_data()::Recieving package data returned with failure");
    return false;
  }
}

bool load_map_data(tilemap* map, tilemap_stringtify_package* out_package) {
  int size = (int)sizeof(out_package->str);
  const u8* _str = LoadFileData(rs_path("map.txt"), &size);
  if (!_str) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::load_map_data()::Reading data returned null");
  }
  copy_memory(out_package->str, _str, size);
  str_to_map(map, out_package);
  
  return out_package->is_success;
}

