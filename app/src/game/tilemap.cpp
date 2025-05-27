#include "tilemap.h"
#include "core/fmemory.h"

#include "tools/fstring.h"
#include "game/resource.h"

void map_to_str(tilemap* map, tilemap_stringtify_package* out_package);
void str_to_map(tilemap* map, tilemap_stringtify_package* out_package);

void create_tilemap(tilesheet_type _type, Vector2 _position, u16 _grid_size, u16 _tile_size, tilemap* out_tilemap) {
  if (_grid_size * _grid_size > MAX_TILEMAP_TILESLOT && (_type >= TILESHEET_TYPE_MAX || _type <= 0)) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::create_tilemap()::grid_size out of bound");
    return;
  }

  tilesheet* sheet = get_tilesheet_by_enum(_type);

  out_tilemap->position = _position;
  out_tilemap->tile_size = _tile_size;
  out_tilemap->map_dim = _grid_size;
  out_tilemap->map_dim_total = _grid_size * _grid_size;

  out_tilemap->position = Vector2 {
    out_tilemap->position.x - (out_tilemap->map_dim * out_tilemap->tile_size) / 2.f,
    out_tilemap->position.y - (out_tilemap->map_dim * out_tilemap->tile_size) / 2.f
  };

  for (int j=0; j<MAX_TILEMAP_LAYERS; ++j) {
    for (u16 i = 0; i < out_tilemap->map_dim_total; ++i) {
      u16 x = i % out_tilemap->map_dim;  
      u16 y = i / out_tilemap->map_dim;  
      switch (j) {
        case 0: { 
          out_tilemap->tiles[j][x][y] = sheet->tile_symbols[1][0];
          break; 
        }
        case 1: { 
          out_tilemap->tiles[j][x][y] = sheet->tile_symbols[0][0];
          break; 
        }
        case 2: { 
          out_tilemap->tiles[j][x][y] = sheet->tile_symbols[0][0];
          break; 
        }
        case 3: { 
          out_tilemap->tiles[j][x][y] = sheet->tile_symbols[0][0];
          break; 
        }
        default:{
          out_tilemap->tiles[j][x][y] = sheet->tile_symbols[0][0];
        }
        break;
      }
    }
  }
    
  out_tilemap->is_initialized = true;
  return;
}
void create_tilesheet(tilesheet_type _type, u16 _dest_tile_size, f32 _offset, tilesheet* out_tilesheet) {
  if (_type >= TILESHEET_TYPE_MAX || _type <= 0) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::create_tilesheet()::Sheet type out of bound");
    return;
  }
  *out_tilesheet = *get_tilesheet_by_enum(_type);;
  out_tilesheet->dest_tile_size = _dest_tile_size;
  out_tilesheet->offset = _offset;

  out_tilesheet->is_initialized = true;
}

void update_tilemap(void) {}

void render_tilemap(tilemap* _tilemap, Rectangle camera_view) {
  if (!_tilemap) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::render_tilemap()::Provided map was null");
    return;
  }
  tilesheet* sheet = get_tilesheet_by_enum(TILESHEET_TYPE_MAP);

  int start_x = (int)((camera_view.x - _tilemap->position.x) / _tilemap->tile_size);
  int start_y = (int)((camera_view.y - _tilemap->position.y) / _tilemap->tile_size);
  int end_x = (int)((camera_view.x + camera_view.width - _tilemap->position.x) / _tilemap->tile_size) + 1;
  int end_y = (int)((camera_view.y + camera_view.height - _tilemap->position.y) / _tilemap->tile_size) + 1;
  
  start_x = start_x < 0 ? 0 : (start_x >= _tilemap->map_dim ? _tilemap->map_dim - 1 : start_x);
  start_y = start_y < 0 ? 0 : (start_y >= _tilemap->map_dim ? _tilemap->map_dim - 1 : start_y);
  end_x = end_x < 0 ? 0 : (end_x > _tilemap->map_dim ? _tilemap->map_dim : end_x);
  end_y = end_y < 0 ? 0 : (end_y > _tilemap->map_dim ? _tilemap->map_dim : end_y);

  for (u16 y = start_y; y < end_y; ++y) {
    for (u16 x = start_x; x < end_x; ++x) {
      if (x >= MAX_TILEMAP_TILESLOT_X || y >= MAX_TILEMAP_TILESLOT_Y) {
        TraceLog(LOG_ERROR, "tilemap::render_tilemap()::Calculated tile's x or y out of bound");
        continue;
      }
      i16 x_pos = _tilemap->position.x + x * _tilemap->tile_size;
      i16 y_pos = _tilemap->position.y + y * _tilemap->tile_size;
      
      for (int i = 0; i < MAX_TILEMAP_LAYERS; ++i) {
        render_tile(&_tilemap->tiles[i][x][y], Rectangle { (f32) x_pos, (f32) y_pos, (f32) _tilemap->tile_size, (f32) _tilemap->tile_size}, sheet);
      }
    }
  }
  
  for (int i = 0; i < _tilemap->prop_count; ++i) {
    if (!_tilemap->props[i].is_initialized) continue;

    const tilemap_prop* prop = &_tilemap->props[i]; if (!prop) {
      TraceLog(LOG_ERROR, "tilemap::render_tilemap()::Prop:%d is not valid", i);
      continue;
    }
    Rectangle prop_rect = prop->dest; 
    if (!CheckCollisionRecs(camera_view, prop_rect)) { continue; }

    Texture2D* tex = get_texture_by_enum(prop->tex_id); if (!tex) {
      TraceLog(LOG_ERROR, "tilemap::render_tilemap()::Atlas texture:%d is not valid", prop->tex_id);
      continue;
    }
    Vector2 origin = VECTOR2(prop->source.width / 2.f, prop->source.height / 2.f);

    Rectangle _dest = Rectangle {
      .x = prop->dest.x,
      .y = prop->dest.y,
      .width = prop->dest.width * prop->scale,
      .height = prop->dest.height * prop->scale,
    };

    DrawTexturePro(*tex, prop->source, _dest, origin, prop->rotation, WHITE);
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
    f32 dest_x   = x * sheet->offset * sheet->dest_tile_size * zoom;  
    f32 dest_y   = y * sheet->offset * sheet->dest_tile_size * zoom; 
    i16 x_pos    = sheet->offset     + sheet->position.x + dest_x; 
    i16 y_pos    = sheet->offset     + sheet->position.y + dest_y; 
    tile_symbol _tile = { .c {
      static_cast<u8>(x + TILEMAP_TILE_START_SYMBOL),
      static_cast<u8>(y + TILEMAP_TILE_START_SYMBOL) 
    }};

    render_tile(&_tile, Rectangle { (f32) x_pos, (f32) y_pos, (f32) sheet->dest_tile_size * zoom, (f32) sheet->dest_tile_size * zoom}, sheet);
  }
}
/**
 * @brief Unsafe!
 * @param tile Needed for x, y
 */
 void render_tile(tile_symbol* symbol, Rectangle dest, tilesheet* sheet) {
  u16 x = (symbol->c[0] - TILEMAP_TILE_START_SYMBOL) * sheet->tile_size;
  u16 y = (symbol->c[1] - TILEMAP_TILE_START_SYMBOL) * sheet->tile_size;
  DrawTexturePro(*sheet->atlas_handle, Rectangle {(f32) x, (f32) y, (f32) sheet->tile_size, (f32) sheet->tile_size }, dest, Vector2{}, 0.f, WHITE);
}

Vector2 get_tilesheet_dim(tilesheet* sheet) {
  if (!sheet) {
    TraceLog(LOG_ERROR, "tilemap::get_tilesheet_dim()::Sheet is invalid");
    return Vector2{};
  }
  return Vector2 {
    (sheet->tile_count_x * sheet->offset) * sheet->dest_tile_size,
    (sheet->tile_count_y * sheet->offset) * sheet->dest_tile_size,
  };
}
tile get_tile_from_sheet_by_mouse_pos(tilesheet* sheet, Vector2 mouse_pos, f32 zoom) {
  if (!sheet) {
    TraceLog(LOG_ERROR, "tilemap::get_tile_from_mouse_pos()::Provided sheet was null");
    return tile { };
  }
  tile _tile = { };

  i16 x_pos    = (mouse_pos.x - sheet->offset - sheet->position.x) / zoom; 
  i16 y_pos    = (mouse_pos.y - sheet->offset - sheet->position.y) / zoom; 
  if (x_pos < 0 || y_pos < 0) {
    return tile { };
  }
  i16 dest_x   = x_pos       / (sheet->dest_tile_size * sheet->offset);  
  i16 dest_y   = y_pos       / (sheet->dest_tile_size * sheet->offset);  

  if (dest_x < 0 || dest_x > sheet->tile_count_x || dest_y < 0 || dest_y > sheet->tile_count_y) {
    return tile { };
  }
  _tile.position.x           = dest_x  * sheet->tile_size;
  _tile.position.y           = dest_y  * sheet->tile_size;
  _tile.symbol = sheet->tile_symbols[dest_x][dest_y];
  _tile.is_initialized = true;

  return _tile;
}
tile get_tile_from_map_by_mouse_pos(tilemap* map, Vector2 mouse_pos, u16 layer) {
  if (!map) {
    TraceLog(LOG_ERROR, "tilemap::get_tile_from_mouse_pos()::Map is not valid");
    return tile { };
  }
  tile _tile = {};
  _tile.position.x = (mouse_pos.x - map->position.x) / map->tile_size;
  _tile.position.y = (mouse_pos.y - map->position.y) / map->tile_size;

  if (_tile.position.x >= map->map_dim || _tile.position.y >= map->map_dim) { // NOTE: Assumes tilemap x and y are unsigned
    return tile { };
  }
  _tile.symbol = map->tiles[layer][_tile.position.x][_tile.position.y];
  _tile.is_initialized = true;

  return _tile;
}

void map_to_str(tilemap* map, tilemap_stringtify_package* out_package) {
  if (!map || !out_package) {
    TraceLog(LOG_ERROR, "Received a NULL pointer");
    return;
  }
  out_package->is_success = false;
  tilesheet* sheet = get_tilesheet_by_enum(TILESHEET_TYPE_MAP);
  if (!sheet) {
    TraceLog(LOG_ERROR, "tilemap::map_to_str()::Map sheet is not valid");
    return;
  }

  for (int i = 0; i < MAX_TILEMAP_LAYERS; ++i) {
    out_package->size_tilemap_str[i] = sizeof(tile_symbol) * MAX_TILEMAP_TILESLOT_X * MAX_TILEMAP_TILESLOT_Y;

    for (int j = 0; j < MAX_TILEMAP_TILESLOT_X; ++j) {
        copy_memory(out_package->str_tilemap[i] + (sizeof(tile_symbol) * j * MAX_TILEMAP_TILESLOT_Y), map->tiles[i][j], sizeof(tile_symbol) * MAX_TILEMAP_TILESLOT_Y);
    }
  }

  for (int i = 0; i < map->prop_count && i < MAX_TILEMAP_PROPS; ++i) {
    if (!map->props[i].is_initialized) continue;
    
    tilemap_prop* prop = &map->props[i];
    const char* symbol = TextFormat("%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d",
      prop->id, prop->tex_id, prop->rotation, prop->scale, prop->prop_type,
      (i32)prop->source.x, (i32)prop->source.y, (i32)prop->source.width, (i32)prop->source.height, 
      (i32)prop->dest.x, (i32)prop->dest.y, (i32)prop->dest.width, (i32)prop->dest.height
    );
    u32 symbol_len = TextLength(symbol);
    u32 copy_len = symbol_len < TILESHEET_PROP_SYMBOL_STR_LEN ? symbol_len : TILESHEET_PROP_SYMBOL_STR_LEN - 1;
    copy_memory(out_package->str_props[i], symbol, copy_len);
    out_package->str_props[i][copy_len] = '\0';
  }
  out_package->size_props_str = sizeof(out_package->str_props);
  out_package->is_success = true;
}
void str_to_map(tilemap* map, tilemap_stringtify_package* out_package) {
  if (!out_package || !map) {
    TraceLog(LOG_ERROR, "Received a NULL pointer");
    return;
  }
  out_package->is_success = false;
  map->prop_count = 0;
  tilesheet* sheet = get_tilesheet_by_enum(TILESHEET_TYPE_MAP);
  if (!sheet) {
    TraceLog(LOG_ERROR, "str_to_map: Failed to get tilesheet");
    return;
  }
  for (int i = 0; i < MAX_TILEMAP_LAYERS; ++i) {

    for (u16 j = 0; j < MAX_TILEMAP_TILESLOT_X; ++j) {
      copy_memory(map->tiles[i][j], out_package->str_tilemap[i] + (sizeof(tile_symbol) * j * MAX_TILEMAP_TILESLOT_X), sizeof(tile_symbol) * MAX_TILEMAP_TILESLOT_X);
    }
  }

  for (int i = 0; i < MAX_TILEMAP_PROPS; ++i) {
    u32 len = TextLength((const char*)out_package->str_props[i]);
    if (len <= 0 || len > TILESHEET_PROP_SYMBOL_STR_LEN) {
      TraceLog(LOG_INFO, "tilemap::str_to_map()::Loaded prop amount:%d", i);
      break;
    }
    if (map->prop_count >= MAX_TILEMAP_PROPS) {
      TraceLog(LOG_WARNING, "str_to_map: Maximum number of props reached (%d)", MAX_TILEMAP_PROPS);
      break;
    }
    tilemap_prop* prop = &map->props[map->prop_count];
    string_parse_result str_par = parse_string((const char*)out_package->str_props[i], ',', MAX_PARSED_TEXT_ARR_LEN, TextLength((const char*)out_package->str_props[i]));
    if (str_par.count < 10) {
      TraceLog(LOG_ERROR, "str_to_map: Failed to parse prop data, expected 10 values, got %d", str_par.count);
      continue;
    }
    prop->id = TextToInteger((const char*)&str_par.buffer[0]);
    prop->tex_id = static_cast<texture_id>(TextToInteger((const char*)&str_par.buffer[1]));
    prop->rotation = TextToFloat((const char*)&str_par.buffer[2]);
    prop->scale = TextToFloat((const char*)&str_par.buffer[3]);
    prop->prop_type = static_cast<tilemap_prop_types>(TextToInteger((const char*)&str_par.buffer[4]));
    prop->source = {
      TextToFloat((const char*)&str_par.buffer[5]), TextToFloat((const char*)&str_par.buffer[6]),
      TextToFloat((const char*)&str_par.buffer[7]), TextToFloat((const char*)&str_par.buffer[8]),
    };
    prop->dest =   {
      TextToFloat((const char*)&str_par.buffer[9]), TextToFloat((const char*)&str_par.buffer[10]),
      TextToFloat((const char*)&str_par.buffer[11]), TextToFloat((const char*)&str_par.buffer[12]),
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
    for (int i=0; i<MAX_TILEMAP_LAYERS; ++i) {
      if (!SaveFileData(map_layer_path((const char*)map->filename[i]), out_package->str_tilemap[i], out_package->size_tilemap_str[i])) {
        TraceLog(LOG_ERROR, "ERROR::tilemap::save_map_data()::Recieving package data returned with failure");
        return false;
      }
    }
    return SaveFileData(map_layer_path((const char*)map->propfile), out_package->str_props, out_package->size_props_str);
  }

  return false;
}
/**
 * @brief Loads from "%s%s", RESOURCE_PATH, "map.txt"
 * 
 * @param map out_map
 * @param out_package Needed because of the local variable array limit. Array must be defined at initialization. Also extracts map data and operation results.
 */
bool load_map_data(tilemap * map, tilemap_stringtify_package * out_package) {
  {
    for(int i=0; i<MAX_TILEMAP_LAYERS; ++i){
      int dataSize = sizeof(out_package->str_tilemap[i]);
      u8* _str_tile = LoadFileData(map_layer_path((const char*)map->filename[i]), &dataSize);
      if (dataSize <= 0 || dataSize == I32_MAX) {
        TraceLog(LOG_ERROR, "tilemap::load_map_data()::Reading data returned null");
      }
      copy_memory(&out_package->str_tilemap[i], _str_tile, sizeof(out_package->str_tilemap[i]));
      UnloadFileData(_str_tile);
    }
  }
  {
    int dataSize = sizeof(out_package->str_props);
    u8* _str_prop = LoadFileData(map_layer_path((const char*)map->propfile), &dataSize);
    if (dataSize <= 0 || dataSize == I32_MAX) {
      TraceLog(LOG_ERROR, "tilemap::load_map_data()::Reading data returned null");
    }
    copy_memory(out_package->str_props, _str_prop, sizeof(out_package->str_props));
    UnloadFileData(_str_prop);
  }
  
  str_to_map(map, out_package);
  
  return out_package->is_success;
}
bool load_or_create_map_data(tilemap * map, tilemap_stringtify_package * out_package) {
  map_to_str(map, out_package);
  {
    for(int i=0; i<MAX_TILEMAP_LAYERS; ++i){
      if (FileExists(map_layer_path((const char*)map->filename[i]))) {
        int dataSize = 1;
        u8* _str_tile = LoadFileData(map_layer_path((const char*)map->filename[i]), &dataSize);
        if (dataSize <= 0 || dataSize == I32_MAX) {
          TraceLog(LOG_ERROR, "tilemap::load_map_data()::Reading data returned null");
        }
        out_package->size_tilemap_str[i] = dataSize;
        copy_memory(out_package->str_tilemap[i], _str_tile, dataSize);
        UnloadFileData(_str_tile);
      }
      else {
        if (!out_package->is_success) {
          TraceLog(LOG_ERROR, "tilemap::load_or_create_map_data()::map cannot successfully converted to string to save as file");
        }
        if (!SaveFileData(map_layer_path((const char*)map->filename[i]), out_package->str_tilemap[i], out_package->size_tilemap_str[i])) {
          TraceLog(LOG_ERROR, "tilemap::save_map_data()::Recieving package data returned with failure");
        }
      }
    }
  }
  {
    if (FileExists(map_layer_path((const char*)map->propfile))) {
      int dataSize = sizeof(out_package->str_props);
      u8* _str_prop = LoadFileData(map_layer_path((const char*)map->propfile), &dataSize);
      if (dataSize <= 0 || dataSize == I32_MAX) {
        TraceLog(LOG_ERROR, "tilemap::load_map_data()::Reading data returned null");
      }
      copy_memory(out_package->str_props, _str_prop, sizeof(out_package->str_props));
      UnloadFileData(_str_prop);
    }
    else {
      if (out_package->is_success) {
        TraceLog(LOG_ERROR, "tilemap::load_or_create_map_data()::map cannot successfully converted to string to save as file");
      }
      if (!SaveFileData(map_layer_path((const char*)map->propfile), out_package->str_props, out_package->size_props_str)) {
        TraceLog(LOG_ERROR, "ERROR::tilemap::save_map_data()::Recieving package data returned with failure");
      }
    }
  }
  str_to_map(map, out_package);
  
  return out_package->is_success;
}

