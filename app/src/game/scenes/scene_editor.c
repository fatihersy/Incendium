#include "scene_editor.h"
#include <defines.h>
#include <settings.h>

#include "core/event.h"
#include "core/fmemory.h"

#include "game/resource.h"
#include "game/tilemap.h"
#include "game/user_interface.h"

typedef enum editor_state_selection_type {
  SLC_TYPE_UNSELECTED,
  SLC_TYPE_TILE,
  SLC_TYPE_PROP,
} editor_state_selection_type;

typedef enum editor_state_mouse_focus {
  MOUSE_FOCUS_UNFOCUSED, // While window unfocused or not on the screen
  MOUSE_FOCUS_MAP,
  MOUSE_FOCUS_TILE_SELECTION,
  MOUSE_FOCUS_PROP_SELECTION,
} editor_state_mouse_focus;

typedef struct scene_editor_state {
  camera_metrics* in_camera_metrics;
  Vector2 target;
  tilemap map;
  tilesheet palette;
  tilemap_stringtify_package package;
  tilemap_prop props[MAX_TILESHEET_PROPS];
  u16 prop_count;

  bool b_show_tilesheet_tile_selection_screen;
  bool b_show_prop_selection_screen;

  panel prop_selection_panel;
  panel tile_selection_panel;
  tilemap_tile selected_tile;
  tilemap_prop selected_prop;
  editor_state_selection_type selection_type;
  editor_state_mouse_focus mouse_focus;
} scene_editor_state;

static scene_editor_state *state;

void editor_update_bindings();
void editor_update_movement();
void editor_update_zoom_controls();
void editor_update_mouse_bindings();
void editor_update_keyboard_bindings();

void add_prop(texture_id source_tex, Rectangle source, f32 scale);
#define add_prop_epic_cemetery(...) add_prop(TEX_ID_MAP_PROPS_ATLAS, __VA_ARGS__)      

void initialize_scene_editor(camera_metrics* _camera_metrics) {
  if (state) {
    TraceLog(LOG_ERROR, "ERROR::scene_in_game_edit::initialize_scene_editor()::initialize function called multiple times");
    return;
  }
  state = (scene_editor_state*)allocate_memory_linear(sizeof(scene_editor_state), true);
  
  create_tilemap(TILESHEET_TYPE_MAP, (Vector2) {0, 0}, 100, 16*3, WHITE, &state->map);
  if(!state->map.is_initialized) {
    TraceLog(LOG_WARNING, "scene_in_game_edit::initialize_scene_in_game_edit()::tilemap initialization failed");
  }
  create_tilesheet(TILESHEET_TYPE_MAP, 16*2, 1.1, &state->palette);
  if(!state->palette.is_initialized) {
    TraceLog(LOG_WARNING, "scene_in_game_edit::initialize_scene_in_game_edit()::palette initialization failed");
  }

  user_interface_system_initialize();
  state->tile_selection_panel = get_default_panel();
  state->tile_selection_panel.signal_state = BTN_STATE_HOVER;
  state->tile_selection_panel.dest = (Rectangle) {0, 0, get_resolution_div4()->x, GetScreenHeight()};
  state->prop_selection_panel = get_default_panel();
  state->prop_selection_panel.signal_state = BTN_STATE_HOVER;
  state->prop_selection_panel.dest = (Rectangle) {0, 0, get_resolution_div4()->x, GetScreenHeight()};

  state->in_camera_metrics = _camera_metrics;
  state->palette.position = (Vector2) {5, 5};
  event_fire(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, 0, (event_context){
    .data.f32[0] = state->target.x,
    .data.f32[1] = state->target.y
  });

  add_prop_epic_cemetery((Rectangle){ 11,  40,  166, 152}, 1);
  add_prop_epic_cemetery((Rectangle){ 193, 101, 122, 91 }, 1);
  add_prop_epic_cemetery((Rectangle){ 321, 43,  127, 149}, 1);
  add_prop_epic_cemetery((Rectangle){ 457, 107, 53 , 85 }, 1);
}

// UPDATE / RENDER
void update_scene_editor() {
  if(!IsWindowFocused()) {
    state->mouse_focus = MOUSE_FOCUS_UNFOCUSED;
    return;
  }
  else state->mouse_focus = MOUSE_FOCUS_MAP;

  editor_update_bindings();
  update_user_interface();
}
void render_scene_editor() {
  render_tilemap(&state->map);
  DrawPixel(0, 0, RED);
}
void render_interface_editor() {
  
  if(state->b_show_tilesheet_tile_selection_screen && !state->b_show_prop_selection_screen) 
  { 
    gui_panel_scissored(state->tile_selection_panel, false, {
      render_tilesheet(&state->palette, state->tile_selection_panel.zoom);
    });
  }
  else if(state->b_show_prop_selection_screen && !state->b_show_tilesheet_tile_selection_screen) 
  {     
    gui_panel_scissored(state->prop_selection_panel, false, {
      u16 prop_height_count = 0;
      for (int i=0; i<state->prop_count; ++i) {
        tilemap_prop* prop = &state->props[i];
        Rectangle dest = prop->dest;
        dest.x = state->prop_selection_panel.scroll;
        dest.y = prop_height_count;
        gui_draw_texture_id_pro(prop->atlas_id, prop->source, dest);
        prop_height_count += prop->dest.height;
      }
    });
  }

  switch (state->selection_type) {
    case SLC_TYPE_TILE: {
      render_tile(state->selected_tile, 
      (Rectangle) {
        GetMousePosition().x, GetMousePosition().y,
        state->map.tile_size,
        state->map.tile_size
      });
      break;
    }
    case SLC_TYPE_PROP: {
      gui_draw_texture_id_pro(
        state->selected_prop.atlas_id, 
        state->selected_prop.source,
        (Rectangle) 
        {
          GetMousePosition().x, GetMousePosition().y, 
          state->selected_prop.dest.width, state->selected_prop.dest.height
        });
      break;
    }
    default: break;
  } 

  render_user_interface();
}
// UPDATE / RENDER


void add_prop(texture_id source_tex, Rectangle source, f32 scale) {
  if (source_tex >= TEX_ID_MAX || source_tex <= TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "scene_editor::add_prop()::Provided texture id out of bound");
    return;
  }
  Texture2D* tex = get_texture_by_enum(source_tex);
  if (!tex || tex->id == 0 || tex->id == U32_MAX) {
    TraceLog(LOG_WARNING, "scene_editor::add_prop()::Provided texture uninitialized or corrupted");
    return;
  }
  if (tex->width < source.x + source.width || tex->height < source.y + source.height) {
    TraceLog(LOG_WARNING, "scene_editor::add_prop()::Provided prop dimentions out of bound");
    return;
  }
  tilemap_prop* prop = &state->props[state->prop_count];
  
  prop->atlas_id = source_tex;
  prop->source = source;
  prop->dest = (Rectangle) {0, 0, source.width * scale, source.height * scale};
  prop->id = state->prop_count;
  prop->is_initialized = true;
  ++state->prop_count;
}


// BINDINGS
void editor_update_bindings() {
  editor_update_mouse_bindings();
  editor_update_keyboard_bindings();
}
void editor_update_mouse_bindings() { 
  editor_update_zoom_controls();

  if(state->b_show_tilesheet_tile_selection_screen && CheckCollisionPointRec(GetMousePosition(), state->tile_selection_panel.dest))
  {
    state->mouse_focus = MOUSE_FOCUS_TILE_SELECTION;
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->selection_type == SLC_TYPE_UNSELECTED) {
      tilemap_tile tile = get_tile_from_sheet_by_mouse_pos(&state->palette, GetMousePosition(), state->tile_selection_panel.zoom);
      if (tile.is_initialized) {
        state->selected_tile = tile;
        state->selection_type = SLC_TYPE_TILE;
      }
    }
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
      state->palette.position.x += GetMouseDelta().x;
      state->palette.position.y += GetMouseDelta().y;
    }
  }
  if(state->b_show_prop_selection_screen && CheckCollisionPointRec(GetMousePosition(), state->prop_selection_panel.dest))
  {
    if (state->mouse_focus == MOUSE_FOCUS_TILE_SELECTION) {
      TraceLog(LOG_ERROR, "scene_editor::editor_update_mouse_bindings()::tile and prop screen activated at the same time.");
      return;
    } 
    else state->mouse_focus = MOUSE_FOCUS_PROP_SELECTION;
  
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->selection_type == SLC_TYPE_UNSELECTED) {
      i32 h = state->prop_selection_panel.scroll + GetMousePosition().y;
      for (int i=0; i<state->prop_count; ++i) {
        if(h - state->props[i].source.height < 0) {
          state->selected_prop = state->props[i];
          state->selection_type = SLC_TYPE_PROP;
          break;
        }
        h -= state->props[i].source.height;
      }
    }
  }
  if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->mouse_focus == MOUSE_FOCUS_MAP) {
    switch (state->selection_type) {
      case SLC_TYPE_TILE: { 
        tilemap_tile tile = get_tile_from_map_by_mouse_pos(&state->map, GetScreenToWorld2D(GetMousePosition(), state->in_camera_metrics->handle));
        state->map.tiles[tile.x][tile.y] = state->selected_tile;
        TraceLog(LOG_INFO, "tile.%d,tile.%d is %d:%d", tile.x, tile.y, state->selected_tile.tile_symbol.c[0], state->selected_tile.tile_symbol.c[1]);
        break; 
      }
      case SLC_TYPE_PROP: {
        Vector2 coord = GetScreenToWorld2D(GetMousePosition(), state->in_camera_metrics->handle);
        state->selected_prop.dest.x = coord.x;
        state->selected_prop.dest.y = coord.y;
        state->map.props[state->map.prop_count++] = state->selected_prop;
        break;
      }
      default: break;
    }
  };
  
  
  if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && state->selection_type != SLC_TYPE_UNSELECTED) {
    state->selected_tile = (tilemap_tile) {0};
    state->selected_prop = (tilemap_prop) {0};
    state->selection_type = SLC_TYPE_UNSELECTED;
  }
}
void editor_update_keyboard_bindings() {
  editor_update_movement();

  if (IsKeyReleased(KEY_ESCAPE)) {
    event_fire(EVENT_CODE_UI_SHOW_PAUSE_MENU, 0, (event_context){0});
  }
  if (IsKeyPressed(KEY_F5)) {
    save_map_data(&state->map, &state->package);
  }
  if (IsKeyPressed(KEY_F8)) {
    load_map_data(&state->map, &state->package);
  }
  if (IsKeyReleased(KEY_TAB)) {
    state->b_show_tilesheet_tile_selection_screen = !state->b_show_tilesheet_tile_selection_screen;
    state->b_show_prop_selection_screen = false;
  }
  if (IsKeyPressed(KEY_I)) {
    state->b_show_prop_selection_screen = !state->b_show_prop_selection_screen;
    state->b_show_tilesheet_tile_selection_screen = false;
  }

}
void editor_update_zoom_controls() {
  if(!state->b_show_tilesheet_tile_selection_screen){
    state->in_camera_metrics->handle.zoom += ((float)GetMouseWheelMove()*0.05f);

    if (state->in_camera_metrics->handle.zoom > 3.0f) state->in_camera_metrics->handle.zoom = 3.0f;
    else if (state->in_camera_metrics->handle.zoom < 0.1f) state->in_camera_metrics->handle.zoom = 0.1f;
  }
  else {
    state->tile_selection_panel.zoom += ((float)GetMouseWheelMove()*0.05f);

    if (state->tile_selection_panel.zoom > 3.0f) state->tile_selection_panel.zoom = 3.0f;
    else if (state->tile_selection_panel.zoom < 0.1f) state->tile_selection_panel.zoom = 0.1f;
  }
}
void editor_update_movement() {
  f32 speed = 10;

  if (IsKeyDown(KEY_W)) {
    state->target.y -= speed;
    event_context context = (event_context)
    {
      .data.f32[0] = state->target.x, 
      .data.f32[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, context);
  }
  if (IsKeyDown(KEY_A)) {
    state->target.x -= speed;
    event_context context = (event_context)
    {
      .data.f32[0] = state->target.x, 
      .data.f32[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, context);
  }
  if (IsKeyDown(KEY_S)) {
    state->target.y += speed;
    event_context context = (event_context)
    {
      .data.f32[0] = state->target.x, 
      .data.f32[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, context);
  }
  if (IsKeyDown(KEY_D)) {
    state->target.x += speed;
    event_context context = (event_context)
    {
      .data.f32[0] = state->target.x, 
      .data.f32[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, context);
  }
}
// BINDINGS
