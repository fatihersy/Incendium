#include "world.h"
#include <core/fmemory.h>

#include "tilemap.h"

typedef struct world_system_state {
  std::array<tilemap, MAX_WORLDMAP_LOCATIONS> map;
  std::array<tilemap_stringtify_package, MAX_WORLDMAP_LOCATIONS>  map_stringtify;
  std::array<worldmap_stage, MAX_WORLDMAP_LOCATIONS> worldmap_locations;
  tilesheet palette;

  f32 palette_zoom;
  u32 next_prop_id;
  worldmap_stage active_stage;
  camera_metrics* in_camera_metrics;
} world_system_state;

static world_system_state * state; 

#define CURR_MAP state->map.at(state->active_stage.map_id)

// 3840x2160 is the dimentions of map selection screen background picture
#define CREATE_WORLDMAP_STAGE_CENTERED(ID, DISPLAY_NAME, FILENAME, SELECTION_SCREEN_LOCATION, WILL_BE_ON_SELECTION_SCREEN)\
  state->worldmap_locations.at(ID) = worldmap_stage(ID, DISPLAY_NAME, FILENAME, SELECTION_SCREEN_LOCATION, true, WILL_BE_ON_SELECTION_SCREEN);

Rectangle get_position_view_rect(Camera2D camera, Vector2 pos, f32 zoom);
constexpr size_t get_renderqueue_prop_index_by_id(i16 zindex, u32 id);

bool world_system_initialize(camera_metrics* _in_camera_metrics) {
  if (state) {
    TraceLog(LOG_WARNING, "world::world_system_initialize()::Initialize called twice");
    return false;
  }
  state = (world_system_state*)allocate_memory_linear(sizeof(world_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "world::world_system_initialize()::State allocation failed");
    return false;
  }
  state->in_camera_metrics = _in_camera_metrics;
  create_tilesheet(TILESHEET_TYPE_MAP, 16*2, 1.1, &state->palette);
  if(!state->palette.is_initialized) {
    TraceLog(LOG_WARNING, "scene_in_game_edit::initialize_scene_in_game_edit()::palette initialization failed");
  }
  state->palette.position = {_in_camera_metrics->screen_offset.x, _in_camera_metrics->screen_offset.y + 100}; // INFO: Tilesheet start position

  { // WORLD LOCATIONS
    CREATE_WORLDMAP_STAGE_CENTERED( 0, "Stage 1",  "stage1",  VECTOR2(   0,    0), false); // main menu background
    CREATE_WORLDMAP_STAGE_CENTERED( 1, "Stage 2",  "stage2",  VECTOR2(1024, 1744), true);
    CREATE_WORLDMAP_STAGE_CENTERED( 2, "Stage 3",  "stage3",  VECTOR2(1467, 1755), true);
    CREATE_WORLDMAP_STAGE_CENTERED( 3, "Stage 4",  "stage4",  VECTOR2(1674, 1727), true);
    CREATE_WORLDMAP_STAGE_CENTERED( 4, "Stage 5",  "stage5",  VECTOR2(2179, 1623), true);
    CREATE_WORLDMAP_STAGE_CENTERED( 5, "Stage 6",  "stage6",  VECTOR2(1811, 1230), true);
    CREATE_WORLDMAP_STAGE_CENTERED( 6, "Stage 7",  "stage7",  VECTOR2(2233, 1240), true);
    CREATE_WORLDMAP_STAGE_CENTERED( 7, "Stage 8",  "stage8",  VECTOR2(2658, 1202), true);
    CREATE_WORLDMAP_STAGE_CENTERED( 8, "Stage 9",  "stage9",  VECTOR2(3009, 1511), true);
    CREATE_WORLDMAP_STAGE_CENTERED( 9, "Stage 10", "stage10", VECTOR2(2767, 1972), true);
    CREATE_WORLDMAP_STAGE_CENTERED(10, "Stage 11", "stage11", VECTOR2(3188, 1415), true);
    CREATE_WORLDMAP_STAGE_CENTERED(11, "Stage 12", "stage12", VECTOR2(3449, 1773), true);
    CREATE_WORLDMAP_STAGE_CENTERED(12, "Stage 13", "stage13", VECTOR2(3073, 1146), true);
    CREATE_WORLDMAP_STAGE_CENTERED(13, "Stage 14", "stage14", VECTOR2(3574, 1291), true);
    CREATE_WORLDMAP_STAGE_CENTERED(14, "Stage 15", "stage15", VECTOR2(3462,  773), true);
    CREATE_WORLDMAP_STAGE_CENTERED(15, "Stage 16", "stage16", VECTOR2(2661,  951), true);
    CREATE_WORLDMAP_STAGE_CENTERED(16, "Stage 17", "stage17", VECTOR2(3548,  974), true);
    CREATE_WORLDMAP_STAGE_CENTERED(17, "Stage 18", "stage18", VECTOR2(2400,  596), true);
    CREATE_WORLDMAP_STAGE_CENTERED(18, "Stage 19", "stage19", VECTOR2(3290,  369), true);
    CREATE_WORLDMAP_STAGE_CENTERED(19, "Stage 20", "stage20", VECTOR2(3449,  224), true);
    CREATE_WORLDMAP_STAGE_CENTERED(20, "Stage 21", "stage21", VECTOR2(1977,  385), true);
    CREATE_WORLDMAP_STAGE_CENTERED(21, "Stage 22", "stage22", VECTOR2(1661,  410), true);
  }
  for (int itr_000 = 0; itr_000 < MAX_WORLDMAP_LOCATIONS; ++itr_000) {
    for (int itr_111 = 0; itr_111 < MAX_TILEMAP_LAYERS; ++itr_111) {
      state->map.at(itr_000).filename.at(itr_111) = TextFormat("%s_layer%d.txt", state->worldmap_locations.at(itr_000).filename.c_str(), itr_111);
    }
    state->map.at(itr_000).propfile = TextFormat("%s_prop.txt", state->worldmap_locations.at(itr_000).filename.c_str());
    create_tilemap(TILESHEET_TYPE_MAP, ZEROVEC2, 100, 60, &state->map.at(itr_000));
    if(!state->map.at(itr_000).is_initialized) {
      TraceLog(LOG_WARNING, "WARNING::scene_in_game_edit::initialize_scene_in_game_edit()::tilemap initialization failed");
    }
    if(!load_or_create_map_data(&state->map.at(itr_000), &state->map_stringtify.at(itr_000))) {
      TraceLog(LOG_WARNING, "world::world_system_initialize()::Stage:%d creating failed", itr_000);
    }
    refresh_render_queue(itr_000);

    if (!state->map_stringtify.at(itr_000).is_success) {
      TraceLog(LOG_ERROR, "game_manager_initialize::game manager unable to load map");
      return false;
    }
    state->worldmap_locations.at(itr_000).spawning_areas.at(0) = {
      state->map.at(itr_000).position.x, state->map.at(itr_000).position.y,
      (f32) (state->map.at(itr_000).map_dim * state->map.at(itr_000).tile_size), (f32) (state->map.at(itr_000).map_dim * state->map.at(itr_000).tile_size)
    };
  }
  return true;
}

/**
 * @brief The size is MAX_WORLDMAP_LOCATIONS
 */
worldmap_stage* get_worldmap_locations(void) {
  return state->worldmap_locations.data();
}
worldmap_stage* get_active_worldmap(void) {
  return &state->active_stage;
}
tilemap* get_active_map(void) {
  return &CURR_MAP;
}
void set_worldmap_location(u16 id) {
  if (id >= MAX_WORLDMAP_LOCATIONS) {
    TraceLog(LOG_WARNING, "world::set_worldmap_location()::Recieved id was out of bound");
    return;
  }
  state->active_stage = state->worldmap_locations.at(id);
}
void set_map_tile(i32 layer, tile src, tile dst) {
  if (layer < 0 || layer >= MAX_TILEMAP_LAYERS) {
    TraceLog(LOG_WARNING, "world::set_map_tile()::Recieved layer was out of bound");
    return;
  }
  if (src.position.x >= MAX_TILEMAP_TILESLOT_X || src.position.x >= MAX_TILEMAP_TILESLOT_Y) {
    TraceLog(LOG_WARNING, "world::set_map_tile()::Recieved tile was out of bound");
    return;
  }
  CURR_MAP.tiles[layer][src.position.x][src.position.y].c[0] = dst.symbol.c[0];
  CURR_MAP.tiles[layer][src.position.x][src.position.y].c[1] = dst.symbol.c[1];
}
tilemap_prop_address get_map_prop_by_pos(Vector2 pos) {
  for (size_t itr_000 = 0; itr_000 < CURR_MAP.static_props.size(); ++itr_000) {
    Rectangle prop_dest = CURR_MAP.static_props.at(itr_000).dest;
    prop_dest.x -= prop_dest.width  * .5f;
    prop_dest.y -= prop_dest.height * .5f;
    if(CheckCollisionPointRec(pos, prop_dest)) { // Props are always centered
      tilemap_prop_address prop = tilemap_prop_address();
      prop.data.prop_static = __builtin_addressof(CURR_MAP.static_props.at(itr_000));
      prop.type = prop.data.prop_static->prop_type;
      return prop;
    }
  }
  for (size_t itr_000 = 0; itr_000 < CURR_MAP.sprite_props.size(); ++itr_000) {
    Rectangle prop_dest = CURR_MAP.sprite_props.at(itr_000).sprite.coord;
    prop_dest.x -= prop_dest.width  * .5f;
    prop_dest.y -= prop_dest.height * .5f;
    if(CheckCollisionPointRec(pos, prop_dest)) { // Props are always centered
      tilemap_prop_address prop = tilemap_prop_address();
      prop.data.prop_sprite = &CURR_MAP.sprite_props.at(itr_000);
      prop.type = prop.data.prop_sprite->prop_type;
      return prop;
    }
  }

  return tilemap_prop_address();
}
constexpr size_t get_renderqueue_prop_index_by_id(i16 zindex, u32 id) {
  if (state == nullptr) {
    TraceLog(LOG_ERROR, "world::get_renderqueue_prop_by_id()::State is not valid");
    return INVALID_IDU32;
  }
  const std::vector<tilemap_prop_address>& _queue = CURR_MAP.render_queue.at(zindex);
  for (size_t itr_000 = 0; itr_000 < _queue.size(); ++itr_000) {
    const tilemap_prop_address * _queue_elm = __builtin_addressof(_queue.at(itr_000));
    if (_queue_elm->type <= TILEMAP_PROP_TYPE_UNDEFINED || _queue_elm->type >= TILEMAP_PROP_TYPE_MAX) {
      continue;
    }
    
    if (_queue_elm->type == TILEMAP_PROP_TYPE_SPRITE && _queue_elm->data.prop_sprite != nullptr && _queue_elm->data.prop_sprite->id == id) return itr_000; 
    if (_queue_elm->type != TILEMAP_PROP_TYPE_SPRITE && _queue_elm->data.prop_static != nullptr && _queue_elm->data.prop_static->id == id) return itr_000;
  }

  return INVALID_IDU32;
}
tilemap_prop_static* get_map_prop_static_by_id(u16 id) {
  for (size_t iter = 0; iter < CURR_MAP.static_props.size(); ++iter) {
    if (CURR_MAP.static_props.at(iter).id == id) {
      return &CURR_MAP.static_props.at(iter);
    }
  }
  TraceLog(LOG_WARNING, "world::get_map_prop_by_id()::No match found");
  return nullptr;  
}
tilemap_prop_sprite* get_map_prop_sprite_by_id(u16 id) {
  for (size_t iter = 0; iter < CURR_MAP.sprite_props.size(); ++iter) {
    if (CURR_MAP.sprite_props.at(iter).id == id) {
      return &CURR_MAP.sprite_props.at(iter);
    }
  }
  TraceLog(LOG_WARNING, "world::get_map_prop_by_id()::No match found");
  return nullptr;  
}


void save_current_map(void) {
  if(!save_map_data(&CURR_MAP, &state->map_stringtify.at(state->active_stage.map_id))) {
    TraceLog(LOG_WARNING, "world::save_current_map()::save_map_data returned false");
  }
}
void load_current_map(void) {
  if(!load_map_data(&CURR_MAP, &state->map_stringtify.at(state->active_stage.map_id))) {
    TraceLog(LOG_WARNING, "world::load_current_map()::load_map_data returned false");
  }

  refresh_render_queue(state->active_stage.map_id);
}

void update_map(void) {
  update_tilemap(&state->map.at(state->active_stage.map_id));
}

void drag_tilesheet(Vector2 vec) {
  state->palette.position.x += vec.x;
  state->palette.position.y += vec.y;
}
void render_map() {
  render_tilemap(&CURR_MAP, state->in_camera_metrics->frustum);
}
void render_map_view_on(Vector2 pos, f32 zoom) {
  render_tilemap(&CURR_MAP, get_position_view_rect(state->in_camera_metrics->handle, pos, zoom));
}

void render_map_palette(f32 zoom) {
  render_tilesheet(&state->palette, zoom);
  state->palette_zoom = zoom;
}

bool add_prop_curr_map(tilemap_prop_static prop_static) {
  if (!prop_static.is_initialized) {
    TraceLog(LOG_WARNING, "world::add_prop_curr_map()::Recieved static prop was empty");
    return false;
  }
  prop_static.id = state->next_prop_id++;
  CURR_MAP.static_props.push_back(prop_static);
  
  refresh_render_queue(state->active_stage.map_id);
  return true;
}
bool add_prop_curr_map(tilemap_prop_sprite prop_sprite) {
  if (!prop_sprite.is_initialized) {
    TraceLog(LOG_WARNING, "world::add_prop_curr_map()::Recieved sprite prop was empty");
    return false;
  }
  prop_sprite.id = state->next_prop_id++;
  CURR_MAP.sprite_props.push_back(prop_sprite);

  refresh_render_queue(state->active_stage.map_id);
  return true;
}
bool remove_prop_cur_map_by_id(u16 id, tilemap_prop_types type) {
  if (type != TILEMAP_PROP_TYPE_SPRITE) {
    if (id >= CURR_MAP.static_props.size()) {
      TraceLog(LOG_WARNING, "world::remove_prop_cur_map_by_id()::Recieved id was out of bound");
      return false;
    }
    for (auto iter = CURR_MAP.static_props.begin(); iter != CURR_MAP.static_props.end(); ++iter) {
      if (iter->id == id) {
        CURR_MAP.static_props.erase(iter);
        return true;
      }
    }
  }
  if (type == TILEMAP_PROP_TYPE_SPRITE) {
    if (id >= CURR_MAP.sprite_props.size()) {
      TraceLog(LOG_WARNING, "world::remove_prop_cur_map_by_id()::Recieved id was out of bound");
      return false;
    }
    for (auto iter = CURR_MAP.sprite_props.begin(); iter != CURR_MAP.sprite_props.end(); iter++) {
      if (iter->id == id) {
        CURR_MAP.sprite_props.erase(iter);
        return true;
      }
    }
  }

  TraceLog(LOG_WARNING, "world::remove_prop_cur_map_by_id()::No match found");
  return false;
}
Rectangle get_position_view_rect(Camera2D camera, Vector2 pos, f32 zoom) {
  int screen_width = BASE_RENDER_RES.x;
  int screen_height = BASE_RENDER_RES.y;
  
  float view_width = screen_width / zoom;
  float view_height = screen_height / zoom;
  
  float x = pos.x;
  float y = pos.y;
  
  x -= camera.offset.x/zoom;
  y -= camera.offset.y/zoom;
  
  return Rectangle{ x, y, view_width, view_height };
}
bool change_prop_zindex(tilemap_prop_types type, u32 id, i16 old_zindex, i16 new_zindex) {
  if (!state) {
    TraceLog(LOG_ERROR, "world::change_prop_zindex()::State is not valid");
    return false;
  }
  if (type <= TILEMAP_PROP_TYPE_UNDEFINED || type >= TILEMAP_PROP_TYPE_MAX) {
    TraceLog(LOG_ERROR, "world::change_prop_zindex()::Type out of bound: %d", type);
    return false;
  }
  
  if (type == TILEMAP_PROP_TYPE_SPRITE) {
    size_t _prop_addr_queue_index = get_renderqueue_prop_index_by_id(old_zindex, id);
    std::vector<tilemap_prop_address>& queue_ref = CURR_MAP.render_queue.at(old_zindex);
    tilemap_prop_address map_prop_backup = queue_ref.at(_prop_addr_queue_index); 
    queue_ref.erase(queue_ref.begin() + _prop_addr_queue_index);

    CURR_MAP.render_queue.at(new_zindex).push_back(tilemap_prop_address(map_prop_backup));
    return true;
  }
  else if (type != TILEMAP_PROP_TYPE_SPRITE) {
    size_t _prop_addr_queue_index = get_renderqueue_prop_index_by_id(old_zindex, id);
    std::vector<tilemap_prop_address>& queue_ref = CURR_MAP.render_queue.at(old_zindex);
    tilemap_prop_address map_prop_backup = queue_ref.at(_prop_addr_queue_index); 
    queue_ref.erase(queue_ref.begin() + _prop_addr_queue_index);

    CURR_MAP.render_queue.at(new_zindex).push_back(tilemap_prop_address(map_prop_backup));
    return true;
  }

  return false;
}
void refresh_render_queue(u16 id) {
  if (state == nullptr) {
    TraceLog(LOG_ERROR, "world::refresh_render_queue()::State is not valid");
    return;
  }
  tilemap& tilemap_ref = state->map.at(id); 
  for (auto& _queue : tilemap_ref.render_queue) {
    _queue.clear();
  }

  for (size_t static_itr_111 = 0; static_itr_111 < tilemap_ref.static_props.size(); ++static_itr_111) {
    tilemap_prop_static * map_static_ptr = __builtin_addressof(tilemap_ref.static_props.at(static_itr_111));

    tilemap_ref.render_queue.at(map_static_ptr->zindex).push_back(tilemap_prop_address(map_static_ptr));
  }

  for (size_t sprite_itr_111 = 0; sprite_itr_111 < tilemap_ref.sprite_props.size(); ++sprite_itr_111) {
    tilemap_prop_sprite * map_sprite_ptr = __builtin_addressof(tilemap_ref.sprite_props.at(sprite_itr_111));

    tilemap_ref.render_queue.at(map_sprite_ptr->zindex).push_back(tilemap_prop_address(map_sprite_ptr));
  }
}

// EXPOSED
tile _get_tile_from_sheet_by_mouse_pos(Vector2 _mouse_pos) {
  return get_tile_from_sheet_by_mouse_pos(&state->palette, _mouse_pos, state->palette_zoom);
}
tile _get_tile_from_map_by_mouse_pos(u16 from_layer, Vector2 _mouse_pos) {
  if (from_layer >= MAX_TILEMAP_LAYERS) {
    TraceLog(LOG_WARNING, "world::_get_tile_from_map_by_mouse_pos()::Recieved layer was out of bound");
    return tile();
  }
  return get_tile_from_map_by_mouse_pos(&CURR_MAP, GetScreenToWorld2D(_mouse_pos, state->in_camera_metrics->handle), from_layer);
}
void _render_tile_on_pos(tile* _tile, Vector2 pos, tilesheet* sheet) {
  if (!_tile || !_tile->is_initialized || !sheet) {
    TraceLog(LOG_WARNING, "world::_render_tile()::One of pointers is not valid");
    return;
  }
  render_tile(&_tile->symbol, Rectangle {pos.x, pos.y, (f32) CURR_MAP.tile_size, (f32) CURR_MAP.tile_size }, sheet);
}
// EXPOSED
