#include "world.h"
#include <core/fmemory.h>

#include "tilemap.h"

typedef struct world_system_state {
  tilemap map[MAX_WORLDMAP_LOCATIONS];
  tilemap_stringtify_package map_stringtify[MAX_WORLDMAP_LOCATIONS];
  std::array<worldmap_stage, MAX_WORLDMAP_LOCATIONS> worldmap_locations;
  tilesheet palette;

  worldmap_stage active_stage;
  f32 palette_zoom;
  camera_metrics* in_camera_metrics;
} world_system_state;

static world_system_state * state; 

#define CURR_MAP state->map[state->active_stage.map_id]

// 3840x2160 is the dimentions of map selection screen background picture
#define CREATE_WORLDMAP_STAGE_CENTERED(ID, DISPLAY_NAME, FILENAME, SELECTION_SCREEN_LOCATION, WILL_BE_ON_SELECTION_SCREEN) state->worldmap_locations.at(ID) = \
  worldmap_stage {\
  .map_id = ID,\
  .displayname = DISPLAY_NAME,\
  .filename = FILENAME,\
  .screen_location = NORMALIZE_VEC2(SELECTION_SCREEN_LOCATION.x, SELECTION_SCREEN_LOCATION.y, 3840.f, 2160.f),\
  .is_centered = true,\
  .is_active = WILL_BE_ON_SELECTION_SCREEN,\
}

Rectangle get_position_view_rect(Camera2D camera, Vector2 pos, f32 zoom);

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
  state->palette.position = {_in_camera_metrics->screen_offset.x, _in_camera_metrics->screen_offset.y + 50};

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
  for (int i=0; i<MAX_WORLDMAP_LOCATIONS; ++i) {
    for (int j=0; j<MAX_TILEMAP_LAYERS; ++j) {
      copy_memory(state->map[i].filename[j], TextFormat("%s_layer%d.txt", state->worldmap_locations[i].filename.c_str(), j), sizeof(i8) * MAX_TILEMAP_FILENAME_LEN);
    }
    copy_memory(state->map[i].propfile,    TextFormat("%s_prop.txt", state->worldmap_locations[i].filename.c_str()), sizeof(i8) * MAX_TILEMAP_FILENAME_LEN);
    create_tilemap(TILESHEET_TYPE_MAP, Vector2 {}, 100, 60, &state->map[i]);
    if(!state->map[i].is_initialized) {
      TraceLog(LOG_WARNING, "WARNING::scene_in_game_edit::initialize_scene_in_game_edit()::tilemap initialization failed");
    }
    load_or_create_map_data(&state->map[i], &state->map_stringtify[i]);

    if (!state->map_stringtify[i].is_success) {
      TraceLog(LOG_ERROR, "game_manager_initialize::game manager unable to load map");
      return false;
    }
    state->worldmap_locations[i].spawning_areas[0] = {
      state->map[i].position.x, state->map[i].position.y,
      (f32) (state->map[i].map_dim * state->map[i].tile_size), (f32) (state->map[i].map_dim * state->map[i].tile_size)
    };
  }
  return true;
}

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
  state->active_stage = state->worldmap_locations[id];
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
tilemap_prop* get_map_prop_by_pos(Vector2 pos) {
  for (int i=0; i<CURR_MAP.prop_count; ++i) {
    Rectangle prop_dest = CURR_MAP.props[i].dest;
    prop_dest.x -= prop_dest.width  * .5f;
    prop_dest.y -= prop_dest.height * .5f;
    if(CheckCollisionPointRec(pos, prop_dest)) { // Props are always centered
      return &CURR_MAP.props[i];
      break;
    }
  }
  return nullptr;
}
tilemap_prop* get_map_prop_by_id(u16 id) {
  for (int i=0; i<CURR_MAP.prop_count; ++i) {
    if (CURR_MAP.props[i].id == id) {
      return &CURR_MAP.props[i];
    }
  }
  TraceLog(LOG_WARNING, "world::get_map_prop_by_id()::No match found");
  return nullptr;  
}

void save_current_map(void) {
  if(!save_map_data(&CURR_MAP, &state->map_stringtify[state->active_stage.map_id])) {
    TraceLog(LOG_WARNING, "world::save_current_map()::save_map_data returned false");
  }
}
void load_current_map(void) {
  if(!load_map_data(&CURR_MAP, &state->map_stringtify[state->active_stage.map_id])) {
    TraceLog(LOG_WARNING, "world::load_current_map()::load_map_data returned false");
  }
}

void update_map(void) {

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

tile _get_tile_from_sheet_by_mouse_pos(Vector2 _mouse_pos) {
  return get_tile_from_sheet_by_mouse_pos(&state->palette, _mouse_pos, state->palette_zoom);
}
tile _get_tile_from_map_by_mouse_pos(u16 from_layer, Vector2 _mouse_pos) {
  if (from_layer >= MAX_TILEMAP_LAYERS) {
    TraceLog(LOG_WARNING, "world::_get_tile_from_map_by_mouse_pos()::Recieved layer was out of bound");
    return tile {};
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
bool add_prop_curr_map(tilemap_prop* prop) {
  if (!prop) {
    TraceLog(LOG_WARNING, "world::add_prop_curr_map()::Recieved prop was empty");
    return false;
  }

  CURR_MAP.props[CURR_MAP.prop_count] = *prop;
  CURR_MAP.props[CURR_MAP.prop_count].id = CURR_MAP.prop_count;
  CURR_MAP.prop_count++;
  return true;
}
bool remove_prop_cur_map_by_id(u16 id) {
  if (id >= MAX_TILEMAP_PROPS) {
    TraceLog(LOG_WARNING, "world::remove_prop_cur_map_by_id()::Recieved id was out of bound");
    return false;
  }

  for (int i=0; i<CURR_MAP.prop_count; ++i) {
    if (CURR_MAP.props[i].id == id) {
      CURR_MAP.prop_count--;
      CURR_MAP.props[i] = CURR_MAP.props[CURR_MAP.prop_count];
      CURR_MAP.props[CURR_MAP.prop_count] = tilemap_prop{};
      return true;
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
