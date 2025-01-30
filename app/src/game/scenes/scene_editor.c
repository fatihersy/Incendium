#include "scene_editor.h"
#include <defines.h>
#include <settings.h>

#include "core/event.h"
#include "core/fmemory.h"

#include "game/resource.h"
#include "game/tilemap.h"
#include "game/user_interface.h"

typedef enum selection_type {
  SLC_TYPE_UNSELECTED,
  SLC_TYPE_TILE,
  SLC_TYPE_PROP
} selection_type;

typedef struct scene_editor_state {
  camera_metrics* in_camera_metrics;
  Vector2 target;
  tilemap map;
  tilesheet palette;
  tilemap_tile selected_tile;
  tilemap_prop selected_prop;
  tilemap_stringtify_package package;
  
  bool b_show_tilesheet_tile_selection_screen;
  bool b_show_prop_selection_screen;
  bool b_is_a_tile_selected;

  panel prop_selection_panel;
  panel tile_selection_panel;
  selection_type slc_type;
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
    TraceLog(LOG_WARNING, "WARNING::scene_in_game_edit::initialize_scene_in_game_edit()::tilemap initialization failed");
  }
  create_tilesheet(TILESHEET_TYPE_MAP, 16*2, 1.1, &state->palette);
  if(!state->palette.is_initialized) {
    TraceLog(LOG_WARNING, "WARNING::scene_in_game_edit::initialize_scene_in_game_edit()::palette initialization failed");
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

  add_prop_epic_cemetery((Rectangle){ 11,  40,  166, 152}, 1);
  add_prop_epic_cemetery((Rectangle){ 193, 101, 122, 91 }, 1);
  add_prop_epic_cemetery((Rectangle){ 321, 43,  127, 149}, 1);
  add_prop_epic_cemetery((Rectangle){ 457, 107, 53 , 85 }, 1);
}

// UPDATE / RENDER
void update_scene_editor() {
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
    Rectangle* dest = &state->tile_selection_panel.dest;
    if(gui_panel_active(&state->tile_selection_panel, *dest, false)) {
      
    }
    BeginScissorMode(dest->x, dest->y, dest->width, dest->height);
    render_tilesheet(&state->palette, state->tile_selection_panel.zoom);
    EndScissorMode();
  }
  if(state->b_show_prop_selection_screen && !state->b_show_tilesheet_tile_selection_screen) 
  {     
    Rectangle* dest = &state->prop_selection_panel.dest;
    if(gui_panel_active(&state->prop_selection_panel, *dest, false)) {
      
    }
    BeginScissorMode(dest->x, dest->y, dest->width, dest->height);
    u16 prop_height_count = 0;
    for (int i=0; i<state->map.prop_count; ++i) {
      tilemap_prop* prop = &state->map.props[i];
      Rectangle dest = (Rectangle) {
        .x = state->prop_selection_panel.scroll, .y = prop_height_count,
        .width = prop->dest.width, .height = prop->dest.height
      };
      gui_draw_texture_id_pro(prop->atlas_id, prop->source, dest);
      prop_height_count += prop->dest.height;
    }
    EndScissorMode();
  }

  if (state->b_is_a_tile_selected) {
    render_tile(state->selected_tile, 
    (tilemap_tile) 
    {
      .x = GetMousePosition().x,
      .y = GetMousePosition().y,
      .tile_size = state->map.tile_size
    });
  }

  render_user_interface();
}
// UPDATE / RENDER


void add_prop(texture_id source_tex, Rectangle source, f32 scale) {
  if (source_tex >= TEX_ID_MAX || source_tex <= TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "Provided texture id out of bound");
    return;
  }
  Texture2D* tex = get_texture_by_enum(source_tex);
  if (!tex || tex->id == 0 || tex->id == U32_MAX) {
    TraceLog(LOG_WARNING, "Provided texture uninitialized or corrupted");
    return;
  }
  if (tex->width < source.x + source.width || tex->height < source.y + source.height) {
    TraceLog(LOG_WARNING, "Provided prop dimentions out of bound");
    return;
  }
  tilemap_prop* prop = &state->map.props[state->map.prop_count];
  
  prop->atlas_id = source_tex;
  prop->source = source;
  prop->dest = (Rectangle) {0, 0, source.width * scale, source.height * scale};
  prop->id = state->map.prop_count;
  ++state->map.prop_count;
}


// BINDINGS
void editor_update_bindings() {
  editor_update_mouse_bindings();
  editor_update_keyboard_bindings();
}
void editor_update_mouse_bindings() { 
  editor_update_zoom_controls();

  if(state->b_show_tilesheet_tile_selection_screen)
  {
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && !state->b_is_a_tile_selected) {
      tilemap_tile tile = get_tile_from_sheet_by_mouse_pos(&state->palette, GetMousePosition(), state->tile_selection_panel.zoom);
      if (tile.is_initialized) {
        state->selected_tile = tile;
        state->b_is_a_tile_selected = true;
      }
    }
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
      state->palette.position.x += GetMouseDelta().x;
      state->palette.position.y += GetMouseDelta().y;
    }
  }
  else if(state->b_show_prop_selection_screen)
  {
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && !state->b_show_tilesheet_tile_selection_screen && state->b_show_prop_selection_screen) {
      i32 h = state->prop_selection_panel.scroll + GetMousePosition().y;
      for (int i=0; i<state->map.prop_count; ++i) {
        if(h - state->map.props[i].source.height > 0) {
          h -= state->map.props[i].source.height;
        }
      }
      
    }
  }
  else 
  {
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && !state->b_show_tilesheet_tile_selection_screen && state->b_is_a_tile_selected) {
      tilemap_tile tile = get_tile_from_map_by_mouse_pos(&state->map, GetScreenToWorld2D(GetMousePosition(), state->in_camera_metrics->handle));
      state->map.tiles[tile.x][tile.y] = state->selected_tile;
      TraceLog(LOG_INFO, "tile.%d,tile.%d is %d:%d", tile.x, tile.y, state->selected_tile.tile_symbol.c[0], state->selected_tile.tile_symbol.c[1]);
    };
  }

  if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && state->b_is_a_tile_selected) {
    tilemap_tile tile = (tilemap_tile) {0};
    state->b_is_a_tile_selected = false;
  }
}
void editor_update_keyboard_bindings() {
  editor_update_movement();

  if (IsKeyReleased(KEY_TAB)) {
    state->b_show_tilesheet_tile_selection_screen = !state->b_show_tilesheet_tile_selection_screen;
  }
  if (IsKeyReleased(KEY_ESCAPE)) {
    event_fire(EVENT_CODE_UI_SHOW_PAUSE_MENU, 0, (event_context){0});
  }
  if (IsKeyPressed(KEY_F5)) {
    save_map_data(&state->map, &state->package);
  }
  if (IsKeyPressed(KEY_I)) {
    state->b_show_prop_selection_screen = !state->b_show_prop_selection_screen;
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
