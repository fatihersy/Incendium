#include "tilemap.h"
#include "core/fmemory.h"

#include "tools/fstring.h"
#include "game/resource.h"
#include "game/spritesheet.h"

#define PROP_PARSE_PROP_BUFFER_PARSE_SYMBOL_STR "_"
#define PROP_PARSE_PROP_BUFFER_PARSE_SYMBOL_C '_'
#define PROP_PARSE_PROP_MEMBER_PARSE_SYMBOL ','
#define PROP_BUFFER_PARSE_TOP_LIMIT 999
#define PROP_MEMBER_PARSE_TOP_LIMIT 99

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

void update_tilemap(tilemap* _tilemap) {
  if (_tilemap) {
    for (size_t iter = 0; iter < _tilemap->sprite_props.size(); ++iter) {
      update_sprite(&_tilemap->sprite_props.at(iter).sprite);
    }
  }
}

void render_tilemap(tilemap* _tilemap, Rectangle camera_view) {
  if (!_tilemap) {
    TraceLog(LOG_ERROR, "tilemap::render_tilemap()::Provided map was null");
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
  
  for (size_t iter = 0; iter < _tilemap->static_props.size(); ++iter) {
    if (!_tilemap->static_props.at(iter).is_initialized) continue;

    const tilemap_prop_static* prop = &_tilemap->static_props.at(iter); if (!prop) {
      TraceLog(LOG_ERROR, "tilemap::render_tilemap()::Prop:%d is not valid", iter);
      continue;
    }
    Rectangle prop_rect = prop->dest; 
    if (!CheckCollisionRecs(camera_view, prop_rect)) { continue; }

    Texture2D* tex = get_texture_by_enum(prop->tex_id); if (!tex) {
      TraceLog(LOG_ERROR, "tilemap::render_tilemap()::Atlas texture:%d is not valid", prop->tex_id);
      continue;
    }
    prop_rect.width = prop->dest.width * prop->scale;
    prop_rect.height = prop->dest.height * prop->scale;
  
    Vector2 origin = VECTOR2(prop_rect.width / 2.f, prop_rect.height / 2.f);

    DrawTexturePro(*tex, prop->source, prop_rect, origin, prop->rotation, WHITE);
  }
  
  for (size_t iter = 0; iter < _tilemap->sprite_props.size(); ++iter) {
    if (!_tilemap->sprite_props.at(iter).is_initialized) continue;

    tilemap_prop_sprite* prop = &_tilemap->sprite_props.at(iter); if (!prop) {
      TraceLog(LOG_ERROR, "tilemap::render_tilemap()::Prop:%d is not valid", iter);
      continue;
    }
    if (!CheckCollisionRecs(camera_view, prop->sprite.coord)) { continue; }

    play_sprite_on_site(&prop->sprite, WHITE, prop->sprite.coord);
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

  for (size_t iter = 0; iter < map->static_props.size(); ++iter) {
    tilemap_prop_static& _prop = map->static_props.at(iter);
    if (!_prop.is_initialized) continue;
    
    tilemap_prop_static* prop = &map->static_props.at(iter);
    i32 _prop_scale = prop->scale * 100.f;
    i32 _prop_rotation = prop->rotation * 10.f;
    const char* symbol = TextFormat("%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d," PROP_PARSE_PROP_BUFFER_PARSE_SYMBOL_STR,
      prop->id,            prop->tex_id,       prop->prop_type,   _prop_rotation, _prop_scale, prop->zindex,
      (i32)prop->source.x, (i32)prop->source.y,(i32)prop->source.width,(i32)prop->source.height, 
      (i32)prop->dest.x,   (i32)prop->dest.y,  (i32)prop->dest.width,  (i32)prop->dest.height
    );
    out_package->str_props.append(symbol);
  }
  for (size_t iter = 0; iter < map->sprite_props.size(); ++iter) {
    if (!map->sprite_props.at(iter).is_initialized) continue;
    
    tilemap_prop_sprite* prop = &map->sprite_props.at(iter);
    i32 _prop_scale = prop->scale * 100.f;
    i32 _prop_rotation = prop->sprite.rotation * 10.f;
    const char* symbol = TextFormat("%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d," PROP_PARSE_PROP_BUFFER_PARSE_SYMBOL_STR,
      prop->id, prop->sprite.sheet_id, prop->prop_type, _prop_rotation, _prop_scale, prop->zindex,
      0, 0, 0, 0, (i32)prop->sprite.coord.x,   (i32)prop->sprite.coord.y,  (i32)prop->sprite.coord.width,  (i32)prop->sprite.coord.height
    );
    out_package->str_props.append(symbol);
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
  map->static_prop_count = 0;
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

  string_parse_result str_par_buffer = parse_string(out_package->str_props.c_str(), PROP_PARSE_PROP_BUFFER_PARSE_SYMBOL_C, PROP_BUFFER_PARSE_TOP_LIMIT);
  for (size_t iter = 0; iter < str_par_buffer.buffer.size(); ++iter) {
    string_parse_result str_par_prop_member = parse_string(str_par_buffer.buffer.at(iter), PROP_PARSE_PROP_MEMBER_PARSE_SYMBOL, PROP_MEMBER_PARSE_TOP_LIMIT);
    if (str_par_prop_member.buffer.size() < 10u) {
      TraceLog(LOG_ERROR, "str_to_map: Failed to parse prop data, expected 10 values, got %zu", str_par_prop_member.buffer.size());
      continue;
    }
    tilemap_prop_types type = static_cast<tilemap_prop_types>(TextToInteger(str_par_prop_member.buffer.at(2).c_str()));
    if (type == TILEMAP_PROP_TYPE_SPRITE ) {
      tilemap_prop_sprite prop = {};
      prop.id = TextToInteger(str_par_prop_member.buffer.at(0).c_str());
      prop.sprite.sheet_id = static_cast<spritesheet_id>(TextToInteger(str_par_prop_member.buffer.at(1).c_str()));
      set_sprite(&prop.sprite, true, false, true);
      prop.prop_type = type;
      prop.sprite.rotation  = TextToFloat(str_par_prop_member.buffer.at(3).c_str());
      prop.scale            = TextToFloat(str_par_prop_member.buffer.at(4).c_str());
      prop.zindex         = TextToInteger(str_par_prop_member.buffer.at(5).c_str());
      prop.sprite.coord =   {
        TextToFloat(str_par_prop_member.buffer.at(10).c_str()), TextToFloat(str_par_prop_member.buffer.at(11).c_str()),
        TextToFloat(str_par_prop_member.buffer.at(12).c_str()), TextToFloat(str_par_prop_member.buffer.at(13).c_str()),
      };
      prop.scale = prop.scale / 100.f;
      prop.sprite.rotation = prop.sprite.rotation / 10.f;

      prop.sprite.coord.width  = prop.sprite.coord.width  * prop.scale;
      prop.sprite.coord.height = prop.sprite.coord.height * prop.scale;
      Vector2 origin = VECTOR2(prop.sprite.coord.width / 2.f, prop.sprite.coord.height / 2.f);
      prop.sprite.origin = origin;

      prop.is_initialized = true;
      map->sprite_props.push_back(prop);
      map->sprite_prop_count++;
    }
    else {
      tilemap_prop_static prop = {};
      prop.id     = TextToInteger(str_par_prop_member.buffer.at(0).c_str());
      prop.tex_id = static_cast<texture_id>(TextToInteger(str_par_prop_member.buffer.at(1).c_str()));
      prop.prop_type = type;
      prop.rotation = TextToFloat(str_par_prop_member.buffer.at(3).c_str());
      prop.scale    = TextToFloat(str_par_prop_member.buffer.at(4).c_str());
      prop.zindex = TextToInteger(str_par_prop_member.buffer.at(5).c_str());
      prop.source = {
        TextToFloat(str_par_prop_member.buffer.at(6).c_str()), TextToFloat(str_par_prop_member.buffer.at(7).c_str()),
        TextToFloat(str_par_prop_member.buffer.at(8).c_str()), TextToFloat(str_par_prop_member.buffer.at(9).c_str()),
      };
      prop.dest =   {
        TextToFloat(str_par_prop_member.buffer.at(10).c_str()), TextToFloat(str_par_prop_member.buffer.at(11).c_str()),
        TextToFloat(str_par_prop_member.buffer.at(12).c_str()), TextToFloat(str_par_prop_member.buffer.at(13).c_str()),
      };

      prop.scale = prop.scale / 100.f;
      prop.rotation = prop.rotation / 10.f;

      prop.is_initialized = true;
      map->static_props.push_back(prop);
      map->static_prop_count++;
    }
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
      if (!SaveFileData(map_layer_path(map->filename.at(i).c_str()), out_package->str_tilemap[i], out_package->size_tilemap_str[i])) {
        TraceLog(LOG_ERROR, "tilemap::save_map_data()::Recieving package data returned with failure");
        return false;
      }
    }
    return SaveFileData(map_layer_path(map->propfile.c_str()), out_package->str_props.data(), out_package->str_props.size());
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
      u8* _str_tile = LoadFileData(map_layer_path(map->filename.at(i).c_str()), &dataSize);
      if (dataSize <= 0 || dataSize == I32_MAX) {
        TraceLog(LOG_ERROR, "tilemap::load_map_data()::Reading data returned null");
      }
      copy_memory(&out_package->str_tilemap[i], _str_tile, dataSize);
      UnloadFileData(_str_tile);
    }
  }
  {
    int dataSize = sizeof(out_package->str_props);
    u8* _str_prop = LoadFileData(map_layer_path(map->propfile.c_str()), &dataSize);
    if (dataSize <= 0 || dataSize == I32_MAX) {
      TraceLog(LOG_ERROR, "tilemap::load_map_data()::Reading data returned null");
    }
    out_package->str_props.assign((char*)_str_prop, dataSize);
    UnloadFileData(_str_prop);
  }
  
  str_to_map(map, out_package);
  
  return out_package->is_success;
}
bool load_or_create_map_data(tilemap * map, tilemap_stringtify_package * out_package) {
  map_to_str(map, out_package);
  {
    for(int i=0; i<MAX_TILEMAP_LAYERS; ++i){
      if (FileExists(map_layer_path(map->filename.at(i).c_str()))) {
        int dataSize = 1;
        u8* _str_tile = LoadFileData(map_layer_path(map->filename.at(i).c_str()), &dataSize);
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
        if (!SaveFileData(map_layer_path(map->filename.at(i).c_str()), out_package->str_tilemap[i], out_package->size_tilemap_str[i])) {
          TraceLog(LOG_ERROR, "tilemap::save_map_data()::Recieving package data returned with failure");
        }
      }
    }
  }
  {
    if (FileExists(map_layer_path(map->propfile.c_str()))) {
      int dataSize = sizeof(out_package->str_props);
      u8* _str_prop = LoadFileData(map_layer_path(map->propfile.c_str()), &dataSize);
      if (dataSize <= 0 || dataSize == I32_MAX) {
        TraceLog(LOG_ERROR, "tilemap::load_map_data()::Reading data returned null");
      }
      out_package->str_props.assign((char*)_str_prop, dataSize);
      UnloadFileData(_str_prop);
    }
    else {
      if (!out_package->is_success) {
        TraceLog(LOG_ERROR, "tilemap::load_or_create_map_data()::map cannot successfully converted to string to save as file");
      }
      if (!SaveFileData(map_layer_path(map->propfile.c_str()), out_package->str_props.data(), out_package->size_props_str)) {
        TraceLog(LOG_ERROR, "tilemap::save_map_data()::Recieving package data returned with failure");
      }
    }
  }
  str_to_map(map, out_package);
  
  return out_package->is_success;
}

