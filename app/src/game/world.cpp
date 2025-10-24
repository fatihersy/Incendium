#include "world.h"
#include <algorithm>
#include <loc_types.h>

#include <core/fmemory.h>
#include <core/logger.h>

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

void create_worldmap_stage(i32 in_map_id, i32 in_title_txt_id, std::string filename, i32 in_stage_level, f32 duration,
  i32 total_spawn_count, i32 total_boss_count, f32 spawn_scale, f32 boss_scale, Vector2 in_screen_location, bool is_display_on_screen, bool is_active) 
{
  state->worldmap_locations.at(in_map_id) = worldmap_stage(
    in_map_id, in_title_txt_id, filename, in_stage_level, duration, total_spawn_count, total_boss_count, spawn_scale, boss_scale, in_screen_location, true, is_display_on_screen, is_active
  );
}

bool world_system_initialize(const app_settings *const _in_app_settings) {
  if (state and state != nullptr) {
    return true;
  }
  state = (world_system_state*)allocate_memory_linear(sizeof(world_system_state), true);
  if (not state or state == nullptr) {
    IERROR("world::world_system_initialize()::State allocation failed");
    return false;
  }
  //*state = world_system_state(); // WARN: State is a total of 7mb of struct. So do not put it into stack

  if (not _in_app_settings or _in_app_settings == nullptr) {
    IERROR("world::world_system_initialize()::Recieved pointer(s) is/are null");
    return false;
  }
  state->in_app_settings = _in_app_settings;
  create_tilesheet(TILESHEET_TYPE_MAP, 16u*2u, 1.1f, __builtin_addressof(state->palette));
  if(not state->palette.is_initialized) {
    IWARN("world::world_system_initialize()::palette initialization failed");
    return false;
  }
  state->palette.position = Vector2 {0.f, 100.f};

  { // WORLD LOCATIONS
    i32 map_id_counter = 0;
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),          "stage1", 1, 300.f,   0, 0, 1, .5f, VECTOR2(   0,    0), false, false); // main menu background
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_WORLD_STAGE_1_TITLE),"stage2", 1, 90.f, 100, 1, 1, .5f, VECTOR2(1024, 1744),  true,  true);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),          "stage3", 1, 300.f,  75, 1, 1, .5f, VECTOR2(1467, 1755),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),          "stage4", 1, 300.f,  75, 1, 1, .5f, VECTOR2(1674, 1727),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),          "stage5", 1, 300.f,  75, 1, 1, .5f, VECTOR2(2179, 1623),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),          "stage6", 1, 300.f,  75, 1, 1, .5f, VECTOR2(1811, 1230),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),          "stage7", 1, 300.f,  75, 1, 1, .5f, VECTOR2(2233, 1240),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),          "stage8", 1, 300.f,  75, 1, 1, .5f, VECTOR2(2658, 1202),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),          "stage9", 1, 300.f,  75, 1, 1, .5f, VECTOR2(3009, 1511),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),         "stage10", 1, 300.f,  75, 1, 1, .5f, VECTOR2(2767, 1972),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),         "stage11", 1, 300.f,  75, 1, 1, .5f, VECTOR2(3188, 1415),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),         "stage12", 1, 300.f,  75, 1, 1, .5f, VECTOR2(3449, 1773),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),         "stage13", 1, 300.f,  75, 1, 1, .5f, VECTOR2(3073, 1146),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),         "stage14", 1, 300.f,  75, 1, 1, .5f, VECTOR2(3574, 1291),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),         "stage15", 1, 300.f,  75, 1, 1, .5f, VECTOR2(3462,  773),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),         "stage16", 1, 300.f,  75, 1, 1, .5f, VECTOR2(2661,  951),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),         "stage17", 1, 300.f,  75, 1, 1, .5f, VECTOR2(3548,  974),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),         "stage18", 1, 300.f,  75, 1, 1, .5f, VECTOR2(2400,  596),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),         "stage19", 1, 300.f,  75, 1, 1, .5f, VECTOR2(3290,  369),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),         "stage20", 1, 300.f,  75, 1, 1, .5f, VECTOR2(3449,  224),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),         "stage21", 1, 300.f,  75, 1, 1, .5f, VECTOR2(1977,  385),  true, false);
    create_worldmap_stage(map_id_counter++, static_cast<i32>(LOC_TEXT_UNDEFINED),         "stage22", 1, 300.f,  75, 1, 1, .5f, VECTOR2(1661,  410),  true, false);
  }
  for (size_t itr_000 = 0u; itr_000 < MAX_WORLDMAP_LOCATIONS; ++itr_000) 
  {
    for (size_t itr_111 = 0u; itr_111 < MAX_TILEMAP_LAYERS; ++itr_111) {
      state->map.at(itr_000).filename.at(itr_111) = TextFormat("%s_layer%d.txt", state->worldmap_locations.at(itr_000).filename.c_str(), itr_111);
    }
    state->map.at(itr_000).index = itr_000;
    state->map.at(itr_000).propfile = TextFormat("%s_prop.txt", state->worldmap_locations.at(itr_000).filename.c_str());
    state->map.at(itr_000).collisionfile = TextFormat("%s_collision.txt", state->worldmap_locations.at(itr_000).filename.c_str());
    if (not create_tilemap(TILESHEET_TYPE_MAP, ZEROVEC2, 100, 60, __builtin_addressof(state->map.at(itr_000)))) {
      IWARN("world::world_system_initialize()::tilemap initialization failed");
      continue;
    }
    if(not state->map.at(itr_000).is_initialized) {
      IWARN("world::world_system_initialize()::tilemap initialization failed");
      continue;
    }

    if(not load_or_create_map_data(__builtin_addressof(state->map.at(itr_000)), __builtin_addressof(state->map_stringtify.at(itr_000)))) {
      IWARN("world::world_system_initialize()::Stage:%d creation failed", itr_000);
      continue;
    }
    if (not state->map_stringtify.at(itr_000).is_success) {
      IWARN("world::world_system_initialize::Stage:%d read failed", itr_000);
      continue;
    }
    state->worldmap_locations.at(itr_000).spawning_areas.at(0u) = Rectangle { 
      state->map.at(itr_000).position.x, 
      state->map.at(itr_000).position.y,
      static_cast<f32>((state->map.at(itr_000).map_dim * state->map.at(itr_000).tile_size)), 
      static_cast<f32>((state->map.at(itr_000).map_dim * state->map.at(itr_000).tile_size))
    };
    refresh_render_queue(itr_000);
  }
  return true;
}
bool world_system_begin(const camera_metrics *const _in_camera_metrics) {
  if (not state or state == nullptr) {
    IERROR("world::world_system_initialize()::State allocation failed");
    return false;
  }
  if (not _in_camera_metrics or _in_camera_metrics == nullptr) {
    IERROR("world::world_system_initialize()::Recieved pointer(s) is/are null");
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
  if (id < 0 or id >= MAX_WORLDMAP_LOCATIONS) {
    IWARN("world::set_worldmap_location()::Worldmap id is out of bound");
    return;
  }
  state->active_map_stage = state->worldmap_locations.at(id);
  state->active_map = __builtin_addressof(state->map.at(state->active_map_stage.map_id));
}
void set_map_tile(i32 layer, tile src, tile dst) {
  if (layer < 0 or layer >= MAX_TILEMAP_LAYERS) {
    IWARN("world::set_map_tile()::Layer is out of bound");
    return;
  }
  if (src.position.x >= MAX_TILEMAP_TILESLOT_X or src.position.x >= MAX_TILEMAP_TILESLOT_Y) {
    IWARN("world::set_map_tile()::Tile is out of bound");
    return;
  }
  state->active_map->tiles[layer][src.position.x][src.position.y].c[0] = dst.symbol.c[0];
  state->active_map->tiles[layer][src.position.x][src.position.y].c[1] = dst.symbol.c[1];
}
tilemap_prop_address get_map_prop_by_pos(Vector2 pos) {
  if (not state or state == nullptr) {
    IERROR("world::get_map_prop_by_pos()::State is not valid");
    return tilemap_prop_address();
  }
  for (size_t itr_000 = 0u; itr_000 < state->active_map->static_props.size(); ++itr_000) {
    Rectangle prop_dest = state->active_map->static_props.at(itr_000).dest;
    prop_dest.x -= prop_dest.width  * .5f;
    prop_dest.y -= prop_dest.height * .5f;
    if(CheckCollisionPointRec(pos, prop_dest)) { // Props are always centered
      tilemap_prop_address prop = tilemap_prop_address(__builtin_addressof(state->active_map->static_props.at(itr_000)));
      prop.type = prop.data.prop_static->prop_type;
      return prop;
    }
  }
  for (size_t itr_000 = 0u; itr_000 < state->active_map->sprite_props.size(); ++itr_000) {
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
  if (not state or state == nullptr) {
    IERROR("world::get_map_collision_by_pos()::State is not valid");
    return nullptr;
  }
  for (size_t itr_000 = 0u; itr_000 < state->active_map->collisions.size(); ++itr_000) {
    Rectangle coll_dest = state->active_map->collisions.at(itr_000).dest;
    if(CheckCollisionPointRec(pos, coll_dest)) {
      return __builtin_addressof(state->active_map->collisions.at(itr_000));;
    }
  }
  return nullptr;
}
constexpr size_t get_renderqueue_prop_index_by_id(i16 zindex, i32 map_id) {
  if (not state or state == nullptr) {
    IERROR("world::get_renderqueue_prop_index_by_id()::State is not valid");
    return INVALID_IDU32;
  }
  const std::vector<tilemap_prop_address>& _queue = state->active_map->render_z_index_queue.at(zindex);
  for (size_t itr_000 = 0u; itr_000 < _queue.size(); ++itr_000) {
    const tilemap_prop_address *const _queue_elm = __builtin_addressof(_queue.at(itr_000));
    if (_queue_elm->type <= TILEMAP_PROP_TYPE_UNDEFINED or _queue_elm->type >= TILEMAP_PROP_TYPE_MAX) {
      continue;
    }
    
    if (_queue_elm->type == TILEMAP_PROP_TYPE_SPRITE and _queue_elm->data.prop_sprite != nullptr and _queue_elm->data.prop_sprite->map_id == map_id) return itr_000; 
    if (_queue_elm->type != TILEMAP_PROP_TYPE_SPRITE and _queue_elm->data.prop_static != nullptr and _queue_elm->data.prop_static->map_id == map_id) return itr_000;
  }
  return INVALID_IDU32;
}
tilemap_prop_static* get_map_prop_static_by_id(i32 map_id) {
  for (size_t iter = 0u; iter < state->active_map->static_props.size(); ++iter) {
    if (state->active_map->static_props.at(iter).map_id == map_id) {
      return __builtin_addressof(state->active_map->static_props.at(iter));
    }
  }
  IWARN("world::get_map_prop_static_by_id()::No match found");
  return nullptr;  
}
tilemap_prop_sprite* get_map_prop_sprite_by_id(i32 map_id) {
  for (size_t iter = 0u; iter < state->active_map->sprite_props.size(); ++iter) {
    if (state->active_map->sprite_props.at(iter).map_id == map_id) {
      return __builtin_addressof(state->active_map->sprite_props.at(iter));
    }
  }
  IWARN("world::get_map_prop_sprite_by_id()::No match found");
  return nullptr;  
}
const map_collision* get_map_collision_by_id(i32 coll_id) {
  if (not state or state == nullptr) {
    IERROR("world::get_map_collision_by_id()::State is not valid");
    return nullptr;
  }
  for (size_t itr_000 = 0u; itr_000 < state->active_map->collisions.size(); ++itr_000) {
    if (state->active_map->collisions.at(itr_000).coll_id == coll_id) {
      return __builtin_addressof(state->active_map->collisions.at(itr_000));
    }
  }
  return nullptr;
}

void save_current_map(void) {
  #ifndef _RELEASE
    if(not save_map_data(state->active_map, &state->map_stringtify.at(state->active_map_stage.map_id))) {
      IWARN("world::save_current_map()::save_map_data returned false");
    }
  #endif
}
void load_current_map(void) {
  if(!load_map_data(state->active_map, &state->map_stringtify.at(state->active_map_stage.map_id))) {
    IWARN("world::load_current_map()::load_map_data returned false");
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
  render_tilesheet(__builtin_addressof(state->palette), zoom);
  state->palette_zoom = zoom;
}

bool add_prop_curr_map(tilemap_prop_static prop_static) {
  if (not prop_static.is_initialized) {
    IWARN("world::add_prop_curr_map()::Static prop is not initialized");
    return false;
  }
  prop_static.map_id = state->active_map->next_map_id++;
  state->active_map->static_props.push_back(prop_static);
  
  refresh_render_queue(state->active_map_stage.map_id);
  return true;
}
bool add_prop_curr_map(tilemap_prop_sprite prop_sprite) {
  if (not prop_sprite.is_initialized) {
    IWARN("world::add_prop_curr_map()::Sprite prop is not initialized");
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
    in_collision.x > state->active_map->position.x or
    in_collision.y > state->active_map->position.y or
    in_collision.x + in_collision.width  < map_extent or
    in_collision.y + in_collision.height < map_extent;
  if (not in_bounds) {
    IWARN("world::add_map_coll_curr_map()::Collision out of the bounds of map");
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
    IWARN("world::remove_prop_cur_map_by_id()::No match found");
    return false;
  }
}
bool remove_map_collision_by_id(i32 coll_id) {
  if (not state or state == nullptr) {
    IERROR("world::remove_map_collision_by_id()::State is not valid");
    return false;
  }
  for (size_t itr_000 = 0u; itr_000 < state->active_map->collisions.size(); ++itr_000) {
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
  if (not state or state == nullptr) {
    IERROR("world::refresh_render_queue()::State is not valid");
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

  for (size_t static_itr_111 = 0u; static_itr_111 < static_prop_queue.size(); ++static_itr_111) {
    tilemap_prop_static *const map_static_ptr = __builtin_addressof(static_prop_queue.at(static_itr_111));

    if (map_static_ptr->prop_type <= TILEMAP_PROP_TYPE_UNDEFINED or map_static_ptr->prop_type >= TILEMAP_PROP_TYPE_MAX or map_static_ptr->prop_type == TILEMAP_PROP_TYPE_SPRITE) {
      static_prop_queue.erase(static_prop_queue.begin() + static_itr_111);
      continue;
    }

    if (map_static_ptr->use_y_based_zindex) {
      tilemap_ref.render_y_based_queue.at(map_static_ptr->zindex).push_back(tilemap_prop_address(map_static_ptr));
    }
    else if(map_static_ptr->zindex >= 0 and map_static_ptr->zindex < MAX_Z_INDEX_SLOT) {
      tilemap_ref.render_z_index_queue.at(map_static_ptr->zindex).push_back(tilemap_prop_address(map_static_ptr));
    }
    else {
      IWARN("world::refresh_render_queue()::Prop's Z-index is out of bound, set it to 0");
      map_static_ptr->zindex = 0;
      tilemap_ref.render_z_index_queue.at(0).push_back(tilemap_prop_address(map_static_ptr));
    }
  }

  for (size_t sprite_itr_111 = 0u; sprite_itr_111 < sprite_prop_queue.size(); ++sprite_itr_111) {
    tilemap_prop_sprite *const map_sprite_ptr = __builtin_addressof(sprite_prop_queue.at(sprite_itr_111));

    if (map_sprite_ptr->prop_type != TILEMAP_PROP_TYPE_SPRITE) {
      sprite_prop_queue.erase(sprite_prop_queue.begin() + sprite_itr_111);
      continue;
    }

    if (map_sprite_ptr->use_y_based_zindex) {
      tilemap_ref.render_y_based_queue.at(map_sprite_ptr->zindex).push_back(tilemap_prop_address(map_sprite_ptr));
    }
    else if(map_sprite_ptr->zindex >= 0 and map_sprite_ptr->zindex < MAX_Z_INDEX_SLOT) { 
      tilemap_ref.render_z_index_queue.at(map_sprite_ptr->zindex).push_back(tilemap_prop_address(map_sprite_ptr));
    }
    else {
      IWARN("world::refresh_render_queue()::Prop's Z-index is out of bound, set it to 0");
      map_sprite_ptr->zindex = 0;
      tilemap_ref.render_z_index_queue.at(0).push_back(tilemap_prop_address(map_sprite_ptr));
    }
  }
  sort_render_y_based_queue(id);
}
void sort_render_y_based_queue(i32 id) {
  if (not state or state == nullptr) {
    IERROR("world::refresh_render_queue()::State is not valid");
    return;
  }
  for (size_t itr_000 = 0u; itr_000 < MAX_Y_INDEX_SLOT; ++itr_000) {
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
  if (not state or state == nullptr) {
    IERROR("world::_sort_render_y_based_queue()::State is not valid");
    return;
  }
  sort_render_y_based_queue(state->active_map_stage.map_id);
}
Rectangle wld_calc_mainmenu_prop_dest(const tilemap * const _tilemap, Rectangle dest, f32 scale) {
  return calc_mainmenu_prop_dest(_tilemap, dest, scale, state->in_app_settings);
}

// EXPOSED
tile _get_tile_from_sheet_by_mouse_pos(Vector2 _mouse_pos) {
  return get_tile_from_sheet_by_mouse_pos(__builtin_addressof(state->palette), _mouse_pos, state->palette_zoom);
}
tile _get_tile_from_map_by_mouse_pos(i32 from_layer, Vector2 _mouse_pos) {
  if (from_layer >= MAX_TILEMAP_LAYERS) {
    IWARN("world::_get_tile_from_map_by_mouse_pos()::Layer is out of bound");
    return tile();
  }
  return get_tile_from_map_by_mouse_pos(state->active_map, GetScreenToWorld2D(_mouse_pos, state->in_camera_metrics->handle), from_layer);
}
void _render_tile_on_pos(const tile *const _tile, Vector2 pos, const tilesheet *const sheet) {
  if (not _tile or not _tile->is_initialized or not sheet) {
    IWARN("world::_render_tile()::Pointer(s) is/are invalid");
    return;
  }
  render_tile(__builtin_addressof(_tile->symbol), Rectangle {pos.x, pos.y, (f32) state->active_map->tile_size, (f32) state->active_map->tile_size }, sheet);
}
// EXPOSED
