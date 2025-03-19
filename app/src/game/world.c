#include "world.h"
#include <core/fmemory.h>

#include "tilemap.h"

typedef struct world_system_state {
  tilemap map[MAX_WORLDMAP_LOCATIONS];
  tilemap_stringtify_package map_stringtify[MAX_WORLDMAP_LOCATIONS];
  worldmap_stage worldmap_locations[MAX_WORLDMAP_LOCATIONS];
  tilesheet palette;

  worldmap_stage active_stage;
  f32 palette_zoom;
  camera_metrics* in_camera_metrics;
} world_system_state;

static world_system_state *restrict state; 

#define CURR_MAP state->map[state->active_stage.map_id]

Rectangle get_camera_view_rect(Camera2D camera);
Rectangle get_position_view_rect(Camera2D camera, Vector2 pos, f32 zoom);

bool world_system_initialize(camera_metrics* _in_camera_metrics, Vector2 resolution_div2) {
  if (state) {
    TraceLog(LOG_WARNING, "world::world_system_initialize()::Initialize called twice");
    return false;
  }
  state = (world_system_state*)allocate_memory_linear(sizeof(world_system_state), true);
  state->in_camera_metrics = _in_camera_metrics;

  { // WORLD LOCATIONS
    state->worldmap_locations[0] = (worldmap_stage) {
      .displayname = "Stage 1",
      .filename    = "stage1",
      .screen_location = NORMALIZE_VEC2(548, 1914, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 0,
      .is_active = true
    };
    state->worldmap_locations[1] = (worldmap_stage) {
      .displayname = "Stage 2",
      .filename    = "stage2",
      .screen_location = NORMALIZE_VEC2(1024, 1744, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 1,
      .is_active = true
    };
    state->worldmap_locations[2] = (worldmap_stage) {
      .displayname = "Stage 3",
      .filename    = "stage3",
      .screen_location = NORMALIZE_VEC2(1467, 1755, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 2,
      .is_active = true
    };
    state->worldmap_locations[3] = (worldmap_stage) {
      .displayname = "Stage 4",
      .filename    = "stage4",
      .screen_location = NORMALIZE_VEC2(1674, 1727, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 3,
      .is_active = true
    };
    state->worldmap_locations[4] = (worldmap_stage) {
      .displayname = "Stage 5",
      .filename    = "stage5",
      .screen_location = NORMALIZE_VEC2(2179, 1623, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 4,
      .is_active = true
    };
    state->worldmap_locations[5] = (worldmap_stage) {
      .displayname = "Stage 6",
      .filename    = "stage6",
      .screen_location = NORMALIZE_VEC2(2233, 1240, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 5,
      .is_active = true
    };
    state->worldmap_locations[6] = (worldmap_stage) {
      .displayname = "Stage 7",
      .filename    = "stage7",
      .screen_location = NORMALIZE_VEC2(1811, 1230, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 6,
      .is_active = true
    };
    state->worldmap_locations[7] = (worldmap_stage) {
      .displayname = "Stage 8",
      .filename    = "stage8",
      .screen_location = NORMALIZE_VEC2(2658, 1202, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 7,
      .is_active = true
    };
    state->worldmap_locations[8] = (worldmap_stage) {
      .displayname = "Stage 9",
      .filename    = "stage9",
      .screen_location = NORMALIZE_VEC2(3009, 1511, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 8,
      .is_active = true
    };
    state->worldmap_locations[9] = (worldmap_stage) {
      .displayname = "Stage 10",
      .filename    = "stage10",
      .screen_location = NORMALIZE_VEC2(2767, 1972, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 9,
      .is_active = true
    };
    state->worldmap_locations[10] = (worldmap_stage) {
      .displayname = "Stage 11",
      .filename    = "stage11",
      .screen_location = NORMALIZE_VEC2(3188, 1415, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 10,
      .is_active = true
    };
    state->worldmap_locations[11] = (worldmap_stage) {
      .displayname = "Stage 12",
      .filename    = "stage12",
      .screen_location = NORMALIZE_VEC2(3449, 1773, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 11,
      .is_active = true
    };
    state->worldmap_locations[12] = (worldmap_stage) {
      .displayname = "Stage 13",
      .filename    = "stage13",
      .screen_location = NORMALIZE_VEC2(3073, 1146, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 12,
      .is_active = true
    };
    state->worldmap_locations[13] = (worldmap_stage) {
      .displayname = "Stage 14",
      .filename    = "stage14",
      .screen_location = NORMALIZE_VEC2(3574, 1291, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 13,
      .is_active = true
    };
    state->worldmap_locations[14] = (worldmap_stage) {
      .displayname = "Stage 15",
      .filename    = "stage15",
      .screen_location = NORMALIZE_VEC2(3462, 773, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 14,
      .is_active = true
    };
    state->worldmap_locations[15] = (worldmap_stage) {
      .displayname = "Stage 16",
      .filename    = "stage16",
      .screen_location = NORMALIZE_VEC2(2661, 951, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 15,
      .is_active = true
    };
    state->worldmap_locations[16] = (worldmap_stage) {
      .displayname = "Stage 17",
      .filename    = "stage17",
      .screen_location = NORMALIZE_VEC2(3548, 974, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 16,
      .is_active = true
    };
    state->worldmap_locations[17] = (worldmap_stage) {
      .displayname = "Stage 18",
      .filename    = "stage18",
      .screen_location = NORMALIZE_VEC2(2400, 596, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 17,
      .is_active = true
    };
    state->worldmap_locations[18] = (worldmap_stage) {
      .displayname = "Stage 19",
      .filename    = "stage19",
      .screen_location = NORMALIZE_VEC2(3290, 369, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 18,
      .is_active = true
    };
    state->worldmap_locations[19] = (worldmap_stage) {
      .displayname = "Stage 20",
      .filename    = "stage20",
      .screen_location = NORMALIZE_VEC2(3449, 224, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 19,
      .is_active = true
    };
    state->worldmap_locations[20] = (worldmap_stage) {
      .displayname = "Stage 21",
      .filename    = "stage21",
      .screen_location = NORMALIZE_VEC2(1977, 385, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 20,
      .is_active = true
    };
    state->worldmap_locations[21] = (worldmap_stage) {
      .displayname = "Stage 22",
      .filename    = "stage22",
      .screen_location = NORMALIZE_VEC2(1661, 410, 3840.f, 2160.f),
      .spawning_areas[0] = (Rectangle) {
        -100, 
        -100, 
        resolution_div2.x,
        resolution_div2.y
      },
      .map_id = 21,
      .is_active = true
    };
  }

  for (int i=0; i<MAX_WORLDMAP_LOCATIONS; ++i) {
    for (int j=0; j<MAX_TILEMAP_LAYERS; ++j) {
      copy_memory(state->map[i].filename[j], TextFormat("%s_layer%d.txt", state->worldmap_locations[i].filename, j), sizeof(i8) * MAX_TILEMAP_FILENAME_LEN);
      copy_memory(state->map[i].propfile,    TextFormat("%s_prop.txt", state->worldmap_locations[i].filename), sizeof(i8) * MAX_TILEMAP_FILENAME_LEN);
    }
    create_tilemap(TILESHEET_TYPE_MAP, (Vector2) {0, 0}, 100, 16*3, &state->map[i]);
    if(!state->map[i].is_initialized) {
      TraceLog(LOG_WARNING, "WARNING::scene_in_game_edit::initialize_scene_in_game_edit()::tilemap initialization failed");
    }
    load_or_create_map_data(&state->map[i], &state->map_stringtify[i]);

    if (!state->map_stringtify[i].is_success) {
      TraceLog(LOG_ERROR, "game_manager_initialize::game manager unable to load map");
      return false;
    }
  }
  create_tilesheet(TILESHEET_TYPE_MAP, 16*2, 1.1, &state->palette);
  if(!state->palette.is_initialized) {
    TraceLog(LOG_WARNING, "scene_in_game_edit::initialize_scene_in_game_edit()::palette initialization failed");
  }
  state->palette.position = (Vector2) {_in_camera_metrics->screen_offset.x, _in_camera_metrics->screen_offset.y + 50};

  return true;
}

worldmap_stage* get_worldmap_locations(void) {
  return state->worldmap_locations;
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
void set_map_tile(i32 layer, tilemap_tile* src, tilemap_tile* dst) {
  if (layer < 0 || layer >= MAX_TILEMAP_LAYERS) {
    TraceLog(LOG_WARNING, "world::set_map_tile()::Recieved layer was out of bound");
    return;
  }
  if (src->x >= MAX_TILEMAP_TILESLOT_X || src->y >= MAX_TILEMAP_TILESLOT_Y) {
    TraceLog(LOG_WARNING, "world::set_map_tile()::Recieved tile was out of bound");
    return;
  }
  CURR_MAP.tiles[layer][src->x][src->y] = *dst;
}
tilemap_prop* get_map_prop_by_pos(Vector2 pos) {
  for (int i=0; i<CURR_MAP.prop_count; ++i) {
    if(CheckCollisionPointRec(pos, CURR_MAP.props[i].dest)) {
      return &CURR_MAP.props[i];
      break;
    }
  }
  return (tilemap_prop*){0};
}
tilemap_prop* get_map_prop_by_id(u16 id) {
  for (int i=0; i<CURR_MAP.prop_count; ++i) {
    if (CURR_MAP.props[i].id == id) {
      return &CURR_MAP.props[i];
    }
  }
  TraceLog(LOG_WARNING, "world::get_map_prop_by_id()::No match found");
  return (tilemap_prop*){0};  
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
  render_tilemap(&CURR_MAP, get_camera_view_rect(state->in_camera_metrics->handle));
}
void render_map_view_on(Vector2 pos, f32 zoom) {
  render_tilemap(&CURR_MAP, get_position_view_rect(state->in_camera_metrics->handle, pos, zoom));
}

void render_map_palette(f32 zoom) {
  render_tilesheet(&state->palette, zoom);
  state->palette_zoom = zoom;
}

tilemap_tile _get_tile_from_sheet_by_mouse_pos() {
  return get_tile_from_sheet_by_mouse_pos(&state->palette, GetMousePosition(), state->palette_zoom);
}
tilemap_tile _get_tile_from_map_by_mouse_pos(u16 from_layer) {
  if (from_layer >= MAX_TILEMAP_LAYERS) {
    TraceLog(LOG_WARNING, "world::_get_tile_from_map_by_mouse_pos()::Recieved layer was out of bound");
    return (tilemap_tile) {0};
  }
  return get_tile_from_map_by_mouse_pos(&CURR_MAP, GetScreenToWorld2D(GetMousePosition(), state->in_camera_metrics->handle), from_layer);
}
void _render_tile(tilemap_tile* tile) {
  if (!tile) {
    TraceLog(LOG_WARNING, "world::_render_tile()::Recieved tile was empty");
    return;
  }

  render_tile(tile, 
    (Rectangle) {GetMousePosition().x, GetMousePosition().y, CURR_MAP.tile_size, CURR_MAP.tile_size});
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
      CURR_MAP.props[CURR_MAP.prop_count] = (tilemap_prop) {0};
      return true;
    }
  }
  TraceLog(LOG_WARNING, "world::remove_prop_cur_map_by_id()::No match found");
  return false;
}
Rectangle get_camera_view_rect(Camera2D camera) {
  // Get the current window dimensions
  int screen_width = GetScreenWidth();
  int screen_height = GetScreenHeight();
  
  // Calculate the camera's view dimensions, accounting for zoom
  float view_width = screen_width / camera.zoom;
  float view_height = screen_height / camera.zoom;
  
  // Calculate the top-left corner of the camera view in world space
  // Since camera.target is the center of the screen, we subtract half the view dimensions
  float x = camera.target.x;
  float y = camera.target.y;
  
  // Apply camera offset
  x -= camera.offset.x/camera.zoom;
  y -= camera.offset.y/camera.zoom;
  
  // Return the camera view rectangle in world coordinates
  return (Rectangle){ x, y, view_width, view_height };
}
Rectangle get_position_view_rect(Camera2D camera, Vector2 pos, f32 zoom) {
  int screen_width = GetScreenWidth();
  int screen_height = GetScreenHeight();
  
  float view_width = screen_width / zoom;
  float view_height = screen_height / zoom;
  
  float x = pos.x;
  float y = pos.y;
  
  x -= camera.offset.x/zoom;
  y -= camera.offset.y/zoom;
  
  return (Rectangle){ x, y, view_width, view_height };
}
