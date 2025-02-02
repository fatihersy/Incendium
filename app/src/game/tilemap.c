#include "tilemap.h"
#include "core/fmemory.h"

#include "tools/fstring.h"
#include "game/resource.h"

void map_to_str(tilemap* map, tilemap_stringtify_package* out_package);
void str_to_map(tilemap* map, tilemap_stringtify_package* out_package);

void create_tilemap(tilesheet_type _type, Vector2 _position, u16 _grid_size, u16 _tile_size, Color _grid_color, tilemap* out_tilemap) {
  if (_grid_size * _grid_size > MAX_TILEMAP_TILESLOT && (_type >= TILESHEET_TYPE_MAX || _type <= 0)) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::create_tilemap()::grid_size out of bound");
    return;
  }

  tilesheet* sheet = get_tilesheet_by_enum(_type);

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

    out_tilemap->tiles[x][y].tex_id = sheet->tex_id;
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
    if (x >= MAX_TILEMAP_TILESLOT_X || x < 0 || y >= MAX_TILEMAP_TILESLOT_Y || y < 0) {
      TraceLog(LOG_ERROR, "tilemap::render_tilemap()::Calculated tile's x or y out of bound");
      return;
    }
    if (!_tilemap->tiles[x][y].is_initialized) {
      TraceLog(LOG_ERROR, "tilemap::render_tilemap()::tile:{%d,%d} is not initialized or corrupted", x, y);
      return;
    }
    render_tile(_tilemap->tiles[x][y], (Rectangle) { x_pos, y_pos, _tilemap->tile_size, _tilemap->tile_size });
  }
  for (int i=0; i < _tilemap->prop_count; ++i) {
    if (!_tilemap->props[i].is_initialized) continue;
    
    tilemap_prop* prop = &_tilemap->props[i];
    Texture2D* tex = get_texture_by_enum(prop->atlas_id);
    DrawTexturePro(*tex, prop->source, prop->dest, (Vector2) {0}, 0.f, WHITE); 
  }
}

void render_tilesheet(tilesheet* sheet, f32 zoom) {
  if (!sheet) {
    TraceLog(LOG_ERROR, "tilemap::render_tilesheet()::Provided sheet was null");
    return;
  }

  for (u16 i = 0; i < sheet->tile_count; ++i) {
    u16 x        = i % sheet->tile_count_x;
    u16 y        = i / sheet->tile_count_x;
    u16 origin_x = x * sheet->tile_size;  
    u16 origin_y = y * sheet->tile_size;  
    f32 dest_x   = x * sheet->offset * sheet->dest_tile_size * zoom;  
    f32 dest_y   = y * sheet->offset * sheet->dest_tile_size * zoom; 
    i16 x_pos    = sheet->offset     + sheet->position.x + dest_x; 
    i16 y_pos    = sheet->offset     + sheet->position.y + dest_y; 
    
    render_tile(
    (tilemap_tile) {
      .tex_id = sheet->tex_id,
      .x = origin_x, .y = origin_y,
      .sheet_type = sheet->sheet_type
    }, 
    (Rectangle) {
      x_pos, y_pos, 
      sheet->dest_tile_size * zoom, sheet->dest_tile_size * zoom
    });
  }
}

/**
 * @brief Unsafe!
 * @param tile Needed for x, y, tex_id and sheet_type
 */
void render_tile(tilemap_tile tile, Rectangle dest) {
  if (tile.sheet_type >= TILESHEET_TYPE_MAX || tile.sheet_type <= 0) {
    TraceLog(LOG_ERROR, "tilemap::render_tile()::Sheet type out of bound");
    return;
  }
  tilesheet* ts = get_tilesheet_by_enum(tile.sheet_type); 
  Texture2D* tx = get_texture_by_enum(tile.tex_id);
  if (!ts || !tx) {
    TraceLog(LOG_WARNING, "tilemap::render_tile()::Recieved tilesheet or texture pointer was NULL");
    return;
  }

  DrawTexturePro(*tx, 
    (Rectangle) { tile.x, tile.y, ts->tile_size, ts->tile_size},
    dest,
    (Vector2){0},0.f, WHITE
  );
}

/**
 * @brief Unsafe!
 */
Vector2 get_tilesheet_dim(tilesheet* sheet) {
  return (Vector2) {
    (sheet->tile_count_x * sheet->offset) * sheet->dest_tile_size,
    (sheet->tile_count_y * sheet->offset) * sheet->dest_tile_size,
  };
}

tilemap_tile get_tile_from_sheet_by_mouse_pos(tilesheet* sheet, Vector2 mouse_pos, f32 zoom) {
  if (!sheet) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::get_tile_from_mouse_pos()::Provided sheet was null");
    return (tilemap_tile) { .is_initialized = false };
  }
  tilemap_tile tile = {0};

  i16 x_pos    = (mouse_pos.x - sheet->offset - sheet->position.x) / zoom; 
  i16 y_pos    = (mouse_pos.y - sheet->offset - sheet->position.y) / zoom; 
  i16 dest_x   = x_pos       / (sheet->dest_tile_size * sheet->offset);  
  i16 dest_y   = y_pos       / (sheet->dest_tile_size * sheet->offset);  

  if (dest_x < 0 || dest_x > sheet->tile_count_x || dest_y < 0 || dest_y > sheet->tile_count_y) {
    return (tilemap_tile) { .is_initialized = false };
  }

  tile.x           = dest_x  * sheet->tile_size;
  tile.y           = dest_y  * sheet->tile_size;
  tile.sheet_type  = sheet->sheet_type;
  tile.tex_id      = sheet->tex_id;
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

  tile.sheet_type = map->tiles[tile.x][tile.y].sheet_type;
  tile.tex_id = map->tiles[tile.x][tile.y].tex_id;
  tile.tile_symbol = get_tilesheet_by_enum(tile.sheet_type)->tile_symbols[tile.x / map->tile_size][tile.y/ map->tile_size];

  tile.is_initialized = true;
  return tile;
}

void map_to_str(tilemap* map, tilemap_stringtify_package* out_package) {
  if (!map) {
    TraceLog(LOG_ERROR, "Recieved a NULL pointer");
    return;
  }

  out_package->size_tilemap_str = sizeof(tile_symbol) * map->map_dim_total;

  for (u16 i=0; i < map->map_dim_total; ++i) {
    u16 map_x   = i % map->map_dim;
    u16 map_y   = i / map->map_dim;  
    tilesheet* sheet = get_tilesheet_by_enum(map->tiles[map_x][map_y].sheet_type);

    u16 origin_x = map->tiles[map_x][map_y].x;
    u16 origin_y = map->tiles[map_x][map_y].y;  
    u16 sheet_x = origin_x / sheet->tile_size;
    u16 sheet_y = origin_y / sheet->tile_size;

    copy_memory(
      out_package->str_tilemap + ((sizeof(sheet->tile_symbols[sheet_x][sheet_y].c) * i)), 
      sheet->tile_symbols[sheet_x][sheet_y].c, 
      sizeof(sheet->tile_symbols[sheet_x][sheet_y].c)
    );
  }

  for (int i=0; i<map->prop_count; ++i) {
    if (!map->props[i].is_initialized) break;
    tilemap_prop* prop = &map->props[i];
    const char* symbol = TextFormat("%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d",
      prop->id, prop->atlas_id, 
      (i32)prop->source.x, (i32)prop->source.y, (i32)prop->source.width, (i32)prop->source.height, 
      (i32)prop->dest.x, (i32)prop->dest.y, (i32)prop->dest.width, (i32)prop->dest.height
    );
    copy_memory(out_package->str_props[i], symbol, sizeof(char)*TILESHEET_PROP_SYMBOL_STR_LEN);
  }
  out_package->size_tilemap_str = sizeof(out_package->str_tilemap);
  out_package->size_props_str = sizeof(out_package->str_props);

  out_package->is_success = true;
}

void str_to_map(tilemap* map, tilemap_stringtify_package* out_package) {
  if (!out_package || !map) {
    TraceLog(LOG_ERROR, "Recieved a NULL pointer");
    return;
  }

  for (u16 i=0; i < map->map_dim_total; ++i) {
    tile_symbol symbol = {0};

    u16 map_x   = i % map->map_dim;
    u16 map_y   = i / map->map_dim;  
    copy_memory(symbol.c, out_package->str_tilemap + (sizeof(tile_symbol) * i), sizeof(symbol));

    tilesheet* sheet = get_tilesheet_by_enum(symbol.c[2]);

    map->tiles[map_x][map_y].x = (symbol.c[0] - TILEMAP_TILE_START_SYMBOL) * sheet->tile_size;
    map->tiles[map_x][map_y].y = (symbol.c[1] - TILEMAP_TILE_START_SYMBOL) * sheet->tile_size;
    map->tiles[map_x][map_y].sheet_type = sheet->sheet_type;
  }
  for (int i=0; i<MAX_TILEMAP_PROPS; ++i) 
  {
    u32 len = TextLength((const char*)out_package->str_props[i]);
    if (len <= 0 || len > TILESHEET_PROP_SYMBOL_STR_LEN) {
      TraceLog(LOG_INFO, "tilemap::str_to_map()::Loaded prop amount:%d", i);
      break;
    }
    tilemap_prop* prop = &map->props[map->prop_count];
    string_parse_result str_par = parse_string(
      (const char*)out_package->str_props[i], ',', 10, 
      TextLength((const char*)out_package->str_props[i])
    );

    prop->id = str_to_U16((const char*)&str_par.buffer[0]);
    prop->atlas_id = str_to_U16((const char*)&str_par.buffer[1]);
    prop->source = (Rectangle) {
      str_to_F32((const char*)&str_par.buffer[2]),
      str_to_F32((const char*)&str_par.buffer[3]),
      str_to_F32((const char*)&str_par.buffer[4]),
      str_to_F32((const char*)&str_par.buffer[5]),
    };
    prop->dest = (Rectangle) {
      str_to_F32((const char*)&str_par.buffer[6]),
      str_to_F32((const char*)&str_par.buffer[7]),
      str_to_F32((const char*)&str_par.buffer[8]),
      str_to_F32((const char*)&str_par.buffer[9]),
    };
    prop->is_initialized = true;
    map->prop_count++;
  }

  out_package->is_success = true;
}

/**
 * @brief Saves at "%s%s", RESOURCE_PATH, "map.txt"
 * 
 * @param map in_map
 * @param out_package Needed because of the local variable array limit. Array must be defined at initialization. Also extracts map data and operation results.
 */
bool save_map_data(tilemap* map, tilemap_stringtify_package* out_package) {
  map_to_str(map, out_package);
  if (out_package->is_success) {
    bool tilemap_save_result = SaveFileData(rs_path("tilemap.txt"), out_package->str_tilemap, out_package->size_tilemap_str);
    bool props_save_result   = SaveFileData(rs_path("map_props.txt"), out_package->str_props, out_package->size_props_str);

    return tilemap_save_result && props_save_result;
  }
  else {
    TraceLog(LOG_ERROR, "ERROR::tilemap::save_map_data()::Recieving package data returned with failure");
    return false;
  }
}

/**
 * @brief Loads from "%s%s", RESOURCE_PATH, "map.txt"
 * 
 * @param map out_map
 * @param out_package Needed because of the local variable array limit. Array must be defined at initialization. Also extracts map data and operation results.
 */
bool load_map_data(tilemap* map, tilemap_stringtify_package* out_package) {
  {
    u8* _str_tile = LoadFileData(rs_path("tilemap.txt"), (i32*)&out_package->size_tilemap_str);
    if (out_package->size_tilemap_str <= 0 || out_package->size_tilemap_str == U64_MAX) {
      TraceLog(LOG_ERROR, "ERROR::tilemap::load_map_data()::Reading data returned null");
    }
    copy_memory(out_package->str_tilemap, _str_tile, out_package->size_tilemap_str);
    UnloadFileData(_str_tile);
  }
  {
    u8* _str_prop = LoadFileData(rs_path("map_props.txt"), (i32*)&out_package->size_props_str);
    if (out_package->size_props_str <= 0 || out_package->size_props_str == U64_MAX) {
      TraceLog(LOG_ERROR, "ERROR::tilemap::load_map_data()::Reading data returned null");
    }
    copy_memory(out_package->str_props, _str_prop, out_package->size_props_str);
    UnloadFileData(_str_prop);
  }
  
  str_to_map(map, out_package);
  
  return out_package->is_success;
}

