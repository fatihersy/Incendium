#include "scene_editor.h"
#include <defines.h>
#include <settings.h>

#include "core/event.h"
#include "core/fmemory.h"

#include "game/world.h"
#include "game/resource.h"
#include "game/user_interface.h"

#define PROP_DRAG_HANDLE_DIM 16
#define PROP_DRAG_HANDLE_DIM_DIV2 PROP_DRAG_HANDLE_DIM/2.f

typedef enum editor_state_selection_type {
  SLC_TYPE_UNSELECTED,
  SLC_TYPE_TILE,
  SLC_TYPE_DROP_PROP,
  SLC_TYPE_SLC_PROP,
} editor_state_selection_type;

typedef enum editor_state_mouse_focus {
  MOUSE_FOCUS_UNFOCUSED, // While window unfocused or not on the screen
  MOUSE_FOCUS_MAP,
  MOUSE_FOCUS_TILE_SELECTION,
  MOUSE_FOCUS_PROP_SELECTION,
} editor_state_mouse_focus;

typedef struct scene_editor_state {
  camera_metrics * in_camera_metrics;
  Vector2 target;
  worldmap_stage worldmap_locations[MAX_WORLDMAP_LOCATIONS];
  tilemap_prop props[MAX_TILESHEET_PROPS];
  u16 prop_count;

  bool b_show_tilesheet_tile_selection_screen;
  bool b_show_prop_selection_screen;
  bool b_dragging_prop;

  panel prop_selection_panel;
  panel tile_selection_panel;
  tile selected_tile;
  tilemap_prop selected_prop;
  tilesheet* selected_sheet;
  u16 edit_layer;
  u16 selected_stage;
  editor_state_selection_type selection_type;
  editor_state_mouse_focus mouse_focus;
  Vector2 mouse_pos_world;
  Vector2 mouse_pos_screen;
} scene_editor_state;

static scene_editor_state * state;

void begin_scene_editor(void);
void editor_update_bindings(void);
void editor_update_movement(void);
void editor_update_mouse_bindings(void);
void editor_update_keyboard_bindings(void);
i32 map_prop_id_to_index(u16 id);
Rectangle se_get_camera_view_rect(Camera2D camera);

void add_prop(atlas_texture_id source_tex, Rectangle source, f32 scale);
#define add_prop_epic_cemetery(...) add_prop(ATLAS_TEX_ID_MAP_PROPS_ATLAS, __VA_ARGS__)      

void initialize_scene_editor(camera_metrics* _camera_metrics) {
  if (state) {
    begin_scene_editor();
    return;
  }
  state = (scene_editor_state*)allocate_memory_linear(sizeof(scene_editor_state), true);

  if (!_camera_metrics) {
    TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::Camera metrics recieved NULL");
    return;
  }
  state->in_camera_metrics = _camera_metrics;
  world_system_initialize(_camera_metrics);

  user_interface_system_initialize();
  
  begin_scene_editor();
}

// UPDATE / RENDER
void update_scene_editor(void) {
  if(!IsWindowFocused()) {
    state->mouse_focus = MOUSE_FOCUS_UNFOCUSED;
    return;
  }
  else state->mouse_focus = MOUSE_FOCUS_MAP;
  state->mouse_pos_screen.x = GetMousePosition().x * get_app_settings()->scale_ratio.x;
  state->mouse_pos_screen.y = GetMousePosition().y * get_app_settings()->scale_ratio.y;
  state->mouse_pos_world = GetScreenToWorld2D(state->mouse_pos_screen, state->in_camera_metrics->handle);
  state->edit_layer = get_slider_current_value(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER)->data.u16[0]; // HACK: Should not updated every frame
  state->in_camera_metrics->frustum = se_get_camera_view_rect(state->in_camera_metrics->handle);

  editor_update_bindings();
  update_map();
  update_user_interface();
}
void render_scene_editor(void) {
  if (state->selected_stage == WORLDMAP_MAINMENU_MAP) {
    render_map_view_on(Vector2 {}, 1);
  }
  else {
    render_map();
  }

  DrawPixel(0, 0, RED);
}
void render_interface_editor(void) {
  
  if(state->b_show_tilesheet_tile_selection_screen && !state->b_show_prop_selection_screen) 
  { 
    gui_panel_scissored(state->tile_selection_panel, false, {
      render_map_palette(state->tile_selection_panel.zoom);
      gui_slider(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER, SCREEN_OFFSET, VECTOR2(4,3), 3.f);

      if(gui_slider_button(BTN_ID_EDITOR_BTN_STAGE_MAP_CHANGE_LEFT, SCREEN_POS(6.5f,9.f))){
        if (state->selected_stage > 0) {
          state->selected_stage -= 1;
          set_worldmap_location(state->selected_stage);
        }
      }

      gui_label_format_v(FONT_TYPE_MOOD, 10, SCREEN_POS(14.f,10.f), WHITE, true, true, "%d", state->selected_stage);

      if(gui_slider_button(BTN_ID_EDITOR_BTN_STAGE_MAP_CHANGE_RIGHT, SCREEN_POS(17.f,9.f))) {
        if (state->selected_stage < MAX_WORLDMAP_LOCATIONS-1) {
          state->selected_stage += 1;
          set_worldmap_location(state->selected_stage);
        }
      }
    }); 
  }
  else if(state->b_show_prop_selection_screen && !state->b_show_tilesheet_tile_selection_screen) 
  {
    panel* pnl = &state->prop_selection_panel;
    gui_panel_scissored((*pnl), false, {
      f32 prop_height_count = 0;
      for (int i = 0; i < state->prop_count; ++i) {
        const tilemap_prop* prop = &state->props[i];
        Rectangle dest = prop->dest;
        dest.x = 0;
        dest.y = (pnl->scroll * pnl->buffer.f32[0]) + prop_height_count;
        gui_draw_atlas_texture_id_pro(prop->atlas_id, prop->relative_source, dest, false, false);
        prop_height_count += prop->dest.height;
      }
      pnl->buffer.f32[0] = prop_height_count;
      pnl->scroll_handle.y = FMAX(pnl->scroll_handle.y, pnl->dest.y + SCREEN_OFFSET.x);
      pnl->scroll_handle.y = FMIN(pnl->scroll_handle.y, pnl->dest.y + pnl->dest.height);
      pnl->scroll = (pnl->scroll_handle.y - pnl->dest.y - SCREEN_OFFSET.x) / (pnl->dest.height - pnl->scroll_handle.height) * -1;
      DrawRectangleRec(pnl->scroll_handle, WHITE);
    });
  }

  switch (state->selection_type) {
    case SLC_TYPE_TILE: {
      _render_tile_on_pos(&state->selected_tile, state->mouse_pos_screen, state->selected_sheet);
      break;
    }
    case SLC_TYPE_DROP_PROP: {
      gui_draw_atlas_texture_id_pro(state->selected_prop.atlas_id, state->selected_prop.relative_source,
        Rectangle {
          state->mouse_pos_screen.x, state->mouse_pos_screen.y, 
          state->selected_prop.dest.width, state->selected_prop.dest.height
        }, false, false);
      break;
    }
    case SLC_TYPE_SLC_PROP: {
      Vector2 prop_pos = GetWorldToScreen2D(Vector2{state->selected_prop.dest.x, state->selected_prop.dest.y}, 
        state->in_camera_metrics->handle
      );
      f32 relative_width = state->selected_prop.dest.width * state->in_camera_metrics->handle.zoom;
      f32 relative_height = state->selected_prop.dest.height * state->in_camera_metrics->handle.zoom;
      DrawRectangleLines(
        prop_pos.x, prop_pos.y, 
        relative_width, 
        relative_height, 
        WHITE);
      prop_pos.x += relative_width / 2.f - PROP_DRAG_HANDLE_DIM_DIV2;
      prop_pos.y += relative_height / 2.f - PROP_DRAG_HANDLE_DIM_DIV2;
      DrawRectangle(prop_pos.x, prop_pos.y, PROP_DRAG_HANDLE_DIM, PROP_DRAG_HANDLE_DIM, WHITE);
      break;
    }
    default: break;
  } 

  render_user_interface();
}
// UPDATE / RENDER

void add_prop(atlas_texture_id source_tex, Rectangle source, f32 scale) {
  if (source_tex >= ATLAS_TEX_ID_MAX || source_tex <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "scene_editor::add_prop()::Provided texture id out of bound");
    return;
  }
  const atlas_texture* tex = get_atlas_texture_by_enum(source_tex);
  if (!tex || !tex->atlas_handle) {
    TraceLog(LOG_WARNING, "scene_editor::add_prop()::Invalid atlas");
    return;
  }
  if (tex->atlas_handle->width < source.x + source.width || tex->atlas_handle->height < source.y + source.height) {
    TraceLog(LOG_WARNING, "scene_editor::add_prop()::Provided prop dimentions out of bound");
    return;
  }
  tilemap_prop* prop = &state->props[state->prop_count];
  
  prop->atlas_id = source_tex;
  prop->source = source;
  prop->relative_source = Rectangle{ 
    prop->source.x + tex->source.x,  
    prop->source.y + tex->source.y,  
    prop->source.width,  
    prop->source.height
  };
  prop->dest = Rectangle {0, 0, source.width * scale, source.height * scale};
  prop->id = state->prop_count;
  prop->is_initialized = true;
  ++state->prop_count;
}

// BINDINGS
void editor_update_bindings(void) {
  editor_update_mouse_bindings();
  editor_update_keyboard_bindings();
}
void editor_update_mouse_bindings(void) { 
  if(state->b_show_tilesheet_tile_selection_screen && CheckCollisionPointRec(state->mouse_pos_screen, state->tile_selection_panel.dest))
  {
    state->mouse_focus = MOUSE_FOCUS_TILE_SELECTION;
    state->tile_selection_panel.zoom += ((float)GetMouseWheelMove()*0.05f);

    if (state->tile_selection_panel.zoom > 3.0f) state->tile_selection_panel.zoom = 3.0f;
    else if (state->tile_selection_panel.zoom < 0.1f) state->tile_selection_panel.zoom = 0.1f;
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->selection_type == SLC_TYPE_UNSELECTED) {
      tile _tile = _get_tile_from_sheet_by_mouse_pos(state->mouse_pos_screen);
      if (_tile.is_initialized) {
        state->selected_tile = _tile;
        state->selection_type = SLC_TYPE_TILE;
      } else {
        TraceLog(LOG_WARNING, "scene_editor::editor_update_mouse_bindings()::Tile selection failed");
        return;
      }
    }
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
      drag_tilesheet(GetMouseDelta());
    }
  }
  else if(state->b_show_prop_selection_screen && CheckCollisionPointRec(state->mouse_pos_screen, state->prop_selection_panel.dest))
  {
    if (state->mouse_focus == MOUSE_FOCUS_TILE_SELECTION) {
      TraceLog(LOG_ERROR, "scene_editor::editor_update_mouse_bindings()::tile and prop screen activated at the same time.");
      return;
    } 
    else state->mouse_focus = MOUSE_FOCUS_PROP_SELECTION;
    panel* pnl = &state->prop_selection_panel;

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && state->selection_type == SLC_TYPE_UNSELECTED && CheckCollisionPointRec(state->mouse_pos_screen, pnl->scroll_handle)) {
      pnl->is_dragging_scroll = true;
    }

    pnl->scroll_handle.y += GetMouseWheelMove() * -10.f;
 
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->selection_type == SLC_TYPE_UNSELECTED && !pnl->is_dragging_scroll) {
      i32 h = (pnl->scroll * pnl->buffer.f32[0] * (-1)) + state->mouse_pos_screen.y;
      for (int i=0; i<state->prop_count; ++i) {
        if(h - state->props[i].source.height < 0) {
          state->selected_prop = state->props[i];
          state->selection_type = SLC_TYPE_DROP_PROP;
          break;
        }
        h -= state->props[i].source.height;
      }
    }
  }
  else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->mouse_focus == MOUSE_FOCUS_MAP) 
  {
    switch (state->selection_type) {
      case SLC_TYPE_UNSELECTED: {
        tilemap_prop* prop = get_map_prop_by_pos(state->mouse_pos_world);
        if (prop) {
          state->selected_prop = *prop;
          state->selection_type = SLC_TYPE_SLC_PROP;
        }
        break;
      }
      case SLC_TYPE_DROP_PROP: {
        Vector2 coord = GetScreenToWorld2D(state->mouse_pos_screen, state->in_camera_metrics->handle);
        state->selected_prop.dest.x = coord.x;
        state->selected_prop.dest.y = coord.y;
        add_prop_curr_map(&state->selected_prop);
        break;
      }
      case SLC_TYPE_SLC_PROP: {
        state->b_dragging_prop = false;
        break;
      }     
      default: break;
    }
  }
  else if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && state->mouse_focus == MOUSE_FOCUS_MAP) 
  {
    switch (state->selection_type) {
      case SLC_TYPE_SLC_PROP: {
        tilemap_prop* prop = get_map_prop_by_id(state->selected_prop.id);
        Rectangle drag_handle = Rectangle {
          prop->dest.x + prop->dest.width/2.f - PROP_DRAG_HANDLE_DIM_DIV2, prop->dest.y + prop->dest.height/2.f - PROP_DRAG_HANDLE_DIM_DIV2, 
          PROP_DRAG_HANDLE_DIM, PROP_DRAG_HANDLE_DIM
        };
        if(!state->b_dragging_prop && CheckCollisionPointRec(state->mouse_pos_world, drag_handle)) {
          state->b_dragging_prop = true;
        }
        if (state->b_dragging_prop) {
          prop->dest.x = state->mouse_pos_world.x - prop->dest.width/2.f;
          prop->dest.y = state->mouse_pos_world.y - prop->dest.height/2.f;
          state->selected_prop.dest = prop->dest;
        }
        break;
      }
      case SLC_TYPE_TILE: { 
        tile _tile = _get_tile_from_map_by_mouse_pos(state->edit_layer, state->mouse_pos_screen);
        if (_tile.position.x < MAX_TILEMAP_TILESLOT_X || _tile.position.y < MAX_TILEMAP_TILESLOT_Y ) {
          set_map_tile(state->edit_layer, _tile, state->selected_tile);
        }
        break; 
      }
      default: break;
    }
  }
  if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) 
  {
    panel* pnl = &state->prop_selection_panel;
    if (state->prop_selection_panel.is_dragging_scroll && state->b_show_prop_selection_screen) {
      i32 mouse_pos_y = state->mouse_pos_screen.y;
      pnl->scroll_handle.y = mouse_pos_y - pnl->scroll_handle.height / 2.f;
    } else if(state->prop_selection_panel.is_dragging_scroll && !state->b_show_prop_selection_screen) state->prop_selection_panel.is_dragging_scroll = false;
  }
  if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) 
  {
    if (state->prop_selection_panel.is_dragging_scroll) {
      state->prop_selection_panel.is_dragging_scroll = false;
    }
  }
  if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && state->selection_type != SLC_TYPE_UNSELECTED) 
  {
    state->selected_tile = {};
    state->selected_prop = {};
    state->selection_type = SLC_TYPE_UNSELECTED;
  }
  if (state->mouse_focus == MOUSE_FOCUS_MAP) {
    state->in_camera_metrics->handle.zoom += ((float)GetMouseWheelMove()*0.05f);

    if (state->in_camera_metrics->handle.zoom > 3.0f) state->in_camera_metrics->handle.zoom = 3.0f;
    else if (state->in_camera_metrics->handle.zoom < 0.1f) state->in_camera_metrics->handle.zoom = 0.1f;
  }
}
void editor_update_keyboard_bindings(void) {
  editor_update_movement();

  if (IsKeyReleased(KEY_ESCAPE)) {
    event_fire(EVENT_CODE_UI_SHOW_PAUSE_MENU, event_context{});
  }
  if (IsKeyPressed(KEY_F5)) {
    save_current_map();
  }
  if (IsKeyPressed(KEY_F8)) {
    load_current_map();
  }
  if (IsKeyReleased(KEY_TAB)) {
    state->b_show_tilesheet_tile_selection_screen = !state->b_show_tilesheet_tile_selection_screen;
    state->b_show_prop_selection_screen = false;
  }
  if (IsKeyPressed(KEY_I)) {
    state->b_show_prop_selection_screen = !state->b_show_prop_selection_screen;
    state->b_show_tilesheet_tile_selection_screen = false;
  }
  if (IsKeyReleased(KEY_DELETE)) {
    if (state->mouse_focus == MOUSE_FOCUS_MAP && state->selection_type == SLC_TYPE_SLC_PROP && !state->b_dragging_prop) {
      if(!remove_prop_cur_map_by_id(state->selected_prop.id)) {
        TraceLog(LOG_WARNING, "scene_editor::editor_update_keyboard_bindings()::Removing property failed.");
      }
      state->selected_prop = tilemap_prop {};
      state->selection_type = SLC_TYPE_UNSELECTED;
    }
  }
  if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyReleased(KEY_D)) {
    TraceLog(LOG_INFO, "");
  }

}
void editor_update_movement(void) {
  f32 speed = 10;

  if (IsKeyDown(KEY_W)) {
    state->target.y -= speed;
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, event_context(data128(state->target.x, state->target.y)));
  }
  if (IsKeyDown(KEY_A)) {
    state->target.x -= speed;
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, event_context(data128(state->target.x, state->target.y)));
  }
  if (IsKeyDown(KEY_S)) {
    state->target.y += speed;
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, event_context(data128(state->target.x, state->target.y)));
  }
  if (IsKeyDown(KEY_D)) {
    state->target.x += speed;
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, event_context(data128(state->target.x, state->target.y)));
  }
}
// BINDINGS

void begin_scene_editor(void) {

  copy_memory(&state->worldmap_locations, get_worldmap_locations(), sizeof(state->worldmap_locations));
  state->tile_selection_panel = get_default_panel();
  state->tile_selection_panel.signal_state = BTN_STATE_HOVER;
  state->tile_selection_panel.dest = Rectangle {0, 0, BASE_RENDER_SCALE(.3f).x, BASE_RENDER_RES.y};
  state->selected_sheet = get_tilesheet_by_enum(TILESHEET_TYPE_MAP);
  panel* prop_pnl = &state->prop_selection_panel;
  *prop_pnl = get_default_panel();
  prop_pnl->signal_state = BTN_STATE_HOVER;
  prop_pnl->dest = Rectangle {0, 0, BASE_RENDER_SCALE(.3f).x, BASE_RENDER_RES.y};
  prop_pnl->scroll_handle = Rectangle{
    .x = prop_pnl->dest.x + prop_pnl->dest.width - PROP_DRAG_HANDLE_DIM - 10, .y = 0,
    .width = PROP_DRAG_HANDLE_DIM, .height = PROP_DRAG_HANDLE_DIM * 5,
  };

  event_fire(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, event_context(data128(state->target.x, state->target.y)));

  // Trees
  {
    add_prop_epic_cemetery(Rectangle{  11,  40, 166, 152}, 1);
    add_prop_epic_cemetery(Rectangle{ 193, 101, 122,  91}, 1);
    add_prop_epic_cemetery(Rectangle{ 321,  43, 127, 149}, 1);
    add_prop_epic_cemetery(Rectangle{ 457, 107,  53,  85}, 1);
    add_prop_epic_cemetery(Rectangle{ 523,  40, 166, 152}, 1);
    add_prop_epic_cemetery(Rectangle{ 705, 101, 122,  91}, 1);
    add_prop_epic_cemetery(Rectangle{ 833,  43, 127, 149}, 1);
    add_prop_epic_cemetery(Rectangle{ 969, 107,  53,  85}, 1);
    add_prop_epic_cemetery(Rectangle{ 197, 192,  84,  62}, 1);
    add_prop_epic_cemetery(Rectangle{ 289, 193,  64,  64}, 1);
    add_prop_epic_cemetery(Rectangle{ 353, 193,  64,  64}, 1);
    add_prop_epic_cemetery(Rectangle{ 864, 192,  64,  64}, 1);
    add_prop_epic_cemetery(Rectangle{ 288, 256,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{ 320, 256,  64,  32}, 1);
    add_prop_epic_cemetery(Rectangle{ 384, 256,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{ 416, 256,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{ 289, 288,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{ 336, 288,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{ 384, 288,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{ 416, 288,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{  14, 208, 144, 176}, 1);
    add_prop_epic_cemetery(Rectangle{ 160, 272, 128, 112}, 1);
    add_prop_epic_cemetery(Rectangle{ 528, 208, 144, 176}, 1);
  }
  // Trees

  // Stones
  {
    add_prop_epic_cemetery(Rectangle{1024, 256,  64,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1088, 256,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 256,  64,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 256,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 256,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 288,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1056, 288,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1088, 288,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1152, 288,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 288,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1056, 320,  64,  48}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 320,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1152, 320,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 320,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 320,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 352,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 352,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1152, 352,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 352,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 352,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 384,  64,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1088, 384,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 384,  64,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 384,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 384,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 416,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1056, 416,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1088, 416,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 416,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1152, 416,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 416,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 416,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 448,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1056, 448,  64,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 448,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1152, 448,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 448,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 448,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 480,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 480,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1152, 480,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 480,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 480,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 512,  64,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1088, 512,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 512,  64,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 512,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 512,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 544,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1056, 544,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1088, 544,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 544,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1152, 544,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 544,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 544,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 576,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1056, 576,  64,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 576,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1152, 576,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 576,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 576,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 608,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 608,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1152, 608,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 608,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 608,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 640,  64,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1088, 640,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 640,  64,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1152, 640,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 640,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 640,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 672,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1056, 672,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1088, 672,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 672,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1152, 672,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 672,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 672,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 704,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1056, 704,  64,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 704,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1152, 704,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 704,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 704,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 736,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1056, 736,  64,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 736,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1152, 736,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 736,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 736,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 768,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 768,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1152, 768,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1184, 768,  32,  32}, 1);
    add_prop_epic_cemetery(Rectangle{1216, 768,  32,  32}, 1);
  }
  // Stones

  // Tombstones
  {
    add_prop_epic_cemetery(Rectangle{  0, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 32, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 64, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 96, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{128, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{160, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{192, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{224, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{256, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{288, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{320, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{352, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{384, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{416, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{448, 384, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{  0, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 32, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 64, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 96, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{128, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{160, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{192, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{224, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{256, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{288, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{320, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{352, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{384, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{416, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{448, 416, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{  0, 448, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 32, 448, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 64, 448, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 96, 448, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{128, 448, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{160, 448, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{192, 448, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{224, 448, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{256, 448, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{288, 448, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{320, 448, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{352, 448, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{384, 448, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{416, 448, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{448, 448, 32, 32}, 1);
  }
  // Tombstones

  // Skulls
  {
    add_prop_epic_cemetery(Rectangle{  0, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 32, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 64, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 96, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{128, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{160, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{192, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{224, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{256, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{288, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{320, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{352, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{384, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{416, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{448, 1088, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{  0, 1120, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 32, 1120, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 64, 1120, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 96, 1120, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{128, 1120, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{160, 1120, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{192, 1120, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{224, 1120, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{256, 1120, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{288, 1120, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{320, 1120, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{352, 1120, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{384, 1120, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{416, 1120, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{  0, 1152, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 32, 1152, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 64, 1152, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 96, 1152, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{128, 1152, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{160, 1152, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{192, 1152, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{224, 1152, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{256, 1152, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{288, 1152, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{320, 1152, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{352, 1152, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{384, 1152, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{416, 1152, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{  0, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 32, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 64, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 96, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{128, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{160, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{192, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{224, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{256, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{288, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{320, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{352, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{384, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{416, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{448, 1184, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{  0, 1216, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 32, 1216, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 64, 1216, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 96, 1216, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{128, 1216, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{160, 1216, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{192, 1216, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{224, 1216, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{256, 1216, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{288, 1216, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{320, 1216, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{352, 1216, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{384, 1216, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{416, 1216, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{  0, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 32, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 64, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 96, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{128, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{160, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{192, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{224, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{256, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{288, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{320, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{352, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{384, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{416, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{448, 1248, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{  0, 1280, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 32, 1280, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 64, 1280, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 96, 1280, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{128, 1280, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{160, 1280, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{192, 1280, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{224, 1280, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{256, 1280, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{288, 1280, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{  0, 1312, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 32, 1312, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 64, 1312, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 96, 1312, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{128, 1312, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{160, 1312, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{192, 1312, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{224, 1312, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{256, 1312, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{288, 1312, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{  0, 1344, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 32, 1344, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 64, 1344, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 96, 1344, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{128, 1344, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{160, 1344, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{192, 1344, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{224, 1344, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{256, 1344, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{288, 1344, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{  0, 1376, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 32, 1376, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 64, 1376, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{ 96, 1376, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{128, 1376, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{160, 1376, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{192, 1376, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{224, 1376, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{256, 1376, 32, 32}, 1);
    add_prop_epic_cemetery(Rectangle{288, 1376, 32, 32}, 1);
  }
  // Skulls

  // Lamps
  {
    add_prop_epic_cemetery(Rectangle{  576, 1056, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  640, 1056, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  704, 1056, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  768, 1056, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  832, 1056, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  576, 1152, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  640, 1152, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  704, 1152, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  768, 1152, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  832, 1152, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  576, 1248, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  640, 1248, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  704, 1248, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  768, 1248, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  832, 1248, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  576, 1344, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  640, 1344, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  704, 1344, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  768, 1344, 64, 96}, 1);
    add_prop_epic_cemetery(Rectangle{  832, 1344, 64, 96}, 1);
  }
  // Lamps

  // Pikes
  {
    add_prop_epic_cemetery(Rectangle{ 928, 1216, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{ 960, 1216, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{ 992, 1216, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 1216, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{1056, 1216, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{1088, 1216, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 1216, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{ 928, 1280, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{ 960, 1280, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{ 992, 1280, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 1280, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{1056, 1280, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{1088, 1280, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{1120, 1280, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{ 928, 1344, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{ 960, 1344, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{ 992, 1344, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{1024, 1344, 32, 64}, 1);
    add_prop_epic_cemetery(Rectangle{1056, 1344, 32, 64}, 1);
  }
  // Pikes

  // Buildings
  {
    add_prop_epic_cemetery(Rectangle{   0, 784, 160, 208}, 1);
    add_prop_epic_cemetery(Rectangle{ 160, 784, 160, 208}, 1);
    add_prop_epic_cemetery(Rectangle{ 320, 784, 160, 208}, 1);
    add_prop_epic_cemetery(Rectangle{ 480, 768, 160, 224}, 1);
    add_prop_epic_cemetery(Rectangle{ 640, 784, 128, 208}, 1);
    add_prop_epic_cemetery(Rectangle{ 768, 784, 160, 208}, 1);
    add_prop_epic_cemetery(Rectangle{ 928, 768, 160, 224}, 1);
    add_prop_epic_cemetery(Rectangle{1088, 832, 128, 160}, 1);
    add_prop_epic_cemetery(Rectangle{ 928,1008, 160, 208}, 1);
    add_prop_epic_cemetery(Rectangle{1088,1008, 160, 208}, 1);
  }
  // Buildings

  for (int i=0; i<MAX_TILEMAP_LAYERS; ++i) {
    gui_slider_add_option(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER, TextFormat("%d",(i+1)), data_pack(DATA_TYPE_U16, data128( (u16) i ), 1));
  }
}
void end_scene_editor(void) {}


Rectangle se_get_camera_view_rect(Camera2D camera) {

  f32 view_width = BASE_RENDER_RES.x / camera.zoom;
  f32 view_height = BASE_RENDER_RES.y / camera.zoom;

  f32 x = camera.target.x;
  f32 y = camera.target.y;
  
  x -= camera.offset.x/camera.zoom;
  y -= camera.offset.y/camera.zoom;
  
  return Rectangle{ x, y, view_width, view_height };
}
