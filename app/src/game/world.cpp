#include "world.h"
#include <algorithm>

#include <core/fmemory.h>

#include "tilemap.h"

typedef struct world_system_state {
  std::array<tilemap, MAX_WORLDMAP_LOCATIONS> map;
  std::array<tilemap_stringtify_package, MAX_WORLDMAP_LOCATIONS>  map_stringtify;
  std::array<worldmap_stage, MAX_WORLDMAP_LOCATIONS> worldmap_locations;
  tilesheet palette;

  f32 palette_zoom;
  worldmap_stage active_map_stage;
  const camera_metrics * in_camera_metrics;
  const app_settings * in_app_settings;
  tilemap * active_map;
} world_system_state;

static world_system_state * state = nullptr;

#define MAINMENU_STAGE_INDEX 0

constexpr Rectangle get_position_view_rect(Camera2D camera, Vector2 pos, f32 zoom);
constexpr size_t get_renderqueue_prop_index_by_id(i16 zindex, i32 map_id);
void sort_render_y_based_queue(i32 id);

void create_worldmap_stage(i32 in_map_id, std::string in_displayname, std::string filename, i32 in_stage_level,
  i32 total_spawn_count, f32 spawn_scale, Vector2 boss_spawn_location, f32 boss_scale, 
  Vector2 in_screen_location, bool is_active) {
  state->worldmap_locations.at(in_map_id) = worldmap_stage(
    in_map_id, in_displayname, filename, in_stage_level, total_spawn_count, spawn_scale, boss_spawn_location, boss_scale, in_screen_location, true, is_active
  );
}

bool world_system_initialize(const app_settings* _in_app_settings) {
  if (state) {
    TraceLog(LOG_WARNING, "world::world_system_initialize()::Initialize called twice");
    return false;
  }
  state = (world_system_state*)allocate_memory_linear(sizeof(world_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "world::world_system_initialize()::State allocation failed");
    return false;
  }
  if (_in_app_settings == nullptr) {
    TraceLog(LOG_ERROR, "world::world_system_initialize()::Recieved pointer(s) is/are null");
    return false;
  }
  state->in_app_settings = _in_app_settings;
  create_tilesheet(TILESHEET_TYPE_MAP, 16u*2u, 1.1f, __builtin_addressof(state->palette));
  if(!state->palette.is_initialized) {
    TraceLog(LOG_WARNING, "world::world_system_initialize()::palette initialization failed");
    return false;
  }
  state->palette.position = Vector2 {0.f, 100.f};

  { // WORLD LOCATIONS
    create_worldmap_stage( 0, "Stage 1",  "stage1" , 1, 0, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(   0,    0), false); // main menu background
    create_worldmap_stage( 1, "Stage 2",  "stage2" , 1, 10, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(1024, 1744), true );
    create_worldmap_stage( 2, "Stage 3",  "stage3" , 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(1467, 1755), true );
    create_worldmap_stage( 3, "Stage 4",  "stage4" , 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(1674, 1727), true );
    create_worldmap_stage( 4, "Stage 5",  "stage5" , 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(2179, 1623), true );
    create_worldmap_stage( 5, "Stage 6",  "stage6" , 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(1811, 1230), true );
    create_worldmap_stage( 6, "Stage 7",  "stage7" , 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(2233, 1240), true );
    create_worldmap_stage( 7, "Stage 8",  "stage8" , 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(2658, 1202), true );
    create_worldmap_stage( 8, "Stage 9",  "stage9" , 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(3009, 1511), true );
    create_worldmap_stage( 9, "Stage 10", "stage10", 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(2767, 1972), true );
    create_worldmap_stage(10, "Stage 11", "stage11", 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(3188, 1415), true );
    create_worldmap_stage(11, "Stage 12", "stage12", 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(3449, 1773), true );
    create_worldmap_stage(12, "Stage 13", "stage13", 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(3073, 1146), true );
    create_worldmap_stage(13, "Stage 14", "stage14", 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(3574, 1291), true );
    create_worldmap_stage(14, "Stage 15", "stage15", 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(3462,  773), true );
    create_worldmap_stage(15, "Stage 16", "stage16", 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(2661,  951), true );
    create_worldmap_stage(16, "Stage 17", "stage17", 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(3548,  974), true );
    create_worldmap_stage(17, "Stage 18", "stage18", 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(2400,  596), true );
    create_worldmap_stage(18, "Stage 19", "stage19", 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(3290,  369), true );
    create_worldmap_stage(19, "Stage 20", "stage20", 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(3449,  224), true );
    create_worldmap_stage(20, "Stage 21", "stage21", 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(1977,  385), true );
    create_worldmap_stage(21, "Stage 22", "stage22", 1, 75, 1, VECTOR2(0.f, 0.f), .5f, VECTOR2(1661,  410), true );
  }
  for (i32 itr_000 = 0; itr_000 < MAX_WORLDMAP_LOCATIONS; ++itr_000)
  {
    for (i32 itr_111 = 0; itr_111 < MAX_TILEMAP_LAYERS; ++itr_111) {
      state->map.at(itr_000).filename.at(itr_111) = TextFormat("%s_layer%d.txt", state->worldmap_locations.at(itr_000).filename.c_str(), itr_111);
    }
    state->map.at(itr_000).propfile = TextFormat("%s_prop.txt", state->worldmap_locations.at(itr_000).filename.c_str());
    state->map.at(itr_000).collisionfile = TextFormat("%s_collision.txt", state->worldmap_locations.at(itr_000).filename.c_str());

    if (!create_tilemap(TILESHEET_TYPE_MAP, ZEROVEC2, 100, 60, __builtin_addressof(state->map.at(itr_000)))) {
      TraceLog(LOG_WARNING, "world::world_system_initialize()::tilemap initialization failed");
      continue;
    }
    if(!state->map.at(itr_000).is_initialized) {
      TraceLog(LOG_WARNING, "world::world_system_initialize()::tilemap initialization failed");
      continue;
    }

    if(!load_or_create_map_data(&state->map.at(itr_000), &state->map_stringtify.at(itr_000))) {
      TraceLog(LOG_WARNING, "world::world_system_initialize()::Stage:%d creation failed", itr_000);
      continue;
    }
    if (!state->map_stringtify.at(itr_000).is_success) {
      TraceLog(LOG_WARNING, "world::world_system_initialize::Stage:%d read failed", itr_000);
      continue;
    }

    state->worldmap_locations.at(itr_000).spawning_areas.at(0) = {
      state->map.at(itr_000).position.x, state->map.at(itr_000).position.y,
      static_cast<f32>((state->map.at(itr_000).map_dim * state->map.at(itr_000).tile_size)), 
      static_cast<f32>((state->map.at(itr_000).map_dim * state->map.at(itr_000).tile_size))
    };

    refresh_render_queue(itr_000);
  }
  return true;
}
bool world_system_begin(const camera_metrics* _in_camera_metrics) {
  if (!state) {
    TraceLog(LOG_ERROR, "world::world_system_initialize()::State allocation failed");
    return false;
  }
  if (!_in_camera_metrics || _in_camera_metrics == nullptr) {
    TraceLog(LOG_ERROR, "world::world_system_initialize()::Recieved pointer(s) is/are null");
    return false;
  }
  state->in_camera_metrics = _in_camera_metrics;
  return true;
}

/**
 * @brief The size is MAX_WORLDMAP_LOCATIONS
 */
const worldmap_stage* get_worldmap_locations(void) {
  return state->worldmap_locations.data();
}
const worldmap_stage* get_active_worldmap(void) {
  return __builtin_addressof(state->active_map_stage);
}
const tilemap* get_active_map(void) {
  return state->active_map;
}
tilemap** get_active_map_ptr(void) {
  return __builtin_addressof(state->active_map);
}
void set_worldmap_location(i32 id) {
  if (id >= MAX_WORLDMAP_LOCATIONS) {
    TraceLog(LOG_WARNING, "world::set_worldmap_location()::Recieved id was out of bound");
    return;
  }
  state->active_map_stage = state->worldmap_locations.at(id);
  state->active_map = __builtin_addressof(state->map.at(state->active_map_stage.map_id));
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
  state->active_map->tiles[layer][src.position.x][src.position.y].c[0] = dst.symbol.c[0];
  state->active_map->tiles[layer][src.position.x][src.position.y].c[1] = dst.symbol.c[1];
}
tilemap_prop_address get_map_prop_by_pos(Vector2 pos) {
  if (!state || state == nullptr) {
    TraceLog(LOG_ERROR, "world::get_map_prop_by_pos()::State is not valid");
    return tilemap_prop_address();
  }
  for (size_t itr_000 = 0; itr_000 < state->active_map->static_props.size(); ++itr_000) {
    Rectangle prop_dest = state->active_map->static_props.at(itr_000).dest;
    prop_dest.x -= prop_dest.width  * .5f;
    prop_dest.y -= prop_dest.height * .5f;
    if(CheckCollisionPointRec(pos, prop_dest)) { // Props are always centered
      tilemap_prop_address prop = tilemap_prop_address(__builtin_addressof(state->active_map->static_props.at(itr_000)));
      prop.type = prop.data.prop_static->prop_type;
      return prop;
    }
  }
  for (size_t itr_000 = 0; itr_000 < state->active_map->sprite_props.size(); ++itr_000) {
    Rectangle prop_dest = state->active_map->sprite_props.at(itr_000).sprite.coord;
    prop_dest.x -= prop_dest.width  * .5f;
    prop_dest.y -= prop_dest.height * .5f;
    if(CheckCollisionPointRec(pos, prop_dest)) { // Props are always centered
      tilemap_prop_address prop = tilemap_prop_address(__builtin_addressof(state->active_map->sprite_props.at(itr_000)));
      prop.type = prop.data.prop_sprite->prop_type;
      return prop;
    }
  }

  return tilemap_prop_address();
}
map_collision* get_map_collision_by_pos(Vector2 pos) {
  if (!state || state == nullptr) {
    TraceLog(LOG_ERROR, "world::get_map_collision_by_pos()::State is not valid");
    return nullptr;
  }
  for (size_t itr_000 = 0; itr_000 < state->active_map->collisions.size(); ++itr_000) {
    Rectangle coll_dest = state->active_map->collisions.at(itr_000).dest;
    if(CheckCollisionPointRec(pos, coll_dest)) {
      map_collision* map_coll = __builtin_addressof(state->active_map->collisions.at(itr_000));
      return map_coll;
    }
  }

  return nullptr;
}
constexpr size_t get_renderqueue_prop_index_by_id(i16 zindex, i32 map_id) {
  if (state == nullptr) {
    TraceLog(LOG_ERROR, "world::get_renderqueue_prop_by_id()::State is not valid");
    return INVALID_IDU32;
  }
  const std::vector<tilemap_prop_address>& _queue = state->active_map->render_z_index_queue.at(zindex);
  for (size_t itr_000 = 0; itr_000 < _queue.size(); ++itr_000) {
    const tilemap_prop_address * _queue_elm = __builtin_addressof(_queue.at(itr_000));
    if (_queue_elm->type <= TILEMAP_PROP_TYPE_UNDEFINED || _queue_elm->type >= TILEMAP_PROP_TYPE_MAX) {
      continue;
    }
    
    if (_queue_elm->type == TILEMAP_PROP_TYPE_SPRITE && _queue_elm->data.prop_sprite != nullptr && _queue_elm->data.prop_sprite->map_id == map_id) return itr_000; 
    if (_queue_elm->type != TILEMAP_PROP_TYPE_SPRITE && _queue_elm->data.prop_static != nullptr && _queue_elm->data.prop_static->map_id == map_id) return itr_000;
  }

  return INVALID_IDU32;
}
tilemap_prop_static* get_map_prop_static_by_id(i32 map_id) {
  for (size_t iter = 0; iter < state->active_map->static_props.size(); ++iter) {
    if (state->active_map->static_props.at(iter).map_id == map_id) {
      return __builtin_addressof(state->active_map->static_props.at(iter));
    }
  }
  TraceLog(LOG_WARNING, "world::get_map_prop_by_id()::No match found");
  return nullptr;  
}
tilemap_prop_sprite* get_map_prop_sprite_by_id(i32 map_id) {
  for (size_t iter = 0; iter < state->active_map->sprite_props.size(); ++iter) {
    if (state->active_map->sprite_props.at(iter).map_id == map_id) {
      return __builtin_addressof(state->active_map->sprite_props.at(iter));
    }
  }
  TraceLog(LOG_WARNING, "world::get_map_prop_by_id()::No match found");
  return nullptr;  
}
const map_collision* get_map_collision_by_id(i32 coll_id) {
  if (!state || state == nullptr) {
    TraceLog(LOG_ERROR, "world::get_map_collision_by_id()::State is not valid");
    return nullptr;
  }
  for (size_t itr_000 = 0; itr_000 < state->active_map->collisions.size(); ++itr_000) {
    if (state->active_map->collisions.at(itr_000).coll_id == coll_id) {
      return __builtin_addressof(state->active_map->collisions.at(itr_000));
    }
  }

  return nullptr;
}


void save_current_map(void) {
  if(!save_map_data(state->active_map, &state->map_stringtify.at(state->active_map_stage.map_id))) {
    TraceLog(LOG_WARNING, "world::save_current_map()::save_map_data returned false");
  }
}
void load_current_map(void) {
  if(!load_map_data(state->active_map, &state->map_stringtify.at(state->active_map_stage.map_id))) {
    TraceLog(LOG_WARNING, "world::load_current_map()::load_map_data returned false");
  }

  refresh_render_queue(state->active_map_stage.map_id);
}

void update_map(void) {
  update_tilemap(&state->map.at(state->active_map_stage.map_id));
}

void drag_tilesheet(Vector2 vec) {
  state->palette.position.x += vec.x;
  state->palette.position.y += vec.y;
}
void render_map() {
  if (state->active_map_stage.map_id == MAINMENU_STAGE_INDEX) {
    render_mainmenu(state->active_map, state->in_camera_metrics->frustum, state->in_app_settings);
  }
  else {
    render_tilemap(state->active_map, state->in_camera_metrics->frustum);
  }
}

void _render_props_y_based(i32 start_y, i32 end_y) {
  render_props_y_based_all(state->active_map, state->in_camera_metrics->frustum, start_y, end_y);
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
  prop_static.map_id = state->active_map->next_map_id++;
  state->active_map->static_props.push_back(prop_static);
  
  refresh_render_queue(state->active_map_stage.map_id);
  return true;
}
bool add_prop_curr_map(tilemap_prop_sprite prop_sprite) {
  if (!prop_sprite.is_initialized) {
    TraceLog(LOG_WARNING, "world::add_prop_curr_map()::Recieved sprite prop was empty");
    return false;
  }
  prop_sprite.map_id = state->active_map->next_map_id++;
  state->active_map->sprite_props.push_back(prop_sprite);

  refresh_render_queue(state->active_map_stage.map_id);
  return true;
}
bool add_map_coll_curr_map(Rectangle in_collision) {
  f32 map_extent = state->active_map->position.x + (state->active_map->map_dim * state->active_map->tile_size);
  bool in_bounds =
    in_collision.x > state->active_map->position.x ||
    in_collision.y > state->active_map->position.y ||
    in_collision.x + in_collision.width  < map_extent ||
    in_collision.y + in_collision.height < map_extent;
  if (!in_bounds) {
    TraceLog(LOG_WARNING, "world::add_map_coll_curr_map()::Collision out of the bounds of map");
    return false;
  }
  state->active_map->collisions.push_back(map_collision(state->active_map->next_collision_id++, in_collision));
  return true;
}
bool remove_prop_cur_map_by_id(i32 map_id, tilemap_prop_types type) {
  bool found = false;

  if (type != TILEMAP_PROP_TYPE_SPRITE) {
    for (size_t itr_000 = 0; itr_000 < state->active_map->static_props.size(); ++itr_000) {
      if (state->active_map->static_props.at(itr_000).map_id == map_id) {
        state->active_map->static_props.erase(state->active_map->static_props.begin() + itr_000);
        found = true;
        break;
      }
    }
  }
  if (type == TILEMAP_PROP_TYPE_SPRITE) {
    for (size_t itr_000 = 0; itr_000 < state->active_map->sprite_props.size(); ++itr_000) {
      if (state->active_map->sprite_props.at(itr_000).map_id == map_id) {
        state->active_map->sprite_props.erase(state->active_map->sprite_props.begin() + itr_000);
        found = true;
        break;
      }
    }
  }

  if (found) {
    refresh_render_queue(state->active_map_stage.map_id);
    return true;
  }
  else {
    TraceLog(LOG_WARNING, "world::remove_prop_cur_map_by_id()::No match found");
    return false;
  }
}
bool remove_map_collision_by_id(i32 coll_id) {
  if (!state || state == nullptr) {
    TraceLog(LOG_ERROR, "world::get_map_collision_by_id()::State is not valid");
    return false;
  }
  for (size_t itr_000 = 0; itr_000 < state->active_map->collisions.size(); ++itr_000) {
    if (state->active_map->collisions.at(itr_000).coll_id == coll_id) {
      state->active_map->collisions.erase(state->active_map->collisions.begin() + itr_000);
      return true;
    }
  }
  return false;
}
constexpr Rectangle get_position_view_rect(Camera2D camera, Vector2 pos, f32 zoom) {
  int screen_width = state->in_app_settings->render_width;
  int screen_height = state->in_app_settings->render_height;
  
  float view_width = screen_width / zoom;
  float view_height = screen_height / zoom;
  
  float x = pos.x;
  float y = pos.y;
  
  x -= camera.offset.x/zoom;
  y -= camera.offset.y/zoom;
  
  return Rectangle{ x, y, view_width, view_height };
}
void refresh_render_queue(i32 id) {
  if (state == nullptr) {
    TraceLog(LOG_ERROR, "world::refresh_render_queue()::State is not valid");
    return;
  }
  tilemap& tilemap_ref = state->map.at(id);
  std::vector<tilemap_prop_static>& static_prop_queue = tilemap_ref.static_props;
  std::vector<tilemap_prop_sprite>& sprite_prop_queue = tilemap_ref.sprite_props;
  for (auto& _queue : tilemap_ref.render_z_index_queue) {
    _queue.clear();
  }
  for (auto& _queue : tilemap_ref.render_y_based_queue) {
    _queue.clear();
  }

  for (size_t static_itr_111 = 0; static_itr_111 < static_prop_queue.size(); ++static_itr_111) {
    tilemap_prop_static * map_static_ptr = __builtin_addressof(static_prop_queue.at(static_itr_111));

    if (map_static_ptr->prop_type <= TILEMAP_PROP_TYPE_UNDEFINED || map_static_ptr->prop_type >= TILEMAP_PROP_TYPE_MAX || map_static_ptr->prop_type == TILEMAP_PROP_TYPE_SPRITE) {
      static_prop_queue.erase(static_prop_queue.begin() + static_itr_111);
      continue;
    }

    if (map_static_ptr->use_y_based_zindex) {
      tilemap_ref.render_y_based_queue.at(map_static_ptr->zindex).push_back(tilemap_prop_address(map_static_ptr));
    }
    else if(map_static_ptr->zindex >= 0 && map_static_ptr->zindex < MAX_Z_INDEX_SLOT) {
      tilemap_ref.render_z_index_queue.at(map_static_ptr->zindex).push_back(tilemap_prop_address(map_static_ptr));
    }
    else {
      TraceLog(LOG_WARNING, "world::refresh_render_queue()::Prop's Z-index is out of bound, set it to 0");
      map_static_ptr->zindex = 0;
      tilemap_ref.render_z_index_queue.at(0).push_back(tilemap_prop_address(map_static_ptr));
    }
  }

  for (size_t sprite_itr_111 = 0; sprite_itr_111 < sprite_prop_queue.size(); ++sprite_itr_111) {
    tilemap_prop_sprite * map_sprite_ptr = __builtin_addressof(sprite_prop_queue.at(sprite_itr_111));

    if (map_sprite_ptr->prop_type != TILEMAP_PROP_TYPE_SPRITE) {
      sprite_prop_queue.erase(sprite_prop_queue.begin() + sprite_itr_111);
      continue;
    }

    if (map_sprite_ptr->use_y_based_zindex) {
      tilemap_ref.render_y_based_queue.at(map_sprite_ptr->zindex).push_back(tilemap_prop_address(map_sprite_ptr));
    }
    else if(map_sprite_ptr->zindex >= 0 && map_sprite_ptr->zindex < MAX_Z_INDEX_SLOT) { 
      tilemap_ref.render_z_index_queue.at(map_sprite_ptr->zindex).push_back(tilemap_prop_address(map_sprite_ptr));
    }
    else {
      TraceLog(LOG_WARNING, "world::refresh_render_queue()::Prop's Z-index is out of bound, set it to 0");
      map_sprite_ptr->zindex = 0;
      tilemap_ref.render_z_index_queue.at(0).push_back(tilemap_prop_address(map_sprite_ptr));
    }
  }

  sort_render_y_based_queue(id);
}
void sort_render_y_based_queue(i32 id) {
  if (state == nullptr) {
    TraceLog(LOG_ERROR, "world::refresh_render_queue()::State is not valid");
    return;
  }

  for (i32 itr_000 = 0; itr_000 < MAX_Y_INDEX_SLOT; ++itr_000) {
    std::vector<tilemap_prop_address>& queue = state->map.at(id).render_y_based_queue.at(itr_000); 

    std::sort(queue.begin(), queue.end(), [](const tilemap_prop_address& a, const tilemap_prop_address& b) {
      if (a.type == TILEMAP_PROP_TYPE_SPRITE) {
        tilemap_prop_sprite * _prp_sprite_a = a.data.prop_sprite;
        i32 y_value_sprite_a = _prp_sprite_a->sprite.coord.y + _prp_sprite_a->sprite.coord.height;

        if (b.type == TILEMAP_PROP_TYPE_SPRITE) {
          tilemap_prop_sprite * _prp_sprite_b = b.data.prop_sprite;
          i32 y_value_sprite_b = _prp_sprite_b->sprite.coord.y + _prp_sprite_b->sprite.coord.height;

          return y_value_sprite_a < y_value_sprite_b;
        }
        else if (b.type != TILEMAP_PROP_TYPE_SPRITE) {
          tilemap_prop_static * _prp_static_b = b.data.prop_static;
          i32 y_value_static_b = _prp_static_b->dest.y + (_prp_static_b->dest.height * _prp_static_b->scale);
        
          return y_value_sprite_a < y_value_static_b;
        }
        else return false;
      }
      else if (a.type != TILEMAP_PROP_TYPE_SPRITE) {
        tilemap_prop_static * _prp_static_a = a.data.prop_static;
        i32 y_value_sprite_a = _prp_static_a->dest.y + (_prp_static_a->dest.height * _prp_static_a->scale);

        if (b.type == TILEMAP_PROP_TYPE_SPRITE) {
          tilemap_prop_sprite * _prp_sprite_b = b.data.prop_sprite;
          i32 y_value_sprite_b = _prp_sprite_b->sprite.coord.y + _prp_sprite_b->sprite.coord.height;
        
          return y_value_sprite_a < y_value_sprite_b;
        }
        else if (b.type != TILEMAP_PROP_TYPE_SPRITE) {
          tilemap_prop_static * _prp_static_b = b.data.prop_static;
          i32 y_value_static_b = _prp_static_b->dest.y + (_prp_static_b->dest.height * _prp_static_b->scale);
        
          return y_value_sprite_a < y_value_static_b;
        }
        else return false;
      }
      else return false;
    });
  }
}
void _sort_render_y_based_queue(void) {
  if (!state || state == nullptr) {
    TraceLog(LOG_ERROR, "world::_sort_render_y_based_queue()::State is not valid");
    return;
  }
  sort_render_y_based_queue(state->active_map_stage.map_id);
}

// EXPOSED
tile _get_tile_from_sheet_by_mouse_pos(Vector2 _mouse_pos) {
  return get_tile_from_sheet_by_mouse_pos(&state->palette, _mouse_pos, state->palette_zoom);
}
tile _get_tile_from_map_by_mouse_pos(i32 from_layer, Vector2 _mouse_pos) {
  if (from_layer >= MAX_TILEMAP_LAYERS) {
    TraceLog(LOG_WARNING, "world::_get_tile_from_map_by_mouse_pos()::Recieved layer was out of bound");
    return tile();
  }
  return get_tile_from_map_by_mouse_pos(state->active_map, GetScreenToWorld2D(_mouse_pos, state->in_camera_metrics->handle), from_layer);
}
void _render_tile_on_pos(const tile* _tile, Vector2 pos,const tilesheet* sheet) {
  if (!_tile || !_tile->is_initialized || !sheet) {
    TraceLog(LOG_WARNING, "world::_render_tile()::One of pointers is not valid");
    return;
  }
  render_tile(&_tile->symbol, Rectangle {pos.x, pos.y, (f32) state->active_map->tile_size, (f32) state->active_map->tile_size }, sheet);
}
// EXPOSED
