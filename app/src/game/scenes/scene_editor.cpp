#include "scene_editor.h"
#include <defines.h>
#include <settings.h>
#include <loc_types.h>

#include "core/event.h"
#include "core/fmemory.h"

#include "game/world.h"
#include "game/resource.h"
#include "game/user_interface.h"

#define PROP_DRAG_HANDLE_DIM 16
#define PROP_DRAG_HANDLE_DIM_DIV2 PROP_DRAG_HANDLE_DIM/2.f
#define PROP_PANEL_PROP_DRAW_STARTING_HEIGHT 100.f

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
  u32 next_prop_id;
  std::vector<tilemap_prop> * tilemap_props_selected;

  bool b_show_tilesheet_tile_selection_screen;
  bool b_show_prop_selection_screen;
  bool b_show_prop_edit_screen;
  bool b_dragging_prop;
  bool b_show_pause_menu;

  std::vector<tilemap_prop> tilemap_props_trees;
  std::vector<tilemap_prop> tilemap_props_tombstones;
  std::vector<tilemap_prop> tilemap_props_stones;
  std::vector<tilemap_prop> tilemap_props_spikes;
  std::vector<tilemap_prop> tilemap_props_skulls;
  std::vector<tilemap_prop> tilemap_props_pillars;
  std::vector<tilemap_prop> tilemap_props_lamps;
  std::vector<tilemap_prop> tilemap_props_fence;
  std::vector<tilemap_prop> tilemap_props_details;
  std::vector<tilemap_prop> tilemap_props_candles;
  std::vector<tilemap_prop> tilemap_props_buildings;

  panel prop_selection_panel;
  panel tile_selection_panel;
  panel prop_edit_panel;
  tile selected_tile;
  tilemap_prop* selected_prop;
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
void update_tilemap_prop_type(void);

bool scene_editor_scale_slider_on_click();
bool scene_editor_scale_slider_on_left_button_trigger();
bool scene_editor_scale_slider_on_right_button_trigger();

bool scene_editor_rotation_slider_on_click();
bool scene_editor_rotation_slider_on_left_button_trigger();
bool scene_editor_rotation_slider_on_right_button_trigger();

bool scene_editor_zindex_slider_on_click();
bool scene_editor_zindex_slider_on_left_button_trigger();
bool scene_editor_zindex_slider_on_right_button_trigger();

void add_prop(texture_id source_tex, tilemap_prop_types type, Rectangle source, f32 scale);
#define add_prop_tree(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_TREE, __VA_ARGS__)
#define add_prop_tombstone(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_TOMBSTONE, __VA_ARGS__)
#define add_prop_stone(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_STONE, __VA_ARGS__)
#define add_prop_spike(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_SPIKE, __VA_ARGS__)
#define add_prop_skull(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_SKULL, __VA_ARGS__)
#define add_prop_pillar(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_PILLAR, __VA_ARGS__)
#define add_prop_lamp(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_LAMP, __VA_ARGS__)
#define add_prop_fence(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_FENCE, __VA_ARGS__)
#define add_prop_detail(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_DETAIL, __VA_ARGS__)
#define add_prop_candle(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_CANDLE, __VA_ARGS__)
#define add_prop_building(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_BUILDING, __VA_ARGS__)

void initialize_scene_editor(camera_metrics* _camera_metrics) {
  if (state) {
    begin_scene_editor();
    return;
  }
  state = (scene_editor_state*)allocate_memory_linear(sizeof(scene_editor_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::State allocation failed!");
    return;
  }

  if (!_camera_metrics) {
    TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::Camera metrics recieved NULL");
    return;
  }
  state->in_camera_metrics = _camera_metrics;
  world_system_initialize(_camera_metrics);

  user_interface_system_initialize();

  copy_memory(&state->worldmap_locations, get_worldmap_locations(), sizeof(state->worldmap_locations));
  state->selected_sheet = get_tilesheet_by_enum(TILESHEET_TYPE_MAP);

  state->tile_selection_panel = get_default_panel();
  state->tile_selection_panel.signal_state = BTN_STATE_HOVER;
  state->tile_selection_panel.dest = Rectangle {0, 0, BASE_RENDER_SCALE(.3f).x, BASE_RENDER_RES.y};
  
  state->prop_selection_panel = get_default_panel();
  state->prop_selection_panel.signal_state = BTN_STATE_HOVER;
  state->prop_selection_panel.dest = Rectangle {0, 0, BASE_RENDER_SCALE(.3f).x, BASE_RENDER_RES.y};
  state->prop_selection_panel.scroll_handle = Rectangle{
    .x = state->prop_selection_panel.dest.x + state->prop_selection_panel.dest.width - PROP_DRAG_HANDLE_DIM - 10, .y = 0,
    .width = PROP_DRAG_HANDLE_DIM, .height = PROP_DRAG_HANDLE_DIM * 5,
  };
  state->prop_selection_panel.buffer.f32[1] = PROP_PANEL_PROP_DRAW_STARTING_HEIGHT;

  state->prop_edit_panel = get_default_panel();
  state->prop_edit_panel.signal_state = BTN_STATE_HOVER;
  Vector2 _prop_edit_panel_dim = Vector2 {BASE_RENDER_SCALE(.3f).x, BASE_RENDER_RES.y};
  state->prop_edit_panel.dest = Rectangle { 
    BASE_RENDER_RES.x - _prop_edit_panel_dim.x, 0, 
    _prop_edit_panel_dim.x, _prop_edit_panel_dim.y
  };

  update_tilemap_prop_type();

  // Prop init
  {
    // Trees
    {
    add_prop_tree(Rectangle{  11, 2056, 166, 152}, 1);
    add_prop_tree(Rectangle{ 193, 2117, 122,  91}, 1);
    add_prop_tree(Rectangle{ 321, 2059, 127, 149}, 1);
    add_prop_tree(Rectangle{ 457, 2123,  53,  85}, 1);
    add_prop_tree(Rectangle{ 523, 2056, 166, 152}, 1);
    add_prop_tree(Rectangle{ 705, 2117, 122,  91}, 1);
    add_prop_tree(Rectangle{ 833, 2059, 127, 149}, 1);
    add_prop_tree(Rectangle{ 969, 2123,  53,  85}, 1);
    add_prop_tree(Rectangle{ 197, 2208,  84,  62}, 1);
    add_prop_tree(Rectangle{ 292, 2224,  57,  38}, 1);
    add_prop_tree(Rectangle{ 354, 2225,  60,  39}, 1);
    add_prop_tree(Rectangle{ 709, 2208,  84,  62}, 1);
    add_prop_tree(Rectangle{ 709, 2208,  84,  62}, 1);
    add_prop_tree(Rectangle{ 320,  256,  64,  32}, 1);
    add_prop_tree(Rectangle{ 804, 2224,  57,  38}, 1);
    add_prop_tree(Rectangle{ 866, 2225,  60,  39}, 1);
    add_prop_tree(Rectangle{ 288, 2272,  32,  32}, 1);
    add_prop_tree(Rectangle{ 320, 2272,  64,  32}, 1);
    add_prop_tree(Rectangle{ 384, 2272,  32,  32}, 1);
    add_prop_tree(Rectangle{ 416, 2272,  32,  32}, 1);
    add_prop_tree(Rectangle{ 800, 2272,  32,  32}, 1);
    add_prop_tree(Rectangle{ 832, 2272,  64,  32}, 1);
    add_prop_tree(Rectangle{ 896, 2272,  32,  32}, 1);
    }
    // Trees

    // Tombstones
    {
    add_prop_tombstone(Rectangle{  0, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{ 32, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{ 64, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{ 96, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{128, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{160, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{192, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{224, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{256, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{288, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{320, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{352, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{384, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{416, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{448, 2400, 32, 32}, 1);
    add_prop_tombstone(Rectangle{  0, 2432, 32, 64}, 1);
    add_prop_tombstone(Rectangle{ 32, 2432, 32, 64}, 1);
    add_prop_tombstone(Rectangle{ 64, 2432, 32, 32}, 1);
    add_prop_tombstone(Rectangle{ 96, 2432, 32, 32}, 1);
    add_prop_tombstone(Rectangle{128, 2432, 32, 32}, 1);
    add_prop_tombstone(Rectangle{160, 2432, 32, 32}, 1);
    add_prop_tombstone(Rectangle{192, 2432, 32, 32}, 1);
    add_prop_tombstone(Rectangle{224, 2432, 32, 32}, 1);
    add_prop_tombstone(Rectangle{256, 2432, 32, 32}, 1);
    add_prop_tombstone(Rectangle{288, 2432, 32, 32}, 1);
    add_prop_tombstone(Rectangle{320, 2432, 32, 32}, 1);
    add_prop_tombstone(Rectangle{352, 2432, 32, 32}, 1);
    add_prop_tombstone(Rectangle{384, 2432, 32, 32}, 1);
    add_prop_tombstone(Rectangle{416, 2432, 32, 32}, 1);
    add_prop_tombstone(Rectangle{ 64, 2464, 32, 32}, 1);
    add_prop_tombstone(Rectangle{ 96, 2464, 32, 32}, 1);
    add_prop_tombstone(Rectangle{128, 2464, 32, 32}, 1);
    add_prop_tombstone(Rectangle{160, 2464, 32, 32}, 1);
    add_prop_tombstone(Rectangle{192, 2464, 32, 32}, 1);
    add_prop_tombstone(Rectangle{224, 2464, 32, 32}, 1);
    add_prop_tombstone(Rectangle{256, 2464, 32, 32}, 1);
    add_prop_tombstone(Rectangle{288, 2464, 32, 32}, 1);
    add_prop_tombstone(Rectangle{320, 2464, 32, 32}, 1);
    add_prop_tombstone(Rectangle{352, 2464, 32, 32}, 1);
    add_prop_tombstone(Rectangle{384, 2464, 32, 32}, 1);
    add_prop_tombstone(Rectangle{416, 2464, 32, 32}, 1);
    }
    // Tombstones

    // Stones
    {
    add_prop_stone(Rectangle{1024, 2272,  64,  32}, 1);
    add_prop_stone(Rectangle{1088, 2272,  32,  32}, 1);
    add_prop_stone(Rectangle{1120, 2272,  64,  32}, 1);
    add_prop_stone(Rectangle{1184, 2272,  32,  32}, 1);
    add_prop_stone(Rectangle{1216, 2272,  32,  32}, 1);
    add_prop_stone(Rectangle{1024, 2304,  32,  32}, 1);
    add_prop_stone(Rectangle{1088, 2304,  32,  32}, 1);
    add_prop_stone(Rectangle{1120, 2304,  32,  32}, 1);
    add_prop_stone(Rectangle{1152, 2304,  32,  32}, 1);
    add_prop_stone(Rectangle{1184, 2304,  32,  32}, 1);
    add_prop_stone(Rectangle{1216, 2304,  32,  32}, 1);
    add_prop_stone(Rectangle{1056, 2336,  64,  48}, 1);
    add_prop_stone(Rectangle{1152, 2336,  32,  32}, 1);
    add_prop_stone(Rectangle{1184, 2336,  32,  32}, 1);
    add_prop_stone(Rectangle{1216, 2368,  32,  32}, 1);
    add_prop_stone(Rectangle{1024, 2400,  64,  32}, 1);
    add_prop_stone(Rectangle{1120, 2400,  64,  32}, 1);
    add_prop_stone(Rectangle{1024, 2432,  32,  32}, 1);
    add_prop_stone(Rectangle{1088, 2432,  32,  32}, 1);
    add_prop_stone(Rectangle{1120, 2432,  32,  32}, 1);
    add_prop_stone(Rectangle{1152, 2432,  32,  32}, 1);
    add_prop_stone(Rectangle{1184, 2432,  32,  32}, 1);
    add_prop_stone(Rectangle{1056, 2464,  64,  64}, 1);
    add_prop_stone(Rectangle{1120, 2464,  32,  32}, 1);
    add_prop_stone(Rectangle{1184, 2464,  32,  32}, 1);
    add_prop_stone(Rectangle{1216, 2496,   32,  32}, 1);
    add_prop_stone(Rectangle{1024, 2528,  64,  32}, 1);
    add_prop_stone(Rectangle{1088, 2528,  32,  32}, 1);
    add_prop_stone(Rectangle{1120, 2528,  64,  32}, 1);
    add_prop_stone(Rectangle{1184, 2528,  32,  32}, 1);
    add_prop_stone(Rectangle{1024, 2560,  32,  32}, 1);
    add_prop_stone(Rectangle{1088, 2560,  32,  32}, 1);
    add_prop_stone(Rectangle{1120, 2560,  32,  32}, 1);
    add_prop_stone(Rectangle{1184, 2560,  32,  32}, 1);
    add_prop_stone(Rectangle{1024, 2592,  32,  32}, 1);
    add_prop_stone(Rectangle{1056, 2592,  32,  32}, 1);
    add_prop_stone(Rectangle{1120, 2592,  32,  32}, 1);
    add_prop_stone(Rectangle{1184, 2592,  32,  32}, 1);
    add_prop_stone(Rectangle{1216, 2624,  32,  32}, 1);
    add_prop_stone(Rectangle{1024, 2656,  64,  32}, 1);
    add_prop_stone(Rectangle{1088, 2656,  32,  32}, 1);
    add_prop_stone(Rectangle{1120, 2656,  64,  32}, 1);
    add_prop_stone(Rectangle{1024, 2688,  32,  32}, 1);
    add_prop_stone(Rectangle{1120, 2688,  32,  32}, 1);
    add_prop_stone(Rectangle{1152, 2688,  32,  32}, 1);
    add_prop_stone(Rectangle{1184, 2688,  32,  32}, 1);
    add_prop_stone(Rectangle{1216, 2688,  32,  32}, 1);
    add_prop_stone(Rectangle{1024, 2720,  32,  32}, 1);
    add_prop_stone(Rectangle{1056, 2720,  64,  48}, 1);
    add_prop_stone(Rectangle{1216, 2752,  32,  32}, 1);
    }
    // Stones

    // Spikes
    {
    add_prop_spike(Rectangle{ 928, 3232, 32, 64}, 1);
    add_prop_spike(Rectangle{ 960, 3232, 32, 64}, 1);
    add_prop_spike(Rectangle{ 992, 3232, 32, 64}, 1);
    add_prop_spike(Rectangle{1024, 3232, 32, 64}, 1);
    add_prop_spike(Rectangle{1056, 3232, 32, 64}, 1);
    add_prop_spike(Rectangle{1088, 3232, 32, 64}, 1);
    add_prop_spike(Rectangle{1120, 3232, 32, 64}, 1);
    add_prop_spike(Rectangle{ 928, 3296, 32, 64}, 1);
    add_prop_spike(Rectangle{ 960, 3296, 32, 64}, 1);
    add_prop_spike(Rectangle{ 992, 3296, 32, 64}, 1);
    add_prop_spike(Rectangle{1024, 3296, 32, 64}, 1);
    add_prop_spike(Rectangle{1056, 3296, 32, 64}, 1);
    add_prop_spike(Rectangle{1088, 3296, 32, 64}, 1);
    add_prop_spike(Rectangle{1120, 3296, 32, 64}, 1);
    add_prop_spike(Rectangle{ 928, 3360, 32, 32}, 1);
    add_prop_spike(Rectangle{ 960, 3360, 32, 32}, 1);
    add_prop_spike(Rectangle{ 992, 3360, 32, 32}, 1);
    add_prop_spike(Rectangle{1024, 3360, 32, 32}, 1);
    add_prop_spike(Rectangle{1056, 3360, 32, 32}, 1);
    } 
    // Spikes

    // Skulls
    {
    add_prop_skull(Rectangle{  0, 3104, 32, 32}, 1);
    add_prop_skull(Rectangle{ 32, 3104, 32, 32}, 1);
    add_prop_skull(Rectangle{ 64, 3104, 32, 32}, 1);
    add_prop_skull(Rectangle{ 96, 3104, 32, 32}, 1);
    add_prop_skull(Rectangle{128, 3104, 32, 32}, 1);
    add_prop_skull(Rectangle{160, 3104, 32, 32}, 1);
    add_prop_skull(Rectangle{192, 3104, 32, 32}, 1);
    add_prop_skull(Rectangle{224, 3104, 32, 32}, 1);
    add_prop_skull(Rectangle{256, 3104, 32, 32}, 1);
    add_prop_skull(Rectangle{288, 3104, 32, 32}, 1);
    add_prop_skull(Rectangle{  0, 3136, 32, 32}, 1);
    add_prop_skull(Rectangle{ 32, 3136, 32, 32}, 1);
    add_prop_skull(Rectangle{ 64, 3136, 32, 32}, 1);
    add_prop_skull(Rectangle{ 96, 3136, 32, 32}, 1);
    add_prop_skull(Rectangle{128, 3136, 32, 32}, 1);
    add_prop_skull(Rectangle{160, 3136, 32, 32}, 1);
    add_prop_skull(Rectangle{192, 3136, 32, 32}, 1);
    add_prop_skull(Rectangle{224, 3136, 32, 32}, 1);
    add_prop_skull(Rectangle{256, 3136, 32, 32}, 1);
    add_prop_skull(Rectangle{288, 3136, 32, 32}, 1);
    add_prop_skull(Rectangle{  0, 3168, 32, 32}, 1);
    add_prop_skull(Rectangle{ 32, 3168, 32, 32}, 1);
    add_prop_skull(Rectangle{ 64, 3168, 32, 32}, 1);
    add_prop_skull(Rectangle{ 96, 3168, 32, 32}, 1);
    add_prop_skull(Rectangle{128, 3168, 32, 32}, 1);
    add_prop_skull(Rectangle{160, 3168, 32, 32}, 1);
    add_prop_skull(Rectangle{192, 3168, 32, 32}, 1);
    add_prop_skull(Rectangle{224, 3168, 32, 32}, 1);
    add_prop_skull(Rectangle{256, 3168, 32, 32}, 1);
    add_prop_skull(Rectangle{288, 3168, 32, 32}, 1);
    add_prop_skull(Rectangle{  0, 3200, 32, 32}, 1);
    add_prop_skull(Rectangle{ 32, 3200, 32, 32}, 1);
    add_prop_skull(Rectangle{ 64, 3200, 32, 32}, 1);
    add_prop_skull(Rectangle{ 96, 3200, 32, 32}, 1);
    add_prop_skull(Rectangle{128, 3200, 32, 32}, 1);
    add_prop_skull(Rectangle{160, 3200, 32, 32}, 1);
    add_prop_skull(Rectangle{192, 3200, 32, 32}, 1);
    add_prop_skull(Rectangle{224, 3200, 32, 32}, 1);
    add_prop_skull(Rectangle{256, 3200, 32, 32}, 1);
    add_prop_skull(Rectangle{288, 3200, 32, 32}, 1);
    add_prop_skull(Rectangle{  0, 3232, 32, 32}, 1);
    add_prop_skull(Rectangle{ 32, 3232, 32, 32}, 1);
    add_prop_skull(Rectangle{ 64, 3232, 32, 32}, 1);
    add_prop_skull(Rectangle{ 96, 3232, 32, 32}, 1);
    add_prop_skull(Rectangle{128, 3232, 32, 32}, 1);
    add_prop_skull(Rectangle{160, 3232, 32, 32}, 1);
    add_prop_skull(Rectangle{192, 3232, 32, 32}, 1);
    add_prop_skull(Rectangle{224, 3232, 32, 32}, 1);
    add_prop_skull(Rectangle{256, 3232, 32, 32}, 1);
    add_prop_skull(Rectangle{288, 3232, 32, 32}, 1);
    add_prop_skull(Rectangle{  0, 3264, 32, 32}, 1);
    add_prop_skull(Rectangle{ 32, 3264, 32, 32}, 1);
    add_prop_skull(Rectangle{ 64, 3264, 32, 32}, 1);
    add_prop_skull(Rectangle{ 96, 3264, 32, 32}, 1);
    add_prop_skull(Rectangle{128, 3264, 32, 32}, 1);
    add_prop_skull(Rectangle{160, 3264, 32, 32}, 1);
    add_prop_skull(Rectangle{192, 3264, 32, 32}, 1);
    add_prop_skull(Rectangle{224, 3264, 32, 32}, 1);
    add_prop_skull(Rectangle{256, 3264, 32, 32}, 1);
    add_prop_skull(Rectangle{288, 3264, 32, 32}, 1);
    add_prop_skull(Rectangle{  0, 3296, 32, 32}, 1);
    add_prop_skull(Rectangle{ 32, 3296, 32, 32}, 1);
    add_prop_skull(Rectangle{ 64, 3296, 32, 32}, 1);
    add_prop_skull(Rectangle{ 96, 3296, 32, 32}, 1);
    add_prop_skull(Rectangle{128, 3296, 32, 32}, 1);
    add_prop_skull(Rectangle{160, 3296, 32, 32}, 1);
    add_prop_skull(Rectangle{192, 3296, 32, 32}, 1);
    add_prop_skull(Rectangle{224, 3296, 32, 32}, 1);
    add_prop_skull(Rectangle{256, 3296, 32, 32}, 1);
    add_prop_skull(Rectangle{288, 3296, 32, 32}, 1);
    add_prop_skull(Rectangle{  0, 3328, 32, 32}, 1);
    add_prop_skull(Rectangle{ 32, 3328, 32, 32}, 1);
    add_prop_skull(Rectangle{ 64, 3328, 32, 32}, 1);
    add_prop_skull(Rectangle{ 96, 3328, 32, 32}, 1);
    add_prop_skull(Rectangle{128, 3328, 32, 32}, 1);
    add_prop_skull(Rectangle{160, 3328, 32, 32}, 1);
    add_prop_skull(Rectangle{192, 3328, 32, 32}, 1);
    add_prop_skull(Rectangle{224, 3328, 32, 32}, 1);
    add_prop_skull(Rectangle{256, 3328, 32, 32}, 1);
    add_prop_skull(Rectangle{288, 3328, 32, 32}, 1);
    add_prop_skull(Rectangle{  0, 3360, 32, 32}, 1);
    add_prop_skull(Rectangle{ 32, 3360, 32, 32}, 1);
    add_prop_skull(Rectangle{ 64, 3360, 32, 32}, 1);
    add_prop_skull(Rectangle{ 96, 3360, 32, 32}, 1);
    add_prop_skull(Rectangle{128, 3360, 32, 32}, 1);
    add_prop_skull(Rectangle{160, 3360, 32, 32}, 1);
    add_prop_skull(Rectangle{192, 3360, 32, 32}, 1);
    add_prop_skull(Rectangle{224, 3360, 32, 32}, 1);
    add_prop_skull(Rectangle{256, 3360, 32, 32}, 1);
    add_prop_skull(Rectangle{288, 3360, 32, 32}, 1);
    add_prop_skull(Rectangle{  0, 3392, 32, 32}, 1);
    add_prop_skull(Rectangle{ 32, 3392, 32, 32}, 1);
    add_prop_skull(Rectangle{ 64, 3392, 32, 32}, 1);
    add_prop_skull(Rectangle{ 96, 3392, 32, 32}, 1);
    add_prop_skull(Rectangle{128, 3392, 32, 32}, 1);
    add_prop_skull(Rectangle{160, 3392, 32, 32}, 1);
    add_prop_skull(Rectangle{192, 3392, 32, 32}, 1);
    add_prop_skull(Rectangle{224, 3392, 32, 32}, 1);
    add_prop_skull(Rectangle{256, 3392, 32, 32}, 1);
    add_prop_skull(Rectangle{288, 3392, 32, 32}, 1);
    }
    // Skulls

    // Pillar
    {
    add_prop_pillar(Rectangle{   928, 3232, 32, 64}, 1);
    add_prop_pillar(Rectangle{   960, 3232, 32, 64}, 1);
    add_prop_pillar(Rectangle{   992, 3232, 32, 64}, 1);
    add_prop_pillar(Rectangle{  1024, 3232, 32, 64}, 1);
    add_prop_pillar(Rectangle{  1056, 3232, 32, 64}, 1);
    add_prop_pillar(Rectangle{  1088, 3232, 32, 64}, 1);
    add_prop_pillar(Rectangle{  1120, 3232, 32, 64}, 1);
    add_prop_pillar(Rectangle{   928, 3296, 32, 64}, 1);
    add_prop_pillar(Rectangle{   960, 3296, 32, 64}, 1);
    add_prop_pillar(Rectangle{   992, 3296, 32, 64}, 1);
    add_prop_pillar(Rectangle{  1024, 3296, 32, 64}, 1);
    add_prop_pillar(Rectangle{  1056, 3296, 32, 64}, 1);
    add_prop_pillar(Rectangle{  1088, 3296, 32, 64}, 1);
    add_prop_pillar(Rectangle{  1120, 3296, 32, 64}, 1);
    add_prop_pillar(Rectangle{   928, 3360, 32, 32}, 1);
    add_prop_pillar(Rectangle{   960, 3360, 32, 32}, 1);
    add_prop_pillar(Rectangle{   992, 3360, 32, 32}, 1);
    add_prop_pillar(Rectangle{  1024, 3360, 32, 32}, 1);
    add_prop_pillar(Rectangle{  1056, 3360, 32, 32}, 1);
    add_prop_pillar(Rectangle{   928, 3392, 32, 32}, 1);
    add_prop_pillar(Rectangle{   960, 3392, 32, 32}, 1);
    add_prop_pillar(Rectangle{   992, 3392, 32, 32}, 1);
    add_prop_pillar(Rectangle{  1024, 3392, 32, 32}, 1);
    add_prop_pillar(Rectangle{  1056, 3392, 32, 32}, 1);
    } 
    // Pillar 

    // Lamps
    {
    add_prop_lamp(Rectangle{  576, 3072, 64, 96}, 1);
    add_prop_lamp(Rectangle{  640, 3072, 64, 96}, 1);
    add_prop_lamp(Rectangle{  704, 3072, 64, 96}, 1);
    add_prop_lamp(Rectangle{  768, 3072, 64, 96}, 1);
    add_prop_lamp(Rectangle{  832, 3072, 64, 96}, 1);
    add_prop_lamp(Rectangle{  576, 3168, 64, 96}, 1);
    add_prop_lamp(Rectangle{  640, 3168, 64, 96}, 1);
    add_prop_lamp(Rectangle{  704, 3168, 64, 96}, 1);
    add_prop_lamp(Rectangle{  768, 3168, 64, 96}, 1);
    add_prop_lamp(Rectangle{  832, 3168, 64, 96}, 1);
    add_prop_lamp(Rectangle{  576, 3264, 64, 96}, 1);
    add_prop_lamp(Rectangle{  640, 3264, 64, 96}, 1);
    add_prop_lamp(Rectangle{  704, 3264, 64, 96}, 1);
    add_prop_lamp(Rectangle{  768, 3264, 64, 96}, 1);
    add_prop_lamp(Rectangle{  832, 3264, 64, 96}, 1);
    add_prop_lamp(Rectangle{  576, 3360, 64, 96}, 1);
    add_prop_lamp(Rectangle{  640, 3360, 64, 96}, 1);
    add_prop_lamp(Rectangle{  704, 3360, 64, 96}, 1);
    add_prop_lamp(Rectangle{  768, 3360, 64, 96}, 1);
    add_prop_lamp(Rectangle{  832, 3360, 64, 96}, 1);
    }
    // Lamps

    // Fence
    {
    add_prop_fence(Rectangle{ 32, 2528, 32, 32}, 1);
    add_prop_fence(Rectangle{ 64, 2528, 32, 64}, 1);
    add_prop_fence(Rectangle{ 96, 2528, 32, 32}, 1);
    add_prop_fence(Rectangle{ 32, 2560, 32, 64}, 1);
    add_prop_fence(Rectangle{ 96, 2560, 32, 64}, 1);
    add_prop_fence(Rectangle{  0, 2560, 32, 64}, 1);
    add_prop_fence(Rectangle{128, 2560, 32, 64}, 1);
    add_prop_fence(Rectangle{  0, 2624, 32, 32}, 1);
    add_prop_fence(Rectangle{128, 2624, 32, 32}, 1);
    add_prop_fence(Rectangle{  0, 2656, 32, 64}, 1);
    add_prop_fence(Rectangle{ 32, 2656, 32, 64}, 1);
    add_prop_fence(Rectangle{ 32, 2720, 32, 32}, 1);
    add_prop_fence(Rectangle{ 64, 2688, 32, 64}, 1);
    add_prop_fence(Rectangle{ 96, 2656, 32, 64}, 1);
    add_prop_fence(Rectangle{ 96, 2720, 32, 32}, 1);
    add_prop_fence(Rectangle{128, 2656, 32, 64}, 1);
    add_prop_fence(Rectangle{128, 2624, 32, 32}, 1);
    add_prop_fence(Rectangle{128, 2528, 32, 32}, 1);
    add_prop_fence(Rectangle{160, 2528, 32, 32}, 1);
    add_prop_fence(Rectangle{128, 2496, 32, 32}, 1);
    add_prop_fence(Rectangle{160, 2496, 32, 32}, 1);
    add_prop_fence(Rectangle{192, 2528, 32, 32}, 1);
    add_prop_fence(Rectangle{224, 2528, 32, 32}, 1);
    add_prop_fence(Rectangle{256, 2528, 32, 32}, 1);
    add_prop_fence(Rectangle{256, 2560, 32, 32}, 1);
    add_prop_fence(Rectangle{288, 2560, 32, 32}, 1);
    add_prop_fence(Rectangle{288, 2592, 32, 32}, 1);
    add_prop_fence(Rectangle{288, 2624, 32, 32}, 1);
    add_prop_fence(Rectangle{256, 2624, 32, 32}, 1);
    add_prop_fence(Rectangle{256, 2656, 32, 32}, 1);
    add_prop_fence(Rectangle{224, 2656, 32, 32}, 1);
    add_prop_fence(Rectangle{192, 2656, 32, 32}, 1);
    add_prop_fence(Rectangle{192, 2624, 32, 32}, 1);
    add_prop_fence(Rectangle{160, 2624, 32, 32}, 1);
    add_prop_fence(Rectangle{160, 2592, 32, 32}, 1);
    add_prop_fence(Rectangle{160, 2560, 32, 32}, 1);
    add_prop_fence(Rectangle{192, 2560, 32, 32}, 1);
    add_prop_fence(Rectangle{352, 2528, 32, 32}, 1);
    add_prop_fence(Rectangle{384, 2528, 32, 32}, 1);
    add_prop_fence(Rectangle{416, 2528, 32, 32}, 1);
    add_prop_fence(Rectangle{416, 2560, 32, 32}, 1);
    add_prop_fence(Rectangle{448, 2560, 32, 32}, 1);
    add_prop_fence(Rectangle{448, 2592, 32, 32}, 1);
    add_prop_fence(Rectangle{448, 2624, 32, 32}, 1);
    add_prop_fence(Rectangle{416, 2624, 32, 32}, 1);
    add_prop_fence(Rectangle{416, 2656, 32, 32}, 1);
    add_prop_fence(Rectangle{384, 2656, 32, 32}, 1);
    add_prop_fence(Rectangle{352, 2656, 32, 32}, 1);
    add_prop_fence(Rectangle{352, 2624, 32, 32}, 1);
    add_prop_fence(Rectangle{320, 2624, 32, 32}, 1);
    add_prop_fence(Rectangle{320, 2592, 32, 32}, 1);
    add_prop_fence(Rectangle{320, 2560, 32, 32}, 1);
    add_prop_fence(Rectangle{152, 2560, 32, 64}, 1);
    add_prop_fence(Rectangle{160, 2688, 128,96}, 1);
    add_prop_fence(Rectangle{288, 2688, 128,96}, 1);
    add_prop_fence(Rectangle{ 48, 3024, 64, 64}, 1);
    add_prop_fence(Rectangle{192, 3024, 96, 64}, 1);
    add_prop_fence(Rectangle{368, 3024, 64, 64}, 1);
    add_prop_fence(Rectangle{528, 3008, 64, 64}, 1);
    add_prop_fence(Rectangle{672, 3008, 64, 64}, 1);

    }
    // Fence 

    // Detail
    {
    add_prop_detail(Rectangle{ 480, 2592, 32, 32}, 1);
    add_prop_detail(Rectangle{ 512, 2592, 32, 32}, 1);
    add_prop_detail(Rectangle{ 544, 2592, 32, 32}, 1);
    add_prop_detail(Rectangle{ 576, 2592, 32, 32}, 1);
    add_prop_detail(Rectangle{ 608, 2592, 32, 32}, 1);
    add_prop_detail(Rectangle{ 480, 2624, 32, 32}, 1);
    add_prop_detail(Rectangle{ 512, 2624, 32, 32}, 1);
    add_prop_detail(Rectangle{ 544, 2624, 32, 64}, 1);
    add_prop_detail(Rectangle{ 576, 2640, 32, 48}, 1);
    add_prop_detail(Rectangle{ 480, 2656, 32, 32}, 1);
    add_prop_detail(Rectangle{ 624, 2640, 64, 48}, 1);
    add_prop_detail(Rectangle{ 544, 2688, 32, 64}, 1);
    add_prop_detail(Rectangle{ 576, 2688, 32, 64}, 1);
    add_prop_detail(Rectangle{ 624, 2704, 48, 48}, 1);
    }
    // Detail 

    // Candle
    {
    add_prop_candle(Rectangle{ 320, 3104, 32, 32}, 1);
    add_prop_candle(Rectangle{ 352, 3104, 32, 32}, 1);
    add_prop_candle(Rectangle{ 384, 3104, 32, 32}, 1);
    add_prop_candle(Rectangle{ 416, 3104, 32, 32}, 1);
    add_prop_candle(Rectangle{ 448, 3104, 32, 32}, 1);
    add_prop_candle(Rectangle{ 320, 3136, 32, 32}, 1);
    add_prop_candle(Rectangle{ 352, 3136, 32, 32}, 1);
    add_prop_candle(Rectangle{ 384, 3136, 32, 32}, 1);
    add_prop_candle(Rectangle{ 416, 3136, 32, 32}, 1);
    add_prop_candle(Rectangle{ 320, 3168, 32, 32}, 1);
    add_prop_candle(Rectangle{ 352, 3168, 32, 32}, 1);
    add_prop_candle(Rectangle{ 384, 3168, 32, 32}, 1);
    add_prop_candle(Rectangle{ 416, 3168, 32, 32}, 1);
    add_prop_candle(Rectangle{ 320, 3200, 32, 32}, 1);
    add_prop_candle(Rectangle{ 352, 3200, 32, 32}, 1);
    add_prop_candle(Rectangle{ 384, 3200, 32, 32}, 1);
    add_prop_candle(Rectangle{ 416, 3200, 32, 32}, 1);
    add_prop_candle(Rectangle{ 448, 3200, 32, 32}, 1);
    add_prop_candle(Rectangle{ 320, 3232, 32, 32}, 1);
    add_prop_candle(Rectangle{ 352, 3232, 32, 32}, 1);
    add_prop_candle(Rectangle{ 384, 3232, 32, 32}, 1);
    add_prop_candle(Rectangle{ 416, 3232, 32, 32}, 1);
    add_prop_candle(Rectangle{ 320, 3264, 32, 32}, 1);
    add_prop_candle(Rectangle{ 352, 3264, 32, 32}, 1);
    add_prop_candle(Rectangle{ 384, 3264, 32, 32}, 1);
    add_prop_candle(Rectangle{ 416, 3264, 32, 32}, 1);
    add_prop_candle(Rectangle{ 448, 3264, 32, 32}, 1);
    }
    // Candle 

    // Buildings
    {
    add_prop_building(Rectangle{   0, 2800, 160, 208}, 1);
    add_prop_building(Rectangle{ 160, 2800, 160, 208}, 1);
    add_prop_building(Rectangle{ 320, 2800, 160, 208}, 1);
    add_prop_building(Rectangle{ 480, 2784, 160, 224}, 1);
    add_prop_building(Rectangle{ 640, 2800, 128, 208}, 1);
    add_prop_building(Rectangle{ 768, 2800, 160, 208}, 1);
    add_prop_building(Rectangle{ 928, 2784, 160, 224}, 1);
    add_prop_building(Rectangle{1088, 2848, 128, 160}, 1);
    add_prop_building(Rectangle{ 928, 3024, 160, 208}, 1);
    add_prop_building(Rectangle{1088, 3024, 160, 208}, 1);
    }
    // Buildings
  }

  for (int i=0; i<MAX_TILEMAP_LAYERS; ++i) {
    gui_slider_add_option(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER, data_pack(DATA_TYPE_U16, data128( (u16) i ), 1), 0, std::to_string(LOC_TEXT_MAINMENU_NUMBERS_1+i).c_str());
  }

  register_slider(
    SDR_ID_EDITOR_PROP_SCALE_SLIDER,  SDR_TYPE_NUMBER, 
    BTN_ID_EDITOR_PROP_SCALE_SLIDER_LEFT, BTN_ID_EDITOR_PROP_SCALE_SLIDER_RIGHT, 
    true, true
  );
  register_slider(
    SDR_ID_EDITOR_PROP_ROTATION_SLIDER,  SDR_TYPE_NUMBER, 
    BTN_ID_EDITOR_PROP_ROTATION_SLIDER_LEFT, BTN_ID_EDITOR_PROP_ROTATION_SLIDER_RIGHT, 
    true, true
  );
  register_slider(
    SDR_ID_EDITOR_PROP_ZINDEX_SLIDER,  SDR_TYPE_NUMBER, 
    BTN_ID_EDITOR_PROP_ZINDEX_SLIDER_LEFT, BTN_ID_EDITOR_PROP_ZINDEX_SLIDER_RIGHT, 
    true, true
  );
  
  slider * scale_slider = get_slider_by_id(SDR_ID_EDITOR_PROP_SCALE_SLIDER);
  if (scale_slider == nullptr) {
    TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::Scale slider couldn't be registered");
    return;
  }
  scale_slider->on_left_button_trigger =  scene_editor_scale_slider_on_left_button_trigger;
  scale_slider->on_right_button_trigger = scene_editor_scale_slider_on_right_button_trigger;
  scale_slider->on_click =                scene_editor_scale_slider_on_click;

  slider * rotation_slider = get_slider_by_id(SDR_ID_EDITOR_PROP_ROTATION_SLIDER);
  if (rotation_slider == nullptr) {
    TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::Rotation slider couldn't be registered");
    return;
  }
  rotation_slider->on_left_button_trigger =  scene_editor_rotation_slider_on_left_button_trigger;
  rotation_slider->on_right_button_trigger = scene_editor_rotation_slider_on_right_button_trigger;
  rotation_slider->on_click =                scene_editor_rotation_slider_on_click;

  slider * zindex_slider = get_slider_by_id(SDR_ID_EDITOR_PROP_ZINDEX_SLIDER);
  if (zindex_slider == nullptr) {
    TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::Z-index slider couldn't be registered");
    return;
  }
  zindex_slider->on_left_button_trigger =  scene_editor_zindex_slider_on_left_button_trigger;
  zindex_slider->on_right_button_trigger = scene_editor_zindex_slider_on_right_button_trigger;
  zindex_slider->on_click =                scene_editor_zindex_slider_on_click;
  
  begin_scene_editor();
}

// UPDATE / RENDER
void update_scene_editor(void) {
  if(!IsWindowFocused()) {
    state->mouse_focus = MOUSE_FOCUS_UNFOCUSED;
    return;
  }
  else state->mouse_focus = MOUSE_FOCUS_MAP;
  state->mouse_pos_screen.x = GetMousePosition().x * get_app_settings()->scale_ratio.at(0);
  state->mouse_pos_screen.y = GetMousePosition().y * get_app_settings()->scale_ratio.at(1);
  state->mouse_pos_world = GetScreenToWorld2D(state->mouse_pos_screen, state->in_camera_metrics->handle);
  state->edit_layer = get_slider_current_value(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER)->data.u16[0]; // HACK: Should not updated every frame
  state->in_camera_metrics->frustum = se_get_camera_view_rect(state->in_camera_metrics->handle);

  editor_update_bindings();
  update_map();
  update_user_interface();
  update_tilemap_prop_type();
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

      gui_label_format_v(FONT_TYPE_MEDIUM, 10, SCREEN_POS(14.f,10.f), WHITE, true, true, "%d", state->selected_stage);

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
      gui_slider(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, VECTOR2(0,0), VECTOR2(5,3), 3.f);
      f32 prop_height_count = pnl->buffer.f32[1];
      for (size_t i = 0; i < state->tilemap_props_selected->size(); ++i) {
        const tilemap_prop& prop = state->tilemap_props_selected->at(i);
        Rectangle dest = prop.dest;
        dest.x = 0;
        dest.y = (pnl->scroll * pnl->buffer.f32[0]) + prop_height_count;
        gui_draw_texture_id_pro(prop.tex_id, prop.source, dest, false);
        prop_height_count += prop.dest.height;
      }
      pnl->buffer.f32[0] = prop_height_count;
      pnl->scroll_handle.y = FMAX(pnl->scroll_handle.y, pnl->dest.y + SCREEN_OFFSET.x);
      pnl->scroll_handle.y = FMIN(pnl->scroll_handle.y, pnl->dest.y + pnl->dest.height);
      pnl->scroll = (pnl->scroll_handle.y - pnl->dest.y - SCREEN_OFFSET.x) / (pnl->dest.height - pnl->scroll_handle.height) * -1;
      DrawRectangleRec(pnl->scroll_handle, WHITE);
    });
  }

  if(state->selection_type == SLC_TYPE_SLC_PROP) 
  {
    panel* pnl = &state->prop_edit_panel;
    gui_panel_scissored((*pnl), false, {
      gui_slider(SDR_ID_EDITOR_PROP_SCALE_SLIDER,    VECTOR2(pnl->dest.x,pnl->dest.y), VECTOR2(5,1),  3.f);
      gui_slider(SDR_ID_EDITOR_PROP_ROTATION_SLIDER, VECTOR2(pnl->dest.x,pnl->dest.y), VECTOR2(5,5),  3.f);
      gui_slider(SDR_ID_EDITOR_PROP_ZINDEX_SLIDER,   VECTOR2(pnl->dest.x,pnl->dest.y), VECTOR2(5,10), 3.f);
    });
  }

  switch (state->selection_type) {
    case SLC_TYPE_TILE: {
      _render_tile_on_pos(&state->selected_tile, state->mouse_pos_screen, state->selected_sheet);
      break;
    }
    case SLC_TYPE_DROP_PROP: {
      gui_draw_texture_id_pro(
        state->selected_prop->tex_id, state->selected_prop->source,
        Rectangle { state->mouse_pos_screen.x, state->mouse_pos_screen.y, state->selected_prop->dest.width, state->selected_prop->dest.height}, 
        false
      );
      break;
    }
    case SLC_TYPE_SLC_PROP: {
      Vector2 prop_pos = GetWorldToScreen2D(Vector2{state->selected_prop->dest.x, state->selected_prop->dest.y}, state->in_camera_metrics->handle);
      f32 relative_width = state->selected_prop->dest.width * state->selected_prop->scale * state->in_camera_metrics->handle.zoom;
      f32 relative_height = state->selected_prop->dest.height * state->selected_prop->scale * state->in_camera_metrics->handle.zoom;
      
      prop_pos.x -= relative_width * 0.5f; // To center to its origin
      prop_pos.y -= relative_height * 0.5f;

      DrawRectangleLines(prop_pos.x, prop_pos.y, relative_width, relative_height, WHITE);
      prop_pos.x += relative_width / 2.f - PROP_DRAG_HANDLE_DIM_DIV2;
      prop_pos.y += relative_height / 2.f - PROP_DRAG_HANDLE_DIM_DIV2;
      DrawRectangle(prop_pos.x, prop_pos.y, PROP_DRAG_HANDLE_DIM, PROP_DRAG_HANDLE_DIM, WHITE);
      break;
    }
    default: break;
  }
  
  if (state->b_show_pause_menu) {
    gui_draw_pause_screen(false);
  }

  render_user_interface();
}
// UPDATE / RENDER

void add_prop(texture_id source_tex, tilemap_prop_types type, Rectangle source, f32 scale) {
  if (source_tex >= TEX_ID_MAX || source_tex <= TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "scene_editor::add_prop()::Provided texture id out of bound");
    return;
  }
  const Texture2D* tex = get_texture_by_enum(source_tex);
  if (!tex) {
    TraceLog(LOG_WARNING, "scene_editor::add_prop()::Invalid texture");
    return;
  }
  if (tex->width < source.x + source.width || tex->height < source.y + source.height) {
    TraceLog(LOG_WARNING, "scene_editor::add_prop()::Provided prop dimentions out of bound");
    return;
  }
  
  tilemap_prop prop = {};

  prop.id = state->next_prop_id++;
  prop.tex_id = source_tex;
  prop.prop_type = type;
  
  prop.source = source;
  prop.dest = Rectangle {0, 0, source.width * scale, source.height * scale};
  prop.scale = 1.f;
  prop.rotation = 0.f;
  prop.zindex = 0;

  prop.is_initialized = true;

  switch (type) {
    case TILEMAP_PROP_TYPE_TREE:      { state->tilemap_props_trees.push_back(prop); break; }
    case TILEMAP_PROP_TYPE_TOMBSTONE: { state->tilemap_props_tombstones.push_back(prop); break; }
    case TILEMAP_PROP_TYPE_STONE:     { state->tilemap_props_stones.push_back(prop); break; }
    case TILEMAP_PROP_TYPE_SPIKE:     { state->tilemap_props_spikes.push_back(prop); break; }
    case TILEMAP_PROP_TYPE_SKULL:     { state->tilemap_props_skulls.push_back(prop); break; }
    case TILEMAP_PROP_TYPE_PILLAR:    { state->tilemap_props_pillars.push_back(prop); break; }
    case TILEMAP_PROP_TYPE_LAMP:      { state->tilemap_props_lamps.push_back(prop); break; }
    case TILEMAP_PROP_TYPE_FENCE:     { state->tilemap_props_fence.push_back(prop); break; }
    case TILEMAP_PROP_TYPE_DETAIL:    { state->tilemap_props_details.push_back(prop); break; }
    case TILEMAP_PROP_TYPE_CANDLE:    { state->tilemap_props_candles.push_back(prop); break; }
    case TILEMAP_PROP_TYPE_BUILDING:  { state->tilemap_props_buildings.push_back(prop); break; }
    default: { 
      TraceLog(LOG_WARNING, "scene_editor::add_prop()::Unsupported tilemap type");
      return;
    }
  }
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
      i32 h = (pnl->scroll * pnl->buffer.f32[0] * (-1)) + state->mouse_pos_screen.y - pnl->buffer.f32[1];
      for (size_t i=0; i< state->tilemap_props_selected->size() && h > 0; ++i) {
        if(h - state->tilemap_props_selected->at(i).source.height < 0) {
          state->selected_prop = &state->tilemap_props_selected->at(i);
          state->selection_type = SLC_TYPE_DROP_PROP;
          break;
        }
        h -= state->tilemap_props_selected->at(i).source.height;
      }
    }
  }
  else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->mouse_focus == MOUSE_FOCUS_MAP) 
  {
    switch (state->selection_type) {
      case SLC_TYPE_UNSELECTED: {
        tilemap_prop* prop = get_map_prop_by_pos(state->mouse_pos_world);
        if (prop != nullptr) {
          state->selected_prop = prop;
          state->selection_type = SLC_TYPE_SLC_PROP;
        }
        break;
      }
      case SLC_TYPE_DROP_PROP: {
        Vector2 coord = GetScreenToWorld2D(state->mouse_pos_screen, state->in_camera_metrics->handle);
        state->selected_prop->dest.x = coord.x;
        state->selected_prop->dest.y = coord.y;
        add_prop_curr_map(state->selected_prop);
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
        tilemap_prop* prop = get_map_prop_by_id(state->selected_prop->id);
        Rectangle drag_handle = Rectangle {
          prop->dest.x - PROP_DRAG_HANDLE_DIM_DIV2, prop->dest.y - PROP_DRAG_HANDLE_DIM_DIV2, 
          PROP_DRAG_HANDLE_DIM, PROP_DRAG_HANDLE_DIM
        };
        if(!state->b_dragging_prop && CheckCollisionPointRec(state->mouse_pos_world, drag_handle)) {
          state->b_dragging_prop = true;
        }
        if (state->b_dragging_prop) {
          prop->dest.x = state->mouse_pos_world.x;
          prop->dest.y = state->mouse_pos_world.y;
          state->selected_prop->dest = prop->dest;
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
    state->b_show_pause_menu = !state->b_show_pause_menu;
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
      if(!remove_prop_cur_map_by_id(state->selected_prop->id)) {
        TraceLog(LOG_WARNING, "scene_editor::editor_update_keyboard_bindings()::Removing property failed.");
      }
      state->selected_prop = nullptr;
      state->selection_type = SLC_TYPE_UNSELECTED;
    }
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

  event_fire(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, event_context(data128(state->target.x, state->target.y)));
}
void end_scene_editor(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::end_scene_editor()::State is not valid");
    return;
  }

  state->b_show_tilesheet_tile_selection_screen = false;
  state->b_show_prop_selection_screen = false;
  state->b_dragging_prop = false;
  state->b_show_pause_menu = false;
  state->selected_tile = {};
  state->selected_prop = {};
  state->target = {};
  state->selection_type = editor_state_selection_type::SLC_TYPE_UNSELECTED;
  state->mouse_focus = editor_state_mouse_focus::MOUSE_FOCUS_UNFOCUSED;
}

Rectangle se_get_camera_view_rect(Camera2D camera) {

  f32 view_width = BASE_RENDER_RES.x / camera.zoom;
  f32 view_height = BASE_RENDER_RES.y / camera.zoom;

  f32 x = camera.target.x;
  f32 y = camera.target.y;
  
  x -= camera.offset.x/camera.zoom;
  y -= camera.offset.y/camera.zoom;
  
  return Rectangle{ x, y, view_width, view_height };
}
void update_tilemap_prop_type(void) {
  if (!state) {
    TraceLog(LOG_WARNING, "scene_editor::update_tilemap_prop_type()::State is not valid");
    return;
  }
  
  tilemap_prop_types selected_prop_type = static_cast<tilemap_prop_types>(get_slider_current_value(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->data.i32[0]);
  switch (selected_prop_type) {
    case TILEMAP_PROP_TYPE_TREE:      { state->tilemap_props_selected = &state->tilemap_props_trees;      break; }
    case TILEMAP_PROP_TYPE_TOMBSTONE: { state->tilemap_props_selected = &state->tilemap_props_tombstones; break; }
    case TILEMAP_PROP_TYPE_STONE:     { state->tilemap_props_selected = &state->tilemap_props_stones;     break; }
    case TILEMAP_PROP_TYPE_SPIKE:     { state->tilemap_props_selected = &state->tilemap_props_spikes;     break; }
    case TILEMAP_PROP_TYPE_SKULL:     { state->tilemap_props_selected = &state->tilemap_props_skulls;     break; }
    case TILEMAP_PROP_TYPE_PILLAR:    { state->tilemap_props_selected = &state->tilemap_props_pillars;    break; }
    case TILEMAP_PROP_TYPE_LAMP:      { state->tilemap_props_selected = &state->tilemap_props_lamps;      break; }
    case TILEMAP_PROP_TYPE_FENCE:     { state->tilemap_props_selected = &state->tilemap_props_fence;      break; }
    case TILEMAP_PROP_TYPE_DETAIL:    { state->tilemap_props_selected = &state->tilemap_props_details;    break; }
    case TILEMAP_PROP_TYPE_CANDLE:    { state->tilemap_props_selected = &state->tilemap_props_candles;    break; }
    case TILEMAP_PROP_TYPE_BUILDING:  { state->tilemap_props_selected = &state->tilemap_props_buildings;  break; }
    default: { 
      TraceLog(LOG_WARNING, "scene_editor::initialize_scene_editor()::Unsupported tilemap prop type");
      ui_set_slider_current_index(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, 1);
      state->tilemap_props_selected = &state->tilemap_props_trees;
      break; 
    }
  }
}


bool scene_editor_scale_slider_on_click() { return true; }
bool scene_editor_scale_slider_on_left_button_trigger() {
  if (state->selected_prop != nullptr) {
    state->selected_prop->scale -= 0.15f;
    if(state->selected_prop->scale < 0.1f) {
      state->selected_prop->scale = 0.1f;
    }
    
    get_slider_by_id(SDR_ID_EDITOR_PROP_SCALE_SLIDER)->options.at(0).no_localized_text = TextFormat("%.2f", state->selected_prop->scale);
    return true;
  }

  return false;
}
bool scene_editor_scale_slider_on_right_button_trigger() {
  if (state->selected_prop != nullptr) {
    state->selected_prop->scale += 0.15f;

    get_slider_by_id(SDR_ID_EDITOR_PROP_SCALE_SLIDER)->options.at(0).no_localized_text = TextFormat("%.2f", state->selected_prop->scale);
    return true;
  }

  return false;
}

bool scene_editor_rotation_slider_on_click() { return true; }
bool scene_editor_rotation_slider_on_left_button_trigger() {
  if (state->selected_prop != nullptr) {
    state->selected_prop->rotation -= 10.f;
    if(state->selected_prop->rotation < 0.f) {
      state->selected_prop->rotation += 360.f;
    }
    
    get_slider_by_id(SDR_ID_EDITOR_PROP_ROTATION_SLIDER)->options.at(0).no_localized_text = TextFormat("%.1f", state->selected_prop->rotation);
    return true;
  }

  return false;
}
bool scene_editor_rotation_slider_on_right_button_trigger() {
  if (state->selected_prop != nullptr) {
    state->selected_prop->rotation += 10.f;
    if(state->selected_prop->rotation > 360.f) {
      state->selected_prop->rotation -= 360.f;
    }
    
    get_slider_by_id(SDR_ID_EDITOR_PROP_ROTATION_SLIDER)->options.at(0).no_localized_text = TextFormat("%.1f", state->selected_prop->rotation);
    return true;
  }

  return false;
}

bool scene_editor_zindex_slider_on_click() { return true; }
bool scene_editor_zindex_slider_on_left_button_trigger() {
  if (state->selected_prop != nullptr) {
    state->selected_prop->zindex -= 1;
    if(state->selected_prop->zindex > MAX_Z_INDEX_SLOT) {
      state->selected_prop->zindex += MAX_Z_INDEX_SLOT;
    }
    
    get_slider_by_id(SDR_ID_EDITOR_PROP_ZINDEX_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", state->selected_prop->zindex);
    return true;
  }

  return false;
}
bool scene_editor_zindex_slider_on_right_button_trigger() {
  if (state->selected_prop != nullptr) {
    state->selected_prop->zindex += 1;
    if(state->selected_prop->zindex > MAX_Z_INDEX_SLOT) {
      state->selected_prop->zindex -= MAX_Z_INDEX_SLOT;
    }
    
    get_slider_by_id(SDR_ID_EDITOR_PROP_ZINDEX_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", state->selected_prop->zindex);
    return true;
  }

  return false;
}
