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

#define COLL_PARSE_COLL_BUFFER_PARSE_SYMBOL_STR "_"
#define COLL_PARSE_COLL_BUFFER_PARSE_SYMBOL_C '_'
#define COLL_PARSE_COLL_MEMBER_PARSE_SYMBOL ','
#define COLL_BUFFER_PARSE_TOP_LIMIT 999
#define COLL_MEMBER_PARSE_TOP_LIMIT 99

#define TOTAL_NUMBER_OF_TILES_TO_THE_HEIGHT 18

void map_to_str(tilemap* const map, tilemap_stringtify_package* const out_package);
void str_to_map(tilemap* const map, tilemap_stringtify_package* const out_package);

bool create_tilemap(const tilesheet_type _type,const Vector2 _position,const i32 _grid_size,const i32 _tile_size,tilemap* const out_tilemap) {
  if (!out_tilemap || out_tilemap == nullptr) {
    TraceLog(LOG_ERROR, "tilemap::create_tilemap()::tilemap is not valid");
    return false;
  }
  if (_grid_size * _grid_size > MAX_TILEMAP_TILESLOT && (_type >= TILESHEET_TYPE_MAX || _type <= TILESHEET_TYPE_UNSPECIFIED)) {
    TraceLog(LOG_ERROR, "tilemap::create_tilemap()::grid_size out of bound");
    return false;
  }

  const tilesheet* sheet = get_tilesheet_by_enum(_type);
  if (!sheet || sheet == nullptr) {
    TraceLog(LOG_ERROR, "tilemap::create_tilemap()::Sheet is not valid");
    return false;
  }
  if (_grid_size > 256) {
    TraceLog(LOG_WARNING, "tilemap::create_tilemap()::Grid size over 256: %d", _grid_size);
  }
  if (_grid_size < 26) {
    TraceLog(LOG_WARNING, "tilemap::create_tilemap()::Grid size below the 26: %d", _grid_size);
  }

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
  return true;
}
void create_tilesheet(tilesheet_type _type, i32 _dest_tile_size, f32 _offset, tilesheet* out_tilesheet) {

  if (_type >= TILESHEET_TYPE_MAX || _type <= 0) {
    TraceLog(LOG_ERROR, "ERROR::tilemap::create_tilesheet()::Sheet type out of bound");
    return;
  }
  *out_tilesheet = *get_tilesheet_by_enum(_type);;
  out_tilesheet->dest_tile_size = _dest_tile_size;
  out_tilesheet->offset = _offset;

  out_tilesheet->is_initialized = true; 
}

void update_tilemap(tilemap* const _tilemap) {
  if (_tilemap) {
    for (size_t iter = 0; iter < _tilemap->sprite_props.size(); ++iter) {
      update_sprite(&_tilemap->sprite_props.at(iter).sprite);
    }
  }
}

/**
 * @brief This function doesn't render Y-Based properties. Render them exclusively.
 */
void render_tilemap(const tilemap* _tilemap, Rectangle camera_view) {
  if (!_tilemap) {
    TraceLog(LOG_ERROR, "tilemap::render_tilemap()::Provided map was null");
    return;
  }
  const tilesheet* sheet = get_tilesheet_by_enum(TILESHEET_TYPE_MAP);

  i32 start_x = (i32)((camera_view.x - _tilemap->position.x) / _tilemap->tile_size);
  i32 start_y = (i32)((camera_view.y - _tilemap->position.y) / _tilemap->tile_size);
  i32 end_x = (i32)((camera_view.x + camera_view.width - _tilemap->position.x) / _tilemap->tile_size) + 1;
  i32 end_y = (i32)((camera_view.y + camera_view.height - _tilemap->position.y) / _tilemap->tile_size) + 1;
  
  start_x = start_x < 0 ? 0 : (start_x >= _tilemap->map_dim ? _tilemap->map_dim - 1 : start_x);
  start_y = start_y < 0 ? 0 : (start_y >= _tilemap->map_dim ? _tilemap->map_dim - 1 : start_y);
  end_x = end_x < 0 ? 0 : (end_x > _tilemap->map_dim ? _tilemap->map_dim : end_x);
  end_y = end_y < 0 ? 0 : (end_y > _tilemap->map_dim ? _tilemap->map_dim : end_y);

  for (i32 y = start_y; y < end_y; ++y) {
    for (i32 x = start_x; x < end_x; ++x) {
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

  for (size_t itr_000 = 0; itr_000 < _tilemap->render_z_index_queue.size(); ++itr_000) {
    for (size_t itr_111 = 0; itr_111 < _tilemap->render_z_index_queue.at(itr_000).size(); ++itr_111) 
    {
      const tilemap_prop_address* _queue_prop_ptr = __builtin_addressof(_tilemap->render_z_index_queue.at(itr_000).at(itr_111));
      if (_queue_prop_ptr->type <= TILEMAP_PROP_TYPE_UNDEFINED || _queue_prop_ptr->type >= TILEMAP_PROP_TYPE_MAX) {
        continue;
      }
      if (_queue_prop_ptr->type == TILEMAP_PROP_TYPE_SPRITE) {
        
        if (_queue_prop_ptr->data.prop_static == nullptr) continue;
        tilemap_prop_sprite *const map_prop_ptr = _queue_prop_ptr->data.prop_sprite;
        if (!map_prop_ptr->is_initialized) continue;

        if (!CheckCollisionRecs(camera_view, map_prop_ptr->sprite.coord)) { continue; }

        play_sprite_on_site(&map_prop_ptr->sprite, map_prop_ptr->sprite.tint, map_prop_ptr->sprite.coord);
        continue;
      }
      if (_queue_prop_ptr->type != TILEMAP_PROP_TYPE_SPRITE) {
        if (_queue_prop_ptr->data.prop_sprite == nullptr) continue;
        
        const tilemap_prop_static * map_prop_ptr = _queue_prop_ptr->data.prop_static;
        if (!map_prop_ptr->is_initialized) continue;
        
        Rectangle prop_rect = map_prop_ptr->dest;
        prop_rect.width *= map_prop_ptr->scale;
        prop_rect.height *= map_prop_ptr->scale;

        if (!CheckCollisionRecs(camera_view, prop_rect)) { continue; }
      
        const Texture2D* tex = get_texture_by_enum(map_prop_ptr->tex_id); 
        if (!tex) continue;
      
        const Vector2 origin = VECTOR2(prop_rect.width * .5f, prop_rect.height * .5f);
      
        DrawTexturePro(*tex, map_prop_ptr->source, prop_rect, origin, map_prop_ptr->rotation, map_prop_ptr->tint);
        
        #if DEBUG_COLLISIONS
          Rectangle coll_dest = Rectangle {
            prop_rect.x - origin.x,
            prop_rect.y - origin.y,
            prop_rect.width,
            prop_rect.height
          };
          DrawRectangleLines(
            static_cast<i32>(coll_dest.x), 
            static_cast<i32>(coll_dest.y), 
            static_cast<i32>(coll_dest.width), 
            static_cast<i32>(coll_dest.height), 
            WHITE
          );
        #endif

        continue;
      }
    }
  }
}
void render_props_y_based_all(const tilemap* _tilemap, Rectangle camera_view, i32 start_y, i32 end_y) {
  if (!_tilemap) {
    TraceLog(LOG_ERROR, "tilemap::render_tilemap()::Provided map was null");
    return;
  }
  for (size_t itr_000 = 0; itr_000 < _tilemap->render_y_based_queue.size(); ++itr_000) {
    for (size_t itr_111 = 0; itr_111 < _tilemap->render_y_based_queue.at(itr_000).size(); ++itr_111) {
      const tilemap_prop_address* _queue_prop_ptr = __builtin_addressof(_tilemap->render_y_based_queue.at(itr_000).at(itr_111));
      if (_queue_prop_ptr->type <= TILEMAP_PROP_TYPE_UNDEFINED || _queue_prop_ptr->type >= TILEMAP_PROP_TYPE_MAX) {
        continue;
      }
      if (_queue_prop_ptr->type == TILEMAP_PROP_TYPE_SPRITE) 
      {
        if (_queue_prop_ptr->data.prop_static == nullptr) continue;
        tilemap_prop_sprite *const map_prop_ptr = _queue_prop_ptr->data.prop_sprite;
        if (!map_prop_ptr->is_initialized) continue;
        if (!CheckCollisionRecs(camera_view, map_prop_ptr->sprite.coord)) { continue; }

        if (map_prop_ptr->sprite.coord.y < start_y) {
          continue;
        }
        else if (map_prop_ptr->sprite.coord.y > end_y) {
          break;
        }

        play_sprite_on_site(&map_prop_ptr->sprite, map_prop_ptr->sprite.tint, map_prop_ptr->sprite.coord);
        continue;
      }
      else
      {
        if (_queue_prop_ptr->data.prop_sprite == nullptr) continue;

        const tilemap_prop_static * map_prop_ptr = _queue_prop_ptr->data.prop_static;
        if (!map_prop_ptr->is_initialized) continue;

        Rectangle prop_rect = map_prop_ptr->dest; 

        if (!CheckCollisionRecs(camera_view, prop_rect)) { continue; }
      
        const Texture2D* tex = get_texture_by_enum(map_prop_ptr->tex_id); 
        if (!tex) continue;
      
        const Vector2 origin = VECTOR2(prop_rect.width * .5f, prop_rect.height * .5f);
      
        if (map_prop_ptr->dest.y + (prop_rect.height * .5f) < start_y) {
          continue;
        }
        else if (map_prop_ptr->dest.y + (prop_rect.height * .5f) > end_y) {
          break;
        }
        DrawTexturePro(*tex, map_prop_ptr->source, prop_rect, origin, map_prop_ptr->rotation, map_prop_ptr->tint);

        #if DEBUG_COLLISIONS
          Rectangle coll_dest = Rectangle {
            prop_rect.x - origin.x,
            prop_rect.y - origin.y,
            prop_rect.width,
            prop_rect.height
          };
          DrawRectangleLines(
            static_cast<i32>(coll_dest.x), 
            static_cast<i32>(coll_dest.y), 
            static_cast<i32>(coll_dest.width), 
            static_cast<i32>(coll_dest.height), 
            WHITE
          );
        #endif
        continue;
      }
    }
  }

}
void render_props_y_based_by_zindex(const tilemap* _tilemap, size_t index, Rectangle camera_view, i32 start_y, i32 end_y) {
  for (size_t itr_111 = 0; itr_111 < _tilemap->render_y_based_queue.at(index).size(); ++itr_111) {
    const tilemap_prop_address* _queue_prop_ptr = __builtin_addressof(_tilemap->render_y_based_queue.at(index).at(itr_111));
    if (_queue_prop_ptr->type <= TILEMAP_PROP_TYPE_UNDEFINED || _queue_prop_ptr->type >= TILEMAP_PROP_TYPE_MAX) {
      continue;
    }
    if (_queue_prop_ptr->type == TILEMAP_PROP_TYPE_SPRITE) 
    {
      if (_queue_prop_ptr->data.prop_sprite == nullptr) continue;
      tilemap_prop_sprite *const map_prop_ptr = _queue_prop_ptr->data.prop_sprite;
      if (!map_prop_ptr->is_initialized) continue;
      if (!CheckCollisionRecs(camera_view, map_prop_ptr->sprite.coord)) { continue; }
      if (map_prop_ptr->sprite.coord.y < start_y) {
        continue;
      }
      else if (map_prop_ptr->sprite.coord.y > end_y) {
        break;
      }
      play_sprite_on_site(&map_prop_ptr->sprite, map_prop_ptr->sprite.tint, map_prop_ptr->sprite.coord);
      continue;
    }
    else
    {
      if (_queue_prop_ptr->data.prop_static == nullptr) continue;
      const tilemap_prop_static * map_prop_ptr = _queue_prop_ptr->data.prop_static;
      if (!map_prop_ptr->is_initialized) continue;
      Rectangle prop_rect = map_prop_ptr->dest;
      prop_rect.width *= map_prop_ptr->scale;
      prop_rect.height *= map_prop_ptr->scale;
      if (!CheckCollisionRecs(camera_view, prop_rect)) { continue; }
    
      const Texture2D* tex = get_texture_by_enum(map_prop_ptr->tex_id); 
      if (!tex) continue;
    
      const Vector2 origin = VECTOR2(prop_rect.width * .5f, prop_rect.height * .5f);
    
      if (map_prop_ptr->dest.y + (prop_rect.height * .5f) < start_y) {
        continue;
      }
      else if (map_prop_ptr->dest.y + (prop_rect.height * .5f) > end_y) {
        break;
      }
      DrawTexturePro(*tex, map_prop_ptr->source, prop_rect, origin, map_prop_ptr->rotation, map_prop_ptr->tint);
      #if DEBUG_COLLISIONS
        Rectangle coll_dest = Rectangle {
          prop_rect.x - origin.x,
          prop_rect.y - origin.y,
          prop_rect.width,
          prop_rect.height
        };
        DrawRectangleLines(
          static_cast<i32>(coll_dest.x), 
          static_cast<i32>(coll_dest.y), 
          static_cast<i32>(coll_dest.width), 
          static_cast<i32>(coll_dest.height), 
          WHITE
        );
      #endif
      continue;
    }
  }
}

// INFO: See also calc_mainmenu_prop_dest()
void render_mainmenu(const tilemap* _tilemap, Rectangle camera_view, const app_settings * in_settings) {
  if (!_tilemap) {
    TraceLog(LOG_ERROR, "tilemap::render_tilemap()::Provided map was null");
    return;
  }
  const tilesheet* sheet = get_tilesheet_by_enum(TILESHEET_TYPE_MAP);
  const f32 dynamic_tile_size = static_cast<f32>(in_settings->render_height) / TOTAL_NUMBER_OF_TILES_TO_THE_HEIGHT; // Based off of certain amouth tiles along render height

  Vector2 map_position = Vector2 {
    ((_tilemap->map_dim * dynamic_tile_size) * -0.5f),
    ((_tilemap->map_dim * dynamic_tile_size) * -0.5f)
  };

  i32 start_x = static_cast<i32>((camera_view.x - map_position.x) / dynamic_tile_size);
  i32 start_y = static_cast<i32>((camera_view.y - map_position.y) / dynamic_tile_size);
  i32 end_x   = static_cast<i32>((camera_view.x + camera_view.width - map_position.x) / dynamic_tile_size) + 1;
  i32 end_y   = static_cast<i32>((camera_view.y + camera_view.height - map_position.y) / dynamic_tile_size) + 1;

  start_x = start_x < 0 ? 0 : (start_x >= _tilemap->map_dim ? _tilemap->map_dim - 1 : start_x);
  start_y = start_y < 0 ? 0 : (start_y >= _tilemap->map_dim ? _tilemap->map_dim - 1 : start_y);
  end_x = end_x < 0 ? 0 : (end_x > _tilemap->map_dim ? _tilemap->map_dim : end_x);
  end_y = end_y < 0 ? 0 : (end_y > _tilemap->map_dim ? _tilemap->map_dim : end_y);

  for (i32 y = start_y; y < end_y; ++y) {
    for (i32 x = start_x; x < end_x; ++x) {
      if (x >= MAX_TILEMAP_TILESLOT_X || y >= MAX_TILEMAP_TILESLOT_Y) {
        TraceLog(LOG_ERROR, "tilemap::render_tilemap()::Calculated tile's x or y out of bound");
        continue;
      }
      const i32 x_pos = map_position.x + x * dynamic_tile_size;
      const i32 y_pos = map_position.y + y * dynamic_tile_size;
      
      for (i32 itr_000 = 0; itr_000 < MAX_TILEMAP_LAYERS; ++itr_000) {
        render_tile(&_tilemap->tiles[itr_000][x][y], Rectangle { (f32) x_pos, (f32) y_pos, (f32) dynamic_tile_size, (f32) dynamic_tile_size}, sheet);
      }
    }
  }

  for (size_t itr_000 = 0; itr_000 < _tilemap->render_z_index_queue.size(); ++itr_000) {
    for (size_t itr_111 = 0; itr_111 < _tilemap->render_z_index_queue.at(itr_000).size(); ++itr_111) 
    {
      const tilemap_prop_address * _queue_prop_ptr = __builtin_addressof(_tilemap->render_z_index_queue.at(itr_000).at(itr_111));
      if (_queue_prop_ptr->type <= TILEMAP_PROP_TYPE_UNDEFINED || _queue_prop_ptr->type >= TILEMAP_PROP_TYPE_MAX) {
        continue;
      }
      if (_queue_prop_ptr->type == TILEMAP_PROP_TYPE_SPRITE) {
        if (_queue_prop_ptr->data.prop_sprite == nullptr) continue;
        tilemap_prop_sprite* const map_prop_ptr = _queue_prop_ptr->data.prop_sprite;
        if (!map_prop_ptr->is_initialized) continue;
        
        Rectangle coord = Rectangle { // Position can be usable as a dimention since it's a square and centered
          (map_prop_ptr->sprite.coord.x      / (-_tilemap->position.x)) * -map_position.x,
          (map_prop_ptr->sprite.coord.y      / (-_tilemap->position.y)) * -map_position.y,
          ((map_prop_ptr->sprite.coord.width) / (-_tilemap->position.x)) * -map_position.x,
          ((map_prop_ptr->sprite.coord.height) / (-_tilemap->position.y)) * -map_position.y,
        };
        Vector2 origin = Vector2 {coord.width  * .5f, coord.height * .5f};

        if (!CheckCollisionRecs(camera_view, coord)) { continue; }

        play_sprite_on_site_pro(__builtin_addressof(map_prop_ptr->sprite), coord, origin, map_prop_ptr->sprite.rotation, map_prop_ptr->sprite.tint);
        continue;
      }
      if (_queue_prop_ptr->type != TILEMAP_PROP_TYPE_SPRITE) {
        if (_queue_prop_ptr->data.prop_static == nullptr) continue;
        
        const tilemap_prop_static * map_prop_ptr = _queue_prop_ptr->data.prop_static;
        if (!map_prop_ptr->is_initialized) continue;

        const Texture2D* tex = get_texture_by_enum(map_prop_ptr->tex_id); 
        if (!tex) continue;
        
        Rectangle prop_rect = Rectangle { // Position can be usable as a dimention since it's a square and centered
          (map_prop_ptr->dest.x      / (-_tilemap->position.x)) * -map_position.x,
          (map_prop_ptr->dest.y      / (-_tilemap->position.y)) * -map_position.y,
          ((map_prop_ptr->dest.width  * map_prop_ptr->scale) / (-_tilemap->position.x)) * -map_position.x,
          ((map_prop_ptr->dest.height * map_prop_ptr->scale) / (-_tilemap->position.y)) * -map_position.y,
        };
        Vector2 origin = Vector2 {prop_rect.width * .5f, prop_rect.height * .5f};

        if (!CheckCollisionRecs(camera_view, prop_rect)) { continue; }
      
        DrawTexturePro(*tex, map_prop_ptr->source, prop_rect, origin, map_prop_ptr->rotation, map_prop_ptr->tint);
        continue;
      }
    }
  }
}

void render_tilesheet(const tilesheet* sheet, f32 zoom) {
  if (!sheet) {
    TraceLog(LOG_ERROR, "tilemap::render_tilesheet()::Provided sheet was null");
    return;
  }

  for (i32 itr_000 = 0; itr_000 < sheet->tile_count; ++itr_000) {
    i32 x        = itr_000 % sheet->tile_count_x;
    i32 y        = itr_000 / sheet->tile_count_x;
    f32 dest_x   = x * sheet->offset * sheet->dest_tile_size * zoom;  
    f32 dest_y   = y * sheet->offset * sheet->dest_tile_size * zoom; 
    i16 x_pos    = sheet->offset     + sheet->position.x + dest_x; 
    i16 y_pos    = sheet->offset     + sheet->position.y + dest_y; 
    tile_symbol _tile = tile_symbol(x + TILEMAP_TILE_START_SYMBOL, y + TILEMAP_TILE_START_SYMBOL);

    render_tile(&_tile, Rectangle { (f32) x_pos, (f32) y_pos, (f32) sheet->dest_tile_size * zoom, (f32) sheet->dest_tile_size * zoom}, sheet);
  }
}
/**
 * @brief Unsafe!
 * @param tile Needed for x, y
 */
 void render_tile(const tile_symbol* symbol,const Rectangle dest, const tilesheet* sheet) {
  i32 c0    = symbol->c[0];
  i32 c1    = symbol->c[1];
  i32 x     = c0 - TILEMAP_TILE_START_SYMBOL;
  i32 y     = c1 - TILEMAP_TILE_START_SYMBOL;
  i32 x_pos = x * sheet->tile_size;
  i32 y_pos = y * sheet->tile_size;

  DrawTexturePro(*sheet->atlas_handle, Rectangle {(f32) x_pos, (f32) y_pos, (f32) sheet->tile_size, (f32) sheet->tile_size }, dest, ZEROVEC2, 0.f, WHITE);
}

Vector2 get_tilesheet_dim(const tilesheet* sheet) {
  if (!sheet) {
    TraceLog(LOG_ERROR, "tilemap::get_tilesheet_dim()::Sheet is invalid");
    return ZEROVEC2;
  }
  return Vector2 {
    (sheet->tile_count_x * sheet->offset) * sheet->dest_tile_size,
    (sheet->tile_count_y * sheet->offset) * sheet->dest_tile_size,
  };
}
Rectangle calc_mainmenu_prop_dest(const tilemap * const _tilemap, Rectangle dest, f32 scale, const app_settings * in_settings) {
  const f32 dynamic_tile_size = static_cast<f32>(in_settings->render_height) / TOTAL_NUMBER_OF_TILES_TO_THE_HEIGHT; // Based off of certain amouth tiles along render height

  Vector2 map_position = Vector2 {
    ((_tilemap->map_dim * dynamic_tile_size) * -0.5f),
    ((_tilemap->map_dim * dynamic_tile_size) * -0.5f)
  };
  return Rectangle { // Position can be usable as a dimention since it's a square and centered
    (dest.x      / (-_tilemap->position.x)) * -map_position.x,
    (dest.y      / (-_tilemap->position.y)) * -map_position.y,
    ((dest.width  * scale) / (-_tilemap->position.x)) * -map_position.x,
    ((dest.height * scale) / (-_tilemap->position.y)) * -map_position.y,
  };
}
tile get_tile_from_sheet_by_mouse_pos(const tilesheet* sheet,const Vector2 mouse_pos,const f32 zoom) {
  if (!sheet) {
    TraceLog(LOG_ERROR, "tilemap::get_tile_from_mouse_pos()::Provided sheet was null");
    return tile();
  }
  tile _tile = tile();

  i16 x_pos    = (mouse_pos.x - sheet->offset - sheet->position.x) / zoom; 
  i16 y_pos    = (mouse_pos.y - sheet->offset - sheet->position.y) / zoom; 
  if (x_pos < 0 || y_pos < 0) {
    return tile();
  }
  i16 dest_x   = x_pos       / (sheet->dest_tile_size * sheet->offset);  
  i16 dest_y   = y_pos       / (sheet->dest_tile_size * sheet->offset);  

  if (dest_x < 0 || dest_x > sheet->tile_count_x || dest_y < 0 || dest_y > sheet->tile_count_y) {
    return tile();
  }
  _tile.position.x           = dest_x  * sheet->tile_size;
  _tile.position.y           = dest_y  * sheet->tile_size;
  _tile.symbol = sheet->tile_symbols[dest_x][dest_y];
  _tile.is_initialized = true;

  return _tile;
}
tile get_tile_from_map_by_mouse_pos (const tilemap* map,const Vector2 mouse_pos,const i32 layer) {
  if (!map) {
    TraceLog(LOG_ERROR, "tilemap::get_tile_from_mouse_pos()::Map is not valid");
    return tile();
  }
  tile _tile = tile();
  _tile.position.x = (mouse_pos.x - map->position.x) / map->tile_size;
  _tile.position.y = (mouse_pos.y - map->position.y) / map->tile_size;

  if (_tile.position.x >= map->map_dim || _tile.position.y >= map->map_dim) { // NOTE: Assumes tilemap x and y are unsigned
    return tile();
  }
  _tile.symbol = map->tiles[layer][_tile.position.x][_tile.position.y];
  _tile.is_initialized = true;

  return _tile;
}

void map_to_str(tilemap* const map, tilemap_stringtify_package* const out_package) {
  if (!map || !out_package) {
    TraceLog(LOG_ERROR, "Received a NULL pointer");
    return;
  }
  out_package->is_success = false;
  out_package->str_props.clear();
  out_package->str_collisions.clear();

  zero_memory(out_package->str_tilemap, sizeof(u8) * MAX_TILEMAP_LAYERS * MAX_TILEMAP_TILESLOT * TILESHEET_TILE_SYMBOL_STR_LEN);

  const tilesheet* sheet = get_tilesheet_by_enum(TILESHEET_TYPE_MAP);
  if (!sheet) {
    TraceLog(LOG_ERROR, "tilemap::map_to_str()::Map sheet is not valid");
    return;
  }

  // TILE SERIALIZE
  for (size_t itr_000 = 0u; itr_000 < MAX_TILEMAP_LAYERS; ++itr_000) {
    out_package->size_tilemap_str[itr_000] = sizeof(tile_symbol) * MAX_TILEMAP_TILESLOT_X * MAX_TILEMAP_TILESLOT_Y;

    for (size_t itr_111 = 0u; itr_111 < MAX_TILEMAP_TILESLOT_X; ++itr_111) {
      copy_memory(out_package->str_tilemap[itr_000] + (sizeof(tile_symbol) * itr_111 * MAX_TILEMAP_TILESLOT_Y), map->tiles[itr_000][itr_111], sizeof(tile_symbol) * MAX_TILEMAP_TILESLOT_Y);
    }
  }
  // TILE SERIALIZE


  // STATIC PROP SERIALIZE
  for (size_t itr_000 = 0u; itr_000 < map->static_props.size(); ++itr_000) {
    tilemap_prop_static& _prop = map->static_props.at(itr_000);
    if (!_prop.is_initialized) continue;
    
    const tilemap_prop_static* prop = __builtin_addressof(map->static_props.at(itr_000));
    const i32 _prop_scale = prop->scale * 100.f;
    const i32 _prop_rotation = prop->rotation * 10.f;
    const char* symbol = TextFormat("%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d," PROP_PARSE_PROP_BUFFER_PARSE_SYMBOL_STR,
      static_cast<i32>(prop->prop_id),
      static_cast<i32>(prop->tex_id),
      static_cast<i32>(prop->prop_type),
      static_cast<i32>(_prop_rotation),
      static_cast<i32>(_prop_scale),
      static_cast<i32>(prop->zindex),
      static_cast<i32>(prop->dest.x),
      static_cast<i32>(prop->dest.y),
      static_cast<i32>(prop->tint.r),
      static_cast<i32>(prop->tint.g),
      static_cast<i32>(prop->tint.b),
      static_cast<i32>(prop->tint.a),
      static_cast<i32>(prop->use_y_based_zindex),
      0,
      0,
      0,
      0,
      0
    );
    out_package->str_props.append(symbol);
  }
  // STATIC PROP SERIALIZE


  // SPRITE PROP SERIALIZE
  for (size_t itr_000 = 0; itr_000 < map->sprite_props.size(); ++itr_000) {
    if (!map->sprite_props.at(itr_000).is_initialized) continue;
    
    const tilemap_prop_sprite* prop = __builtin_addressof(map->sprite_props.at(itr_000));
    const i32 _prop_scale = prop->scale * 100.f;
    const i32 _prop_rotation = prop->sprite.rotation * 10.f;
    const char* symbol = TextFormat("%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d,%.4d," PROP_PARSE_PROP_BUFFER_PARSE_SYMBOL_STR,
      static_cast<i32>(prop->prop_id),
      static_cast<i32>(prop->sprite.sheet_id - SHEET_ENUM_MAP_SPRITE_START),
      static_cast<i32>(prop->prop_type),
      static_cast<i32>(_prop_rotation),
      static_cast<i32>(_prop_scale),
      static_cast<i32>(prop->zindex),
      static_cast<i32>(prop->sprite.coord.x),
      static_cast<i32>(prop->sprite.coord.y),
      static_cast<i32>(prop->sprite.tint.r),
      static_cast<i32>(prop->sprite.tint.g),
      static_cast<i32>(prop->sprite.tint.b),
      static_cast<i32>(prop->sprite.tint.a),
      static_cast<i32>(prop->use_y_based_zindex),
      0,
      0,
      0,
      0,
      0
    );
    out_package->str_props.append(symbol);
  }
  // SPRITE PROP SERIALIZE


  // MAP COLLISIONS SERIALIZE
  for (size_t itr_000 = 0; itr_000 < map->collisions.size(); ++itr_000) {
    const Rectangle& coll = map->collisions.at(itr_000).dest;
    const char* symbol = TextFormat("%.4d,%.4d,%.4d,%.4d," PROP_PARSE_PROP_BUFFER_PARSE_SYMBOL_STR, 
      static_cast<i32>(coll.x), 
      static_cast<i32>(coll.y), 
      static_cast<i32>(coll.width), 
      static_cast<i32>(coll.height));
    out_package->str_collisions.append(symbol);
  }
  // MAP COLLISIONS SERIALIZE



  out_package->size_props_str = sizeof(out_package->str_props);
  out_package->is_success = true;
}
void str_to_map(tilemap* const map, tilemap_stringtify_package* const out_package) {
  if (out_package == nullptr || map == nullptr) {
    TraceLog(LOG_ERROR, "tilemap::str_to_map()::Received a NULL pointer");
    return;
  }
  out_package->is_success = false;
  map->sprite_props.clear();
  map->static_props.clear();
  for (auto itr_000 : map->render_z_index_queue) itr_000.clear();
  
  const tilesheet* sheet = get_tilesheet_by_enum(TILESHEET_TYPE_MAP);
  if (!sheet) {
    TraceLog(LOG_ERROR, "tilemap::str_to_map()::Failed to get tilesheet");
    return;
  }
  for (size_t itr_000 = 0; itr_000 < MAX_TILEMAP_LAYERS; ++itr_000) {
    for (size_t itr_111 = 0; itr_111 < MAX_TILEMAP_TILESLOT_X; ++itr_111) {
      copy_memory(
        map->tiles[itr_000][itr_111], 
        out_package->str_tilemap[itr_000] + (sizeof(tile_symbol) * itr_111 * MAX_TILEMAP_TILESLOT_X), 
        sizeof(tile_symbol) * MAX_TILEMAP_TILESLOT_X);
    }
  }

  string_parse_result str_prop_parse_buffer = parse_string(out_package->str_props.c_str(), PROP_PARSE_PROP_BUFFER_PARSE_SYMBOL_C, PROP_BUFFER_PARSE_TOP_LIMIT);

  for (size_t itr_000 = 0; itr_000 < str_prop_parse_buffer.buffer.size(); ++itr_000) {
    string_parse_result str_par_prop_member = parse_string(str_prop_parse_buffer.buffer.at(itr_000), PROP_PARSE_PROP_MEMBER_PARSE_SYMBOL, PROP_MEMBER_PARSE_TOP_LIMIT);
    if (str_par_prop_member.buffer.size() < 17u) {
      TraceLog(LOG_ERROR, "str_to_map: Failed to parse prop data, expected 17 values, got %zu", str_par_prop_member.buffer.size());
      continue;
    }
    tilemap_prop_types type = static_cast<tilemap_prop_types>(TextToInteger(str_par_prop_member.buffer.at(2).c_str()));
    if (type == TILEMAP_PROP_TYPE_SPRITE ) {
      tilemap_prop_sprite prop = tilemap_prop_sprite();
      prop.map_id = map->next_map_id++;
      prop.prop_id = TextToInteger(str_par_prop_member.buffer.at(0).c_str());
      prop.sprite.sheet_id = static_cast<spritesheet_id>(TextToInteger(str_par_prop_member.buffer.at(1).c_str()) + SHEET_ENUM_MAP_SPRITE_START);
      set_sprite(&prop.sprite, true, false);

      prop.prop_type = type;
      prop.sprite.rotation = TextToFloat(str_par_prop_member.buffer.at(3).c_str());
      prop.sprite.rotation = prop.sprite.rotation / 10.f;
      
      prop.scale = TextToFloat(str_par_prop_member.buffer.at(4).c_str());
      prop.scale = prop.scale / 100.f;
      
      prop.zindex = TextToInteger(str_par_prop_member.buffer.at(5).c_str());
      prop.sprite.coord = Rectangle {
        TextToFloat(str_par_prop_member.buffer.at(6).c_str()), TextToFloat(str_par_prop_member.buffer.at(7).c_str()),
        prop.sprite.current_frame_rect.width * prop.scale, prop.sprite.current_frame_rect.height * prop.scale,
      };
      prop.sprite.tint = Color {
        (u8)TextToInteger(str_par_prop_member.buffer.at(8).c_str()), (u8)TextToInteger(str_par_prop_member.buffer.at(9).c_str()),
        (u8)TextToInteger(str_par_prop_member.buffer.at(10).c_str()), (u8)TextToInteger(str_par_prop_member.buffer.at(11).c_str()),
      };
      prop.use_y_based_zindex = static_cast<bool>(TextToInteger(str_par_prop_member.buffer.at(12).c_str()));
      
      prop.sprite.origin = VECTOR2(prop.sprite.coord.width / 2.f, prop.sprite.coord.height / 2.f);

      prop.is_initialized = true;
      map->sprite_props.push_back(prop);
    }
    else {
      tilemap_prop_static prop = tilemap_prop_static();
      prop.map_id = map->next_map_id++;
      prop.prop_id = TextToInteger(str_par_prop_member.buffer.at(0).c_str());
      prop.prop_type = type;
      const tilemap_prop_address template_prop = resource_get_map_prop_by_prop_id(prop.prop_id, prop.prop_type); 

      prop.tex_id    = static_cast<texture_id>(TextToInteger(str_par_prop_member.buffer.at(1).c_str()));
      prop.rotation  = TextToFloat(str_par_prop_member.buffer.at(3).c_str());
      prop.rotation = prop.rotation / 10.f;
      prop.scale     = TextToFloat(str_par_prop_member.buffer.at(4).c_str());
      prop.scale = prop.scale / 100.f;
      prop.zindex    = TextToInteger(str_par_prop_member.buffer.at(5).c_str());

      if (template_prop.data.prop_static and template_prop.data.prop_static != nullptr) {
        prop.source = template_prop.data.prop_static->source;
      }
      
      prop.dest = Rectangle {
        TextToFloat(str_par_prop_member.buffer.at(6).c_str()), TextToFloat(str_par_prop_member.buffer.at(7).c_str()),
        prop.source.width, prop.source.height,
      };
      prop.tint = Color {
        (u8)TextToInteger(str_par_prop_member.buffer.at(8).c_str()), (u8)TextToInteger(str_par_prop_member.buffer.at(9).c_str()),
        (u8)TextToInteger(str_par_prop_member.buffer.at(10).c_str()), (u8)TextToInteger(str_par_prop_member.buffer.at(11).c_str()),
      };
      prop.use_y_based_zindex = static_cast<bool>(TextToInteger(str_par_prop_member.buffer.at(12).c_str()));

      prop.is_initialized = true;
      map->static_props.push_back(prop);
    }
  }

  
  string_parse_result str_coll_par_buffer = parse_string(out_package->str_collisions.c_str(), COLL_PARSE_COLL_BUFFER_PARSE_SYMBOL_C, COLL_BUFFER_PARSE_TOP_LIMIT);
  for (size_t itr_000 = 0; itr_000 < str_coll_par_buffer.buffer.size(); ++itr_000) {
    string_parse_result str_coll_par_member = parse_string(str_coll_par_buffer.buffer.at(itr_000), COLL_PARSE_COLL_MEMBER_PARSE_SYMBOL, COLL_MEMBER_PARSE_TOP_LIMIT);
    if (str_coll_par_member.buffer.size() < 4u) {
      TraceLog(LOG_ERROR, "tilemap::str_to_map() Failed to parse collision data, expected 4 values, got %zu", str_coll_par_member.buffer.size());
      continue;
    }
    map_collision collision = map_collision(map->next_collision_id++, Rectangle {
      TextToFloat(str_coll_par_member.buffer.at(0).c_str()), TextToFloat(str_coll_par_member.buffer.at(1).c_str()),
      TextToFloat(str_coll_par_member.buffer.at(2).c_str()), TextToFloat(str_coll_par_member.buffer.at(3).c_str())
    });
    
    map->collisions.push_back(collision);
  }

  out_package->is_success = true;
}
/**
 * @brief Saves at "%s%s", RESOURCE_PATH, "map.txt"
 * 
 * @param map in_map
 * @param out_package Needed because of the local variable array limit. Array must be defined at initialization. Also extracts map data and operation results.
 */
bool save_map_data(tilemap* const map, tilemap_stringtify_package* const out_package) {
  map_to_str(map, out_package);
  if (!out_package->is_success) {
    TraceLog(LOG_ERROR, "tilemap::save_map_data()::Map data serialization failed");
    return false;
  }
  for (i32 itr_000 = 0; itr_000 < MAX_TILEMAP_LAYERS; ++itr_000) {
    SaveFileData(map_layer_path(map->filename.at(itr_000).c_str()), out_package->str_tilemap[itr_000], out_package->size_tilemap_str[itr_000]);
  }
  SaveFileData(map_layer_path(map->propfile.c_str()), out_package->str_props.data(), out_package->str_props.size());
  SaveFileData(map_layer_path(map->collisionfile.c_str()), out_package->str_collisions.data(), out_package->str_collisions.size());

  return true;
}
/**
 * @brief Loads from "%s%s", RESOURCE_PATH, "map.txt"
 * 
 * @param map out_map
 * @param out_package Needed because of the local variable array limit. Array must be defined at initialization. Also extracts map data and operation results.
 */
bool load_map_data(tilemap* const map, tilemap_stringtify_package* const out_package) {
  {
    for(int i=0; i<MAX_TILEMAP_LAYERS; ++i){
      int dataSize = 1;
      u8* _str_tile = LoadFileData(map_layer_path(map->filename.at(i).c_str()), &dataSize);
      if (dataSize <= 0) {
        TraceLog(LOG_ERROR, "tilemap::load_map_data()::Reading data returned null");
      }
      copy_memory(&out_package->str_tilemap[i], _str_tile, dataSize);
      UnloadFileData(_str_tile);
    }
  }
  {
    int dataSize = 1;
    u8* _str_prop = LoadFileData(map_layer_path(map->propfile.c_str()), &dataSize);
    if (dataSize <= 0) {
      TraceLog(LOG_ERROR, "tilemap::load_map_data()::Reading data returned null");
    }
    out_package->str_props.assign((char*)_str_prop, dataSize);
    UnloadFileData(_str_prop);
  }
  
  str_to_map(map, out_package);
  
  return out_package->is_success;
}
bool load_or_create_map_data(tilemap* const map, tilemap_stringtify_package* const out_package) {
  map_to_str(map, out_package);
  {
    for(int i=0; i<MAX_TILEMAP_LAYERS; ++i){
      if (FileExists(map_layer_path(map->filename.at(i).c_str()))) {
        int dataSize = 1;
        u8* _str_tile = LoadFileData(map_layer_path(map->filename.at(i).c_str()), &dataSize);
        if (dataSize > 1 && dataSize < TOTAL_ALLOCATED_MEMORY) {
          out_package->size_tilemap_str[i] = dataSize;
          copy_memory(out_package->str_tilemap[i], _str_tile, dataSize);
        }
        if (_str_tile) {
          UnloadFileData(_str_tile);
        }
      }
      else {
        if (!out_package->is_success) {
          TraceLog(LOG_ERROR, "tilemap::load_or_create_map_data()::map cannot successfully converted to string to save as file");
        }
        if (!SaveFileData(map_layer_path(map->filename.at(i).c_str()), out_package->str_tilemap[i], out_package->size_tilemap_str[i])) {
          TraceLog(LOG_ERROR, "tilemap::load_or_create_map_data()::Recieving package data returned with failure");
        }
      }
    }
  }
  {
    if (FileExists(map_layer_path(map->propfile.c_str()))) {
      int dataSize = 1;
      u8* _str_prop = LoadFileData(map_layer_path(map->propfile.c_str()), &dataSize);
      if (dataSize > 1 && dataSize < TOTAL_ALLOCATED_MEMORY) {
        out_package->size_props_str = dataSize;
        out_package->str_props.assign(reinterpret_cast<char*>(_str_prop), dataSize);
      }
      if (_str_prop) {
        UnloadFileData(_str_prop);
      }
    }
    else {
      if (!out_package->is_success) {
        TraceLog(LOG_ERROR, "tilemap::load_or_create_map_data()::map cannot successfully converted to string to save as file");
      }
      if (!SaveFileData(map_layer_path(map->propfile.c_str()), out_package->str_props.data(), out_package->size_props_str)) {
        TraceLog(LOG_ERROR, "tilemap::load_or_create_map_data()::Recieving package data returned with failure");
      }
    }
  }
  {
    if (FileExists(map_layer_path(map->collisionfile.c_str()))) {
      int dataSize = 1;
      u8* _str_collision = LoadFileData(map_layer_path(map->collisionfile.c_str()), &dataSize);
      if (dataSize > 1 && dataSize < TOTAL_ALLOCATED_MEMORY) {
        out_package->size_collisions_str = dataSize;
        out_package->str_collisions.assign(reinterpret_cast<char*>(_str_collision), dataSize);
      }
      if (_str_collision) {
        UnloadFileData(_str_collision);
      }
    }
    else {
      if (!out_package->is_success) {
        TraceLog(LOG_ERROR, "tilemap::load_or_create_map_data()::map cannot successfully converted to string to save as file");
      }
      if (!SaveFileData(map_layer_path(map->collisionfile.c_str()), out_package->str_collisions.data(), out_package->size_collisions_str)) {
        TraceLog(LOG_ERROR, "tilemap::load_or_create_map_data()::Recieving package data returned with failure");
      }
    }
  }
  str_to_map(map, out_package);
  
  return out_package->is_success;
}
