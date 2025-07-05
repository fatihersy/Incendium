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
  SLC_TYPE_DROP_PROP_STATIC,
  SLC_TYPE_SLC_PROP_STATIC,
  SLC_TYPE_DROP_PROP_SPRITE,
  SLC_TYPE_SLC_PROP_SPRITE,
} editor_state_selection_type;

typedef enum editor_state_mouse_focus {
  MOUSE_FOCUS_UNFOCUSED, // While window unfocused or not on the screen
  MOUSE_FOCUS_MAP,
  MOUSE_FOCUS_TILE_SELECTION,
  MOUSE_FOCUS_PROP_SELECTION,
} editor_state_mouse_focus;

typedef struct scene_editor_state {
  bool b_show_tilesheet_tile_selection_screen;
  bool b_prop_selection_screen_update_prop_sprites;
  bool b_show_prop_selection_screen;
  bool b_show_prop_edit_screen;
  bool b_dragging_prop;
  bool b_show_pause_menu;

  Vector2 target;
  std::vector<tilemap_prop_static> * tilemap_props_static_selected; // INFO: The prop list that shown on props panel
  std::vector<tilemap_prop_sprite> * tilemap_props_sprite_selected;
  tilemap_prop_static* selected_prop_static_map_prop_address;
  tilemap_prop_sprite* selected_prop_sprite_map_prop_address;
  tilemap_prop_static  selected_prop_static_panel_selection_copy;
  tilemap_prop_sprite  selected_prop_sprite_panel_selection_copy;
  tilesheet* selected_sheet;
  tile selected_tile;
  u16 edit_layer;
  u16 selected_stage;
  editor_state_selection_type selection_type;
  editor_state_mouse_focus mouse_focus;

  std::vector<tilemap_prop_static> tilemap_props_trees;
  std::vector<tilemap_prop_static> tilemap_props_tombstones;
  std::vector<tilemap_prop_static> tilemap_props_stones;
  std::vector<tilemap_prop_static> tilemap_props_spikes;
  std::vector<tilemap_prop_static> tilemap_props_skulls;
  std::vector<tilemap_prop_static> tilemap_props_pillars;
  std::vector<tilemap_prop_static> tilemap_props_lamps;
  std::vector<tilemap_prop_static> tilemap_props_fence;
  std::vector<tilemap_prop_static> tilemap_props_details;
  std::vector<tilemap_prop_static> tilemap_props_candles;
  std::vector<tilemap_prop_static> tilemap_props_buildings;
  std::vector<tilemap_prop_sprite> tilemap_props_sprite;

  camera_metrics * in_camera_metrics;
  std::array<worldmap_stage, MAX_WORLDMAP_LOCATIONS> worldmap_locations;
  panel prop_selection_panel;
  panel tile_selection_panel;
  panel prop_edit_panel;
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

bool scene_editor_scale_slider_on_left_button_trigger(void);
bool scene_editor_scale_slider_on_right_button_trigger(void);
bool scene_editor_rotation_slider_on_left_button_trigger(void);
bool scene_editor_rotation_slider_on_right_button_trigger(void);
bool scene_editor_zindex_slider_on_left_button_trigger(void);
bool scene_editor_zindex_slider_on_right_button_trigger(void);
bool scene_editor_map_layer_slc_slider_on_left_button_trigger(void);
bool scene_editor_map_layer_slc_slider_on_right_button_trigger(void);
bool scene_editor_map_stage_slc_slider_on_left_button_trigger(void);
bool scene_editor_map_stage_slc_slider_on_right_button_trigger(void);
bool scene_editor_map_prop_type_slc_slider_on_left_button_trigger(void);
bool scene_editor_map_prop_type_slc_slider_on_right_button_trigger(void);

void add_prop(texture_id source_tex, tilemap_prop_types type, Rectangle source, f32 scale = 1.f, Color tint = WHITE);
void add_prop(tilemap_prop_types type, spritesheet_id sprite_id, f32 scale = 1.f, Color tint = WHITE);
#define add_prop_tree(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_TREE __VA_OPT__(,) __VA_ARGS__)
#define add_prop_tombstone(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_TOMBSTONE, __VA_ARGS__)
#define add_prop_stone(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_STONE __VA_OPT__(,) __VA_ARGS__)
#define add_prop_spike(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_SPIKE __VA_OPT__(,) __VA_ARGS__)
#define add_prop_skull(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_SKULL __VA_OPT__(,) __VA_ARGS__)
#define add_prop_pillar(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_PILLAR __VA_OPT__(,) __VA_ARGS__)
#define add_prop_lamp(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_LAMP __VA_OPT__(,) __VA_ARGS__)
#define add_prop_fence(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_FENCE __VA_OPT__(,) __VA_ARGS__)
#define add_prop_detail(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_DETAIL __VA_OPT__(,) __VA_ARGS__)
#define add_prop_candle(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_CANDLE __VA_OPT__(,) __VA_ARGS__)
#define add_prop_building(...) add_prop(TEX_ID_ASSET_ATLAS, TILEMAP_PROP_TYPE_BUILDING __VA_OPT__(,) __VA_ARGS__)
#define add_prop_sprite(SPRITE_ID, ...) add_prop(TILEMAP_PROP_TYPE_SPRITE, SPRITE_ID __VA_OPT__(,) __VA_ARGS__)

bool initialize_scene_editor(camera_metrics* _camera_metrics) {
  if (state) {
    update_tilemap_prop_type();
    begin_scene_editor();
    return true;
  }
  state = (scene_editor_state*)allocate_memory_linear(sizeof(scene_editor_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::State allocation failed!");
    return false;
  }

  if (!_camera_metrics) {
    TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::Camera metrics recieved NULL");
    return false;
  }
  state->in_camera_metrics = _camera_metrics;
  world_system_initialize(_camera_metrics);

  if(!user_interface_system_initialize()) {
    TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::User interface failed to initialize!");
    return false;
  }

  copy_memory(state->worldmap_locations.data(), get_worldmap_locations(), MAX_WORLDMAP_LOCATIONS * sizeof(worldmap_stage));
  state->selected_sheet = get_tilesheet_by_enum(TILESHEET_TYPE_MAP);

  state->tile_selection_panel = panel();
  state->tile_selection_panel.signal_state = BTN_STATE_HOVER;
  state->tile_selection_panel.dest = Rectangle {0, 0, BASE_RENDER_SCALE(.45f).x, BASE_RENDER_RES.y};
  
  state->prop_selection_panel = panel();
  state->prop_selection_panel.signal_state = BTN_STATE_HOVER;
  state->prop_selection_panel.dest = Rectangle {0, 0, BASE_RENDER_SCALE(.35f).x, BASE_RENDER_RES.y};
  state->prop_selection_panel.scroll_handle = Rectangle{
    .x = state->prop_selection_panel.dest.x + state->prop_selection_panel.dest.width - PROP_DRAG_HANDLE_DIM - 10, .y = 0,
    .width = PROP_DRAG_HANDLE_DIM, .height = PROP_DRAG_HANDLE_DIM * 5,
  };
  state->prop_selection_panel.buffer.f32[1] = PROP_PANEL_PROP_DRAW_STARTING_HEIGHT;

  state->prop_edit_panel = panel();
  state->prop_edit_panel.signal_state = BTN_STATE_HOVER;
  Vector2 _prop_edit_panel_dim = Vector2 {BASE_RENDER_SCALE(.3f).x, BASE_RENDER_RES.y};
  state->prop_edit_panel.dest = Rectangle { 
    BASE_RENDER_RES.x - _prop_edit_panel_dim.x, 0, 
    _prop_edit_panel_dim.x, _prop_edit_panel_dim.y
  };

  // Prop init
  {
    // Trees
    {
      add_prop_tree(Rectangle{  11, 2056, 166, 152});
      add_prop_tree(Rectangle{ 193, 2117, 122,  91});
      add_prop_tree(Rectangle{ 321, 2059, 127, 149});
      add_prop_tree(Rectangle{ 457, 2123,  53,  85});
      add_prop_tree(Rectangle{ 523, 2056, 166, 152});
      add_prop_tree(Rectangle{ 705, 2117, 122,  91});
      add_prop_tree(Rectangle{ 833, 2059, 127, 149});
      add_prop_tree(Rectangle{ 969, 2123,  53,  85});
      add_prop_tree(Rectangle{ 197, 2208,  84,  62});
      add_prop_tree(Rectangle{ 292, 2224,  57,  38});
      add_prop_tree(Rectangle{ 354, 2225,  60,  39});
      add_prop_tree(Rectangle{ 709, 2208,  84,  62});
      add_prop_tree(Rectangle{ 709, 2208,  84,  62});
      add_prop_tree(Rectangle{ 320,  256,  64,  32});
      add_prop_tree(Rectangle{ 804, 2224,  57,  38});
      add_prop_tree(Rectangle{ 866, 2225,  60,  39});
      add_prop_tree(Rectangle{ 288, 2272,  32,  32});
      add_prop_tree(Rectangle{ 320, 2272,  64,  32});
      add_prop_tree(Rectangle{ 384, 2272,  32,  32});
      add_prop_tree(Rectangle{ 416, 2272,  32,  32});
      add_prop_tree(Rectangle{ 800, 2272,  32,  32});
      add_prop_tree(Rectangle{ 832, 2272,  64,  32});
      add_prop_tree(Rectangle{ 896, 2272,  32,  32});
    }
    // Trees

    // Tombstones
    {
      add_prop_tombstone(Rectangle{  0, 2400, 32, 32});
      add_prop_tombstone(Rectangle{ 32, 2400, 32, 32});
      add_prop_tombstone(Rectangle{ 64, 2400, 32, 32});
      add_prop_tombstone(Rectangle{ 96, 2400, 32, 32});
      add_prop_tombstone(Rectangle{128, 2400, 32, 32});
      add_prop_tombstone(Rectangle{160, 2400, 32, 32});
      add_prop_tombstone(Rectangle{192, 2400, 32, 32});
      add_prop_tombstone(Rectangle{224, 2400, 32, 32});
      add_prop_tombstone(Rectangle{256, 2400, 32, 32});
      add_prop_tombstone(Rectangle{288, 2400, 32, 32});
      add_prop_tombstone(Rectangle{320, 2400, 32, 32});
      add_prop_tombstone(Rectangle{352, 2400, 32, 32});
      add_prop_tombstone(Rectangle{384, 2400, 32, 32});
      add_prop_tombstone(Rectangle{416, 2400, 32, 32});
      add_prop_tombstone(Rectangle{448, 2400, 32, 32});
      add_prop_tombstone(Rectangle{  0, 2432, 32, 64});
      add_prop_tombstone(Rectangle{ 32, 2432, 32, 64});
      add_prop_tombstone(Rectangle{ 64, 2432, 32, 32});
      add_prop_tombstone(Rectangle{ 96, 2432, 32, 32});
      add_prop_tombstone(Rectangle{128, 2432, 32, 32});
      add_prop_tombstone(Rectangle{160, 2432, 32, 32});
      add_prop_tombstone(Rectangle{192, 2432, 32, 32});
      add_prop_tombstone(Rectangle{224, 2432, 32, 32});
      add_prop_tombstone(Rectangle{256, 2432, 32, 32});
      add_prop_tombstone(Rectangle{288, 2432, 32, 32});
      add_prop_tombstone(Rectangle{320, 2432, 32, 32});
      add_prop_tombstone(Rectangle{352, 2432, 32, 32});
      add_prop_tombstone(Rectangle{384, 2432, 32, 32});
      add_prop_tombstone(Rectangle{416, 2432, 32, 32});
      add_prop_tombstone(Rectangle{ 64, 2464, 32, 32});
      add_prop_tombstone(Rectangle{ 96, 2464, 32, 32});
      add_prop_tombstone(Rectangle{128, 2464, 32, 32});
      add_prop_tombstone(Rectangle{160, 2464, 32, 32});
      add_prop_tombstone(Rectangle{192, 2464, 32, 32});
      add_prop_tombstone(Rectangle{224, 2464, 32, 32});
      add_prop_tombstone(Rectangle{256, 2464, 32, 32});
      add_prop_tombstone(Rectangle{288, 2464, 32, 32});
      add_prop_tombstone(Rectangle{320, 2464, 32, 32});
      add_prop_tombstone(Rectangle{352, 2464, 32, 32});
      add_prop_tombstone(Rectangle{384, 2464, 32, 32});
      add_prop_tombstone(Rectangle{416, 2464, 32, 32});
    }
    // Tombstones

    // Stones
    {
      add_prop_stone(Rectangle{1024, 2272,  64,  32});
      add_prop_stone(Rectangle{1088, 2272,  32,  32});
      add_prop_stone(Rectangle{1120, 2272,  64,  32});
      add_prop_stone(Rectangle{1184, 2272,  32,  32});
      add_prop_stone(Rectangle{1216, 2272,  32,  32});
      add_prop_stone(Rectangle{1024, 2304,  32,  32});
      add_prop_stone(Rectangle{1088, 2304,  32,  32});
      add_prop_stone(Rectangle{1120, 2304,  32,  32});
      add_prop_stone(Rectangle{1152, 2304,  32,  32});
      add_prop_stone(Rectangle{1184, 2304,  32,  32});
      add_prop_stone(Rectangle{1216, 2304,  32,  32});
      add_prop_stone(Rectangle{1056, 2336,  64,  48});
      add_prop_stone(Rectangle{1152, 2336,  32,  32});
      add_prop_stone(Rectangle{1184, 2336,  32,  32});
      add_prop_stone(Rectangle{1216, 2368,  32,  32});
      add_prop_stone(Rectangle{1024, 2400,  64,  32});
      add_prop_stone(Rectangle{1120, 2400,  64,  32});
      add_prop_stone(Rectangle{1024, 2432,  32,  32});
      add_prop_stone(Rectangle{1088, 2432,  32,  32});
      add_prop_stone(Rectangle{1120, 2432,  32,  32});
      add_prop_stone(Rectangle{1152, 2432,  32,  32});
      add_prop_stone(Rectangle{1184, 2432,  32,  32});
      add_prop_stone(Rectangle{1056, 2464,  64,  64});
      add_prop_stone(Rectangle{1120, 2464,  32,  32});
      add_prop_stone(Rectangle{1184, 2464,  32,  32});
      add_prop_stone(Rectangle{1216, 2496,   32, 32});
      add_prop_stone(Rectangle{1024, 2528,  64,  32});
      add_prop_stone(Rectangle{1088, 2528,  32,  32});
      add_prop_stone(Rectangle{1120, 2528,  64,  32});
      add_prop_stone(Rectangle{1184, 2528,  32,  32});
      add_prop_stone(Rectangle{1024, 2560,  32,  32});
      add_prop_stone(Rectangle{1088, 2560,  32,  32});
      add_prop_stone(Rectangle{1120, 2560,  32,  32});
      add_prop_stone(Rectangle{1184, 2560,  32,  32});
      add_prop_stone(Rectangle{1024, 2592,  32,  32});
      add_prop_stone(Rectangle{1056, 2592,  32,  32});
      add_prop_stone(Rectangle{1120, 2592,  32,  32});
      add_prop_stone(Rectangle{1184, 2592,  32,  32});
      add_prop_stone(Rectangle{1216, 2624,  32,  32});
      add_prop_stone(Rectangle{1024, 2656,  64,  32});
      add_prop_stone(Rectangle{1088, 2656,  32,  32});
      add_prop_stone(Rectangle{1120, 2656,  64,  32});
      add_prop_stone(Rectangle{1024, 2688,  32,  32});
      add_prop_stone(Rectangle{1120, 2688,  32,  32});
      add_prop_stone(Rectangle{1152, 2688,  32,  32});
      add_prop_stone(Rectangle{1184, 2688,  32,  32});
      add_prop_stone(Rectangle{1216, 2688,  32,  32});
      add_prop_stone(Rectangle{1024, 2720,  32,  32});
      add_prop_stone(Rectangle{1056, 2720,  64,  48});
      add_prop_stone(Rectangle{1216, 2752,  32,  32});
    }
    // Stones

    // Spikes
    {
      add_prop_spike(Rectangle{ 928, 3232, 32, 64});
      add_prop_spike(Rectangle{ 960, 3232, 32, 64});
      add_prop_spike(Rectangle{ 992, 3232, 32, 64});
      add_prop_spike(Rectangle{1024, 3232, 32, 64});
      add_prop_spike(Rectangle{1056, 3232, 32, 64});
      add_prop_spike(Rectangle{1088, 3232, 32, 64});
      add_prop_spike(Rectangle{1120, 3232, 32, 64});
      add_prop_spike(Rectangle{ 928, 3296, 32, 64});
      add_prop_spike(Rectangle{ 960, 3296, 32, 64});
      add_prop_spike(Rectangle{ 992, 3296, 32, 64});
      add_prop_spike(Rectangle{1024, 3296, 32, 64});
      add_prop_spike(Rectangle{1056, 3296, 32, 64});
      add_prop_spike(Rectangle{1088, 3296, 32, 64});
      add_prop_spike(Rectangle{1120, 3296, 32, 64});
      add_prop_spike(Rectangle{ 928, 3360, 32, 32});
      add_prop_spike(Rectangle{ 960, 3360, 32, 32});
      add_prop_spike(Rectangle{ 992, 3360, 32, 32});
      add_prop_spike(Rectangle{1024, 3360, 32, 32});
      add_prop_spike(Rectangle{1056, 3360, 32, 32});
    } 
    // Spikes

    // Skulls
    {
      add_prop_skull(Rectangle{  0, 3104, 32, 32});
      add_prop_skull(Rectangle{ 32, 3104, 32, 32});
      add_prop_skull(Rectangle{ 64, 3104, 32, 32});
      add_prop_skull(Rectangle{ 96, 3104, 32, 32});
      add_prop_skull(Rectangle{128, 3104, 32, 32});
      add_prop_skull(Rectangle{160, 3104, 32, 32});
      add_prop_skull(Rectangle{192, 3104, 32, 32});
      add_prop_skull(Rectangle{224, 3104, 32, 32});
      add_prop_skull(Rectangle{256, 3104, 32, 32});
      add_prop_skull(Rectangle{288, 3104, 32, 32});
      add_prop_skull(Rectangle{  0, 3136, 32, 32});
      add_prop_skull(Rectangle{ 32, 3136, 32, 32});
      add_prop_skull(Rectangle{ 64, 3136, 32, 32});
      add_prop_skull(Rectangle{ 96, 3136, 32, 32});
      add_prop_skull(Rectangle{128, 3136, 32, 32});
      add_prop_skull(Rectangle{160, 3136, 32, 32});
      add_prop_skull(Rectangle{192, 3136, 32, 32});
      add_prop_skull(Rectangle{224, 3136, 32, 32});
      add_prop_skull(Rectangle{256, 3136, 32, 32});
      add_prop_skull(Rectangle{288, 3136, 32, 32});
      add_prop_skull(Rectangle{  0, 3168, 32, 32});
      add_prop_skull(Rectangle{ 32, 3168, 32, 32});
      add_prop_skull(Rectangle{ 64, 3168, 32, 32});
      add_prop_skull(Rectangle{ 96, 3168, 32, 32});
      add_prop_skull(Rectangle{128, 3168, 32, 32});
      add_prop_skull(Rectangle{160, 3168, 32, 32});
      add_prop_skull(Rectangle{192, 3168, 32, 32});
      add_prop_skull(Rectangle{224, 3168, 32, 32});
      add_prop_skull(Rectangle{256, 3168, 32, 32});
      add_prop_skull(Rectangle{288, 3168, 32, 32});
      add_prop_skull(Rectangle{  0, 3200, 32, 32});
      add_prop_skull(Rectangle{ 32, 3200, 32, 32});
      add_prop_skull(Rectangle{ 64, 3200, 32, 32});
      add_prop_skull(Rectangle{ 96, 3200, 32, 32});
      add_prop_skull(Rectangle{128, 3200, 32, 32});
      add_prop_skull(Rectangle{160, 3200, 32, 32});
      add_prop_skull(Rectangle{192, 3200, 32, 32});
      add_prop_skull(Rectangle{224, 3200, 32, 32});
      add_prop_skull(Rectangle{256, 3200, 32, 32});
      add_prop_skull(Rectangle{288, 3200, 32, 32});
      add_prop_skull(Rectangle{  0, 3232, 32, 32});
      add_prop_skull(Rectangle{ 32, 3232, 32, 32});
      add_prop_skull(Rectangle{ 64, 3232, 32, 32});
      add_prop_skull(Rectangle{ 96, 3232, 32, 32});
      add_prop_skull(Rectangle{128, 3232, 32, 32});
      add_prop_skull(Rectangle{160, 3232, 32, 32});
      add_prop_skull(Rectangle{192, 3232, 32, 32});
      add_prop_skull(Rectangle{224, 3232, 32, 32});
      add_prop_skull(Rectangle{256, 3232, 32, 32});
      add_prop_skull(Rectangle{288, 3232, 32, 32});
      add_prop_skull(Rectangle{  0, 3264, 32, 32});
      add_prop_skull(Rectangle{ 32, 3264, 32, 32});
      add_prop_skull(Rectangle{ 64, 3264, 32, 32});
      add_prop_skull(Rectangle{ 96, 3264, 32, 32});
      add_prop_skull(Rectangle{128, 3264, 32, 32});
      add_prop_skull(Rectangle{160, 3264, 32, 32});
      add_prop_skull(Rectangle{192, 3264, 32, 32});
      add_prop_skull(Rectangle{224, 3264, 32, 32});
      add_prop_skull(Rectangle{256, 3264, 32, 32});
      add_prop_skull(Rectangle{288, 3264, 32, 32});
      add_prop_skull(Rectangle{  0, 3296, 32, 32});
      add_prop_skull(Rectangle{ 32, 3296, 32, 32});
      add_prop_skull(Rectangle{ 64, 3296, 32, 32});
      add_prop_skull(Rectangle{ 96, 3296, 32, 32});
      add_prop_skull(Rectangle{128, 3296, 32, 32});
      add_prop_skull(Rectangle{160, 3296, 32, 32});
      add_prop_skull(Rectangle{192, 3296, 32, 32});
      add_prop_skull(Rectangle{224, 3296, 32, 32});
      add_prop_skull(Rectangle{256, 3296, 32, 32});
      add_prop_skull(Rectangle{288, 3296, 32, 32});
      add_prop_skull(Rectangle{  0, 3328, 32, 32});
      add_prop_skull(Rectangle{ 32, 3328, 32, 32});
      add_prop_skull(Rectangle{ 64, 3328, 32, 32});
      add_prop_skull(Rectangle{ 96, 3328, 32, 32});
      add_prop_skull(Rectangle{128, 3328, 32, 32});
      add_prop_skull(Rectangle{160, 3328, 32, 32});
      add_prop_skull(Rectangle{192, 3328, 32, 32});
      add_prop_skull(Rectangle{224, 3328, 32, 32});
      add_prop_skull(Rectangle{256, 3328, 32, 32});
      add_prop_skull(Rectangle{288, 3328, 32, 32});
      add_prop_skull(Rectangle{  0, 3360, 32, 32});
      add_prop_skull(Rectangle{ 32, 3360, 32, 32});
      add_prop_skull(Rectangle{ 64, 3360, 32, 32});
      add_prop_skull(Rectangle{ 96, 3360, 32, 32});
      add_prop_skull(Rectangle{128, 3360, 32, 32});
      add_prop_skull(Rectangle{160, 3360, 32, 32});
      add_prop_skull(Rectangle{192, 3360, 32, 32});
      add_prop_skull(Rectangle{224, 3360, 32, 32});
      add_prop_skull(Rectangle{256, 3360, 32, 32});
      add_prop_skull(Rectangle{288, 3360, 32, 32});
      add_prop_skull(Rectangle{  0, 3392, 32, 32});
      add_prop_skull(Rectangle{ 32, 3392, 32, 32});
      add_prop_skull(Rectangle{ 64, 3392, 32, 32});
      add_prop_skull(Rectangle{ 96, 3392, 32, 32});
      add_prop_skull(Rectangle{128, 3392, 32, 32});
      add_prop_skull(Rectangle{160, 3392, 32, 32});
      add_prop_skull(Rectangle{192, 3392, 32, 32});
      add_prop_skull(Rectangle{224, 3392, 32, 32});
      add_prop_skull(Rectangle{256, 3392, 32, 32});
      add_prop_skull(Rectangle{288, 3392, 32, 32});
    }
    // Skulls

    // Pillar
    {
      add_prop_pillar(Rectangle{   928, 3232, 32, 64});
      add_prop_pillar(Rectangle{   960, 3232, 32, 64});
      add_prop_pillar(Rectangle{   992, 3232, 32, 64});
      add_prop_pillar(Rectangle{  1024, 3232, 32, 64});
      add_prop_pillar(Rectangle{  1056, 3232, 32, 64});
      add_prop_pillar(Rectangle{  1088, 3232, 32, 64});
      add_prop_pillar(Rectangle{  1120, 3232, 32, 64});
      add_prop_pillar(Rectangle{   928, 3296, 32, 64});
      add_prop_pillar(Rectangle{   960, 3296, 32, 64});
      add_prop_pillar(Rectangle{   992, 3296, 32, 64});
      add_prop_pillar(Rectangle{  1024, 3296, 32, 64});
      add_prop_pillar(Rectangle{  1056, 3296, 32, 64});
      add_prop_pillar(Rectangle{  1088, 3296, 32, 64});
      add_prop_pillar(Rectangle{  1120, 3296, 32, 64});
      add_prop_pillar(Rectangle{   928, 3360, 32, 32});
      add_prop_pillar(Rectangle{   960, 3360, 32, 32});
      add_prop_pillar(Rectangle{   992, 3360, 32, 32});
      add_prop_pillar(Rectangle{  1024, 3360, 32, 32});
      add_prop_pillar(Rectangle{  1056, 3360, 32, 32});
      add_prop_pillar(Rectangle{   928, 3392, 32, 32});
      add_prop_pillar(Rectangle{   960, 3392, 32, 32});
      add_prop_pillar(Rectangle{   992, 3392, 32, 32});
      add_prop_pillar(Rectangle{  1024, 3392, 32, 32});
      add_prop_pillar(Rectangle{  1056, 3392, 32, 32});
    } 
    // Pillar 

    // Lamps
    {
      add_prop_lamp(Rectangle{  576, 3072, 64, 96});
      add_prop_lamp(Rectangle{  640, 3072, 64, 96});
      add_prop_lamp(Rectangle{  704, 3072, 64, 96});
      add_prop_lamp(Rectangle{  768, 3072, 64, 96});
      add_prop_lamp(Rectangle{  832, 3072, 64, 96});
      add_prop_lamp(Rectangle{  576, 3168, 64, 96});
      add_prop_lamp(Rectangle{  640, 3168, 64, 96});
      add_prop_lamp(Rectangle{  704, 3168, 64, 96});
      add_prop_lamp(Rectangle{  768, 3168, 64, 96});
      add_prop_lamp(Rectangle{  832, 3168, 64, 96});
      add_prop_lamp(Rectangle{  576, 3264, 64, 96});
      add_prop_lamp(Rectangle{  640, 3264, 64, 96});
      add_prop_lamp(Rectangle{  704, 3264, 64, 96});
      add_prop_lamp(Rectangle{  768, 3264, 64, 96});
      add_prop_lamp(Rectangle{  832, 3264, 64, 96});
      add_prop_lamp(Rectangle{  576, 3360, 64, 96});
      add_prop_lamp(Rectangle{  640, 3360, 64, 96});
      add_prop_lamp(Rectangle{  704, 3360, 64, 96});
      add_prop_lamp(Rectangle{  768, 3360, 64, 96});
      add_prop_lamp(Rectangle{  832, 3360, 64, 96});
    }
    // Lamps

    // Fence
    {
      add_prop_fence(Rectangle{ 32, 2528, 32, 32});
      add_prop_fence(Rectangle{ 64, 2528, 32, 64});
      add_prop_fence(Rectangle{ 96, 2528, 32, 32});
      add_prop_fence(Rectangle{ 32, 2560, 32, 64});
      add_prop_fence(Rectangle{ 96, 2560, 32, 64});
      add_prop_fence(Rectangle{  0, 2560, 32, 64});
      add_prop_fence(Rectangle{128, 2560, 32, 64});
      add_prop_fence(Rectangle{  0, 2624, 32, 32});
      add_prop_fence(Rectangle{128, 2624, 32, 32});
      add_prop_fence(Rectangle{  0, 2656, 32, 64});
      add_prop_fence(Rectangle{ 32, 2656, 32, 64});
      add_prop_fence(Rectangle{ 32, 2720, 32, 32});
      add_prop_fence(Rectangle{ 64, 2688, 32, 64});
      add_prop_fence(Rectangle{ 96, 2656, 32, 64});
      add_prop_fence(Rectangle{ 96, 2720, 32, 32});
      add_prop_fence(Rectangle{128, 2656, 32, 64});
      add_prop_fence(Rectangle{128, 2624, 32, 32});
      add_prop_fence(Rectangle{128, 2528, 32, 32});
      add_prop_fence(Rectangle{160, 2528, 32, 32});
      add_prop_fence(Rectangle{128, 2496, 32, 32});
      add_prop_fence(Rectangle{160, 2496, 32, 32});
      add_prop_fence(Rectangle{192, 2528, 32, 32});
      add_prop_fence(Rectangle{224, 2528, 32, 32});
      add_prop_fence(Rectangle{256, 2528, 32, 32});
      add_prop_fence(Rectangle{256, 2560, 32, 32});
      add_prop_fence(Rectangle{288, 2560, 32, 32});
      add_prop_fence(Rectangle{288, 2592, 32, 32});
      add_prop_fence(Rectangle{288, 2624, 32, 32});
      add_prop_fence(Rectangle{256, 2624, 32, 32});
      add_prop_fence(Rectangle{256, 2656, 32, 32});
      add_prop_fence(Rectangle{224, 2656, 32, 32});
      add_prop_fence(Rectangle{192, 2656, 32, 32});
      add_prop_fence(Rectangle{192, 2624, 32, 32});
      add_prop_fence(Rectangle{160, 2624, 32, 32});
      add_prop_fence(Rectangle{160, 2592, 32, 32});
      add_prop_fence(Rectangle{160, 2560, 32, 32});
      add_prop_fence(Rectangle{192, 2560, 32, 32});
      add_prop_fence(Rectangle{352, 2528, 32, 32});
      add_prop_fence(Rectangle{384, 2528, 32, 32});
      add_prop_fence(Rectangle{416, 2528, 32, 32});
      add_prop_fence(Rectangle{416, 2560, 32, 32});
      add_prop_fence(Rectangle{448, 2560, 32, 32});
      add_prop_fence(Rectangle{448, 2592, 32, 32});
      add_prop_fence(Rectangle{448, 2624, 32, 32});
      add_prop_fence(Rectangle{416, 2624, 32, 32});
      add_prop_fence(Rectangle{416, 2656, 32, 32});
      add_prop_fence(Rectangle{384, 2656, 32, 32});
      add_prop_fence(Rectangle{352, 2656, 32, 32});
      add_prop_fence(Rectangle{352, 2624, 32, 32});
      add_prop_fence(Rectangle{320, 2624, 32, 32});
      add_prop_fence(Rectangle{320, 2592, 32, 32});
      add_prop_fence(Rectangle{320, 2560, 32, 32});
      add_prop_fence(Rectangle{152, 2560, 32, 64});
      add_prop_fence(Rectangle{160, 2688, 128,96});
      add_prop_fence(Rectangle{288, 2688, 128,96});
      add_prop_fence(Rectangle{ 48, 3024, 64, 64});
      add_prop_fence(Rectangle{192, 3024, 96, 64});
      add_prop_fence(Rectangle{368, 3024, 64, 64});
      add_prop_fence(Rectangle{528, 3008, 64, 64});
      add_prop_fence(Rectangle{672, 3008, 64, 64});
    }
    // Fence 

    // Detail
    {
      add_prop_detail(Rectangle{ 480, 2592, 32, 32});
      add_prop_detail(Rectangle{ 512, 2592, 32, 32});
      add_prop_detail(Rectangle{ 544, 2592, 32, 32});
      add_prop_detail(Rectangle{ 576, 2592, 32, 32});
      add_prop_detail(Rectangle{ 608, 2592, 32, 32});
      add_prop_detail(Rectangle{ 480, 2624, 32, 32});
      add_prop_detail(Rectangle{ 512, 2624, 32, 32});
      add_prop_detail(Rectangle{ 544, 2624, 32, 64});
      add_prop_detail(Rectangle{ 576, 2640, 32, 48});
      add_prop_detail(Rectangle{ 480, 2656, 32, 32});
      add_prop_detail(Rectangle{ 624, 2640, 64, 48});
      add_prop_detail(Rectangle{ 544, 2688, 32, 64});
      add_prop_detail(Rectangle{ 576, 2688, 32, 64});
      add_prop_detail(Rectangle{ 624, 2704, 48, 48});
      add_prop_detail(Rectangle{1152, 3296,400, 64}, 1.f, (Color {255, 255, 255, 128}));
    }
    // Detail 

    // Candle
    {
      add_prop_candle(Rectangle{ 320, 3104, 32, 32});
      add_prop_candle(Rectangle{ 352, 3104, 32, 32});
      add_prop_candle(Rectangle{ 384, 3104, 32, 32});
      add_prop_candle(Rectangle{ 416, 3104, 32, 32});
      add_prop_candle(Rectangle{ 448, 3104, 32, 32});
      add_prop_candle(Rectangle{ 320, 3136, 32, 32});
      add_prop_candle(Rectangle{ 352, 3136, 32, 32});
      add_prop_candle(Rectangle{ 384, 3136, 32, 32});
      add_prop_candle(Rectangle{ 416, 3136, 32, 32});
      add_prop_candle(Rectangle{ 320, 3168, 32, 32});
      add_prop_candle(Rectangle{ 352, 3168, 32, 32});
      add_prop_candle(Rectangle{ 384, 3168, 32, 32});
      add_prop_candle(Rectangle{ 416, 3168, 32, 32});
      add_prop_candle(Rectangle{ 320, 3200, 32, 32});
      add_prop_candle(Rectangle{ 352, 3200, 32, 32});
      add_prop_candle(Rectangle{ 384, 3200, 32, 32});
      add_prop_candle(Rectangle{ 416, 3200, 32, 32});
      add_prop_candle(Rectangle{ 448, 3200, 32, 32});
      add_prop_candle(Rectangle{ 320, 3232, 32, 32});
      add_prop_candle(Rectangle{ 352, 3232, 32, 32});
      add_prop_candle(Rectangle{ 384, 3232, 32, 32});
      add_prop_candle(Rectangle{ 416, 3232, 32, 32});
      add_prop_candle(Rectangle{ 320, 3264, 32, 32});
      add_prop_candle(Rectangle{ 352, 3264, 32, 32});
      add_prop_candle(Rectangle{ 384, 3264, 32, 32});
      add_prop_candle(Rectangle{ 416, 3264, 32, 32});
      add_prop_candle(Rectangle{ 448, 3264, 32, 32});
    }
    // Candle 

    // Buildings
    {
      add_prop_building(Rectangle{   0, 2800, 160, 208});
      add_prop_building(Rectangle{ 160, 2800, 160, 208});
      add_prop_building(Rectangle{ 320, 2800, 160, 208});
      add_prop_building(Rectangle{ 480, 2784, 160, 224});
      add_prop_building(Rectangle{ 640, 2800, 128, 208});
      add_prop_building(Rectangle{ 768, 2800, 160, 208});
      add_prop_building(Rectangle{ 928, 2784, 160, 224});
      add_prop_building(Rectangle{1088, 2848, 128, 160});
      add_prop_building(Rectangle{ 928, 3024, 160, 208});
      add_prop_building(Rectangle{1088, 3024, 160, 208});
    }
    // Buildings

    // Sprites
    {
      add_prop_sprite(SHEET_ID_ENVIRONMENTAL_PARTICLES);
      add_prop_sprite(SHEET_ID_LIGHT_INSECTS);
      add_prop_sprite(SHEET_ID_CANDLE_FX);
      add_prop_sprite(SHEET_ID_GENERIC_LIGHT);
    }
    // Sprites
  }

  // Registering sliders
  {
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
    register_slider(
      SDR_ID_EDITOR_MAP_STAGE_SLC_SLIDER, SDR_TYPE_NUMBER, 
      BTN_ID_EDITOR_SDR_ACTIVE_STAGE_INDEX_DEC,BTN_ID_EDITOR_SDR_ACTIVE_STAGE_INDEX_INC, false, false);
    register_slider(
      SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER, SDR_TYPE_NUMBER, 
      BTN_ID_EDITOR_ACTIVE_TILEMAP_EDIT_LAYER_DEC,BTN_ID_EDITOR_ACTIVE_TILEMAP_EDIT_LAYER_INC, false, false);
    register_slider(
      SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER,  SDR_TYPE_OPTION, 
      BTN_ID_EDITOR_PROP_TYPE_SLC_SLIDER_LEFT, BTN_ID_EDITOR_PROP_TYPE_SLC_SLIDER_RIGHT, false, true);

    gui_slider_add_option(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)TILEMAP_PROP_TYPE_TREE),      1), LOC_TEXT_EDITOR_SDR_PROP_CHANGE_TREE,      "");
    gui_slider_add_option(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)TILEMAP_PROP_TYPE_TOMBSTONE), 1), LOC_TEXT_EDITOR_SDR_PROP_CHANGE_TOMBSTONE, "");
    gui_slider_add_option(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)TILEMAP_PROP_TYPE_STONE),     1), LOC_TEXT_EDITOR_SDR_PROP_CHANGE_STONE,     "");
    gui_slider_add_option(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)TILEMAP_PROP_TYPE_SPIKE),     1), LOC_TEXT_EDITOR_SDR_PROP_CHANGE_SPIKE,     "");
    gui_slider_add_option(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)TILEMAP_PROP_TYPE_SKULL),     1), LOC_TEXT_EDITOR_SDR_PROP_CHANGE_SKULL,     "");
    gui_slider_add_option(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)TILEMAP_PROP_TYPE_PILLAR),    1), LOC_TEXT_EDITOR_SDR_PROP_CHANGE_PILLAR,    "");
    gui_slider_add_option(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)TILEMAP_PROP_TYPE_LAMP),      1), LOC_TEXT_EDITOR_SDR_PROP_CHANGE_LAMP,      "");
    gui_slider_add_option(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)TILEMAP_PROP_TYPE_FENCE),     1), LOC_TEXT_EDITOR_SDR_PROP_CHANGE_FENCE,     "");
    gui_slider_add_option(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)TILEMAP_PROP_TYPE_DETAIL),    1), LOC_TEXT_EDITOR_SDR_PROP_CHANGE_DETAIL,    "");
    gui_slider_add_option(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)TILEMAP_PROP_TYPE_CANDLE),    1), LOC_TEXT_EDITOR_SDR_PROP_CHANGE_CANDLE,    "");
    gui_slider_add_option(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)TILEMAP_PROP_TYPE_BUILDING),  1), LOC_TEXT_EDITOR_SDR_PROP_CHANGE_BUILDING,  "");
    gui_slider_add_option(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)TILEMAP_PROP_TYPE_SPRITE),    1), LOC_TEXT_EDITOR_SDR_PROP_CHANGE_SPRITE,    "");
    ui_set_slider_current_value(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, slider_option {LOC_TEXT_EDITOR_SDR_PROP_CHANGE_TREE, data_pack(DATA_TYPE_I32, data128((i32)TILEMAP_PROP_TYPE_TREE), 1)});
    
    slider * scale_slider = get_slider_by_id(SDR_ID_EDITOR_PROP_SCALE_SLIDER);
    if (scale_slider == nullptr) {
      TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::Cannot found Scale slider");
      return false;
    }
    scale_slider->on_left_button_trigger  = scene_editor_scale_slider_on_left_button_trigger;
    scale_slider->on_right_button_trigger = scene_editor_scale_slider_on_right_button_trigger;
    scale_slider->on_click =                nullptr;

    slider * rotation_slider = get_slider_by_id(SDR_ID_EDITOR_PROP_ROTATION_SLIDER);
    if (rotation_slider == nullptr) {
      TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::Cannot found Rotation slider");
      return false;
    }
    rotation_slider->on_left_button_trigger  = scene_editor_rotation_slider_on_left_button_trigger;
    rotation_slider->on_right_button_trigger = scene_editor_rotation_slider_on_right_button_trigger;
    rotation_slider->on_click =                nullptr;

    slider * zindex_slider = get_slider_by_id(SDR_ID_EDITOR_PROP_ZINDEX_SLIDER);
    if (zindex_slider == nullptr) {
      TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::Cannot found Z-index slider");
      return false;
    }
    zindex_slider->on_left_button_trigger  = scene_editor_zindex_slider_on_left_button_trigger;
    zindex_slider->on_right_button_trigger = scene_editor_zindex_slider_on_right_button_trigger;
    zindex_slider->on_click =                nullptr;

    slider * layer_slider = get_slider_by_id(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER);
    if (layer_slider == nullptr) {
      TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::Cannot found map layer slider");
      return false;
    }
    layer_slider->on_left_button_trigger  = scene_editor_map_layer_slc_slider_on_left_button_trigger;
    layer_slider->on_right_button_trigger = scene_editor_map_layer_slc_slider_on_right_button_trigger;
    layer_slider->on_click =                nullptr;

    slider * stage_slider = get_slider_by_id(SDR_ID_EDITOR_MAP_STAGE_SLC_SLIDER);
    if (stage_slider == nullptr) {
      TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::Cannot found map stage slider");
      return false;
    }
    stage_slider->on_left_button_trigger =  scene_editor_map_stage_slc_slider_on_left_button_trigger;
    stage_slider->on_right_button_trigger = scene_editor_map_stage_slc_slider_on_right_button_trigger;
    stage_slider->on_click =                nullptr;

    slider * prop_type_slider = get_slider_by_id(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER);
    if (prop_type_slider == nullptr) {
      TraceLog(LOG_ERROR, "scene_editor::initialize_scene_editor()::Cannot found map prop type slider");
      return false;
    }
    prop_type_slider->on_left_button_trigger =  scene_editor_map_prop_type_slc_slider_on_left_button_trigger;
    prop_type_slider->on_right_button_trigger = scene_editor_map_prop_type_slc_slider_on_right_button_trigger;
    prop_type_slider->on_click =                nullptr;
  }

  update_tilemap_prop_type();
  begin_scene_editor();

  return true;
}

// UPDATE / RENDER
void update_scene_editor(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::update_scene_editor()::State is not valid");
    return;
  }
  if(!IsWindowFocused()) {
    state->mouse_focus = MOUSE_FOCUS_UNFOCUSED;
    return;
  }
  else state->mouse_focus = MOUSE_FOCUS_MAP;
  state->mouse_pos_screen.x = GetMousePosition().x * get_app_settings()->scale_ratio.at(0);
  state->mouse_pos_screen.y = GetMousePosition().y * get_app_settings()->scale_ratio.at(1);
  state->mouse_pos_world = GetScreenToWorld2D(state->mouse_pos_screen, state->in_camera_metrics->handle);
  state->in_camera_metrics->frustum = se_get_camera_view_rect(state->in_camera_metrics->handle);

  if (state->b_prop_selection_screen_update_prop_sprites) {
    for (size_t iter = 0; iter < state->tilemap_props_sprite.size(); iter++) {
      spritesheet * sprite = __builtin_addressof(state->tilemap_props_sprite.at(iter).sprite);
      ui_update_sprite(sprite);
    }
    state->b_prop_selection_screen_update_prop_sprites = false;
  }
  if (state->selected_prop_sprite_panel_selection_copy.is_initialized && state->selection_type == SLC_TYPE_DROP_PROP_SPRITE) {
    ui_update_sprite(&state->selected_prop_sprite_panel_selection_copy.sprite);
  }

  editor_update_bindings();
  update_map();
  update_user_interface();
}
void render_scene_editor(void) {
  if (state->selected_stage == WORLDMAP_MAINMENU_MAP) {
    render_map_view_on(ZEROVEC2, 1);
  }
  else {
    render_map();
  }
  DrawPixel(0, 0, RED);
}
void render_interface_editor(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::render_interface_editor()::State is not valid");
    return;
  }
  if(state->b_show_tilesheet_tile_selection_screen && !state->b_show_prop_selection_screen) 
  {
    Rectangle& panel_dest = state->tile_selection_panel.dest; 
    gui_panel_scissored(state->tile_selection_panel, false, {
      render_map_palette(state->tile_selection_panel.zoom);
      Vector2 label_anchor  = VECTOR2(panel_dest.x + panel_dest.width * .5f, panel_dest.y + panel_dest.height * .05f);
      Vector2 slider_anchor = VECTOR2(panel_dest.x + panel_dest.width * .5f, panel_dest.y + panel_dest.height * .05f);

      gui_label_grid("Layer", FONT_TYPE_MEDIUM, 10, label_anchor, WHITE, true, true, VECTOR2(-15.f, -4.f));
      gui_slider(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER, slider_anchor, VECTOR2(15.f,-4.f));

      gui_label_grid("Stage", FONT_TYPE_MEDIUM, 10, label_anchor, WHITE, true, true, VECTOR2(-15.f, 4.f));
      gui_slider(SDR_ID_EDITOR_MAP_STAGE_SLC_SLIDER, slider_anchor, VECTOR2(15.f,4.f));
    }); 
  }
  else if(state->b_show_prop_selection_screen && !state->b_show_tilesheet_tile_selection_screen) 
  {
    panel* pnl = &state->prop_selection_panel;
    gui_panel_scissored((*pnl), false, {
      Vector2 label_anchor = VECTOR2(pnl->dest.x + pnl->dest.width * .25f, pnl->dest.y + pnl->dest.height * .05f);
      Vector2 slider_anchor = VECTOR2(pnl->dest.x + pnl->dest.width * .675f, pnl->dest.y + pnl->dest.height * .05f);
      gui_label_grid("Prop Type", FONT_TYPE_MEDIUM, 10, label_anchor, WHITE, true, true, VECTOR2(0.f, 0.f));
      gui_slider(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, slider_anchor, VECTOR2(0.f, 0.f));

      f32 prop_height_count = pnl->buffer.f32[1];
      if (get_slider_current_value(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->data.i32[0] == TILEMAP_PROP_TYPE_SPRITE) {
        state->b_prop_selection_screen_update_prop_sprites = true;
        for (size_t iter = 0; iter < state->tilemap_props_sprite_selected->size(); ++iter) {
          tilemap_prop_sprite& prop = state->tilemap_props_sprite_selected->at(iter);
          Rectangle dest = prop.sprite.coord;
          dest.x = 0;
          dest.y = (pnl->scroll * pnl->buffer.f32[0]) + prop_height_count;
          ui_play_sprite_on_site(&prop.sprite, prop.sprite.tint, dest);
          prop_height_count += dest.height;
        }
      } else {
        for (size_t iter = 0; iter < state->tilemap_props_static_selected->size(); ++iter) {
          const tilemap_prop_static& prop = state->tilemap_props_static_selected->at(iter);
          Rectangle dest = prop.dest;
          dest.x = 0;
          dest.y = (pnl->scroll * pnl->buffer.f32[0]) + prop_height_count;
          gui_draw_texture_id_pro(prop.tex_id, prop.source, dest, prop.tint);
          prop_height_count += prop.dest.height;
        }
      }
      pnl->buffer.f32[0] = prop_height_count;
      pnl->scroll_handle.y = FMAX(pnl->scroll_handle.y, pnl->dest.y + SCREEN_OFFSET.x);
      pnl->scroll_handle.y = FMIN(pnl->scroll_handle.y, pnl->dest.y + pnl->dest.height);
      pnl->scroll = (pnl->scroll_handle.y - pnl->dest.y - SCREEN_OFFSET.x) / (pnl->dest.height - pnl->scroll_handle.height) * -1;
      DrawRectangleRec(pnl->scroll_handle, WHITE);
    });
  }

  if(state->selection_type == SLC_TYPE_SLC_PROP_STATIC) 
  {
    panel* pnl = &state->prop_edit_panel;
    tilemap_prop_static** p_prop = &state->selected_prop_static_map_prop_address;
    if ((*p_prop) == nullptr) {
      return;
    }
    gui_panel_scissored((*pnl), false, {
      ui_set_slider_current_value(SDR_ID_EDITOR_PROP_SCALE_SLIDER, slider_option(TextFormat("%.2f", (*p_prop)->scale), data_pack()));
      Vector2 sdr_center = VECTOR2( pnl->dest.x + pnl->dest.width * .75f, pnl->dest.y + pnl->dest.height * .5f);
      Vector2 label_center = VECTOR2( pnl->dest.x + pnl->dest.width * .25f, pnl->dest.y + pnl->dest.height * .5f);

      gui_label_grid("Scale", FONT_TYPE_MEDIUM, 10, label_center, WHITE, true, true, VECTOR2(0.f, -10.f));
      get_slider_by_id(SDR_ID_EDITOR_PROP_SCALE_SLIDER)->options.at(0).no_localized_text = TextFormat("%.2f", (*p_prop)->scale);
      gui_slider(SDR_ID_EDITOR_PROP_SCALE_SLIDER, sdr_center, VECTOR2(0.f, -10.f));

      gui_label_grid("Rotation", FONT_TYPE_MEDIUM, 10, label_center, WHITE, true, true, VECTOR2(0.f, 0.f));
      get_slider_by_id(SDR_ID_EDITOR_PROP_ROTATION_SLIDER)->options.at(0).no_localized_text = TextFormat("%.1f", (*p_prop)->rotation);
      gui_slider(SDR_ID_EDITOR_PROP_ROTATION_SLIDER, sdr_center, VECTOR2(0.f, 0.f));

      gui_label_grid("Z-Index", FONT_TYPE_MEDIUM, 10, label_center, WHITE, true, true, VECTOR2(0.f, 10.f));
      get_slider_by_id(SDR_ID_EDITOR_PROP_ZINDEX_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", (*p_prop)->zindex);
      gui_slider(SDR_ID_EDITOR_PROP_ZINDEX_SLIDER, sdr_center, VECTOR2(0.f, 10.f));
    });
  }

  if(state->selection_type == SLC_TYPE_SLC_PROP_SPRITE)
  {
    panel* pnl = &state->prop_edit_panel;
    tilemap_prop_sprite** p_prop = &state->selected_prop_sprite_map_prop_address;
    if ((*p_prop) == nullptr) {
      return;
    }
    gui_panel_scissored((*pnl), false, {
      ui_set_slider_current_value(SDR_ID_EDITOR_PROP_SCALE_SLIDER, slider_option(TextFormat("%.2f", (*p_prop)->scale), data_pack()));
      Vector2 sdr_center = VECTOR2( pnl->dest.x + pnl->dest.width * .75f, pnl->dest.y + pnl->dest.height * .5f);
      Vector2 label_center = VECTOR2( pnl->dest.x + pnl->dest.width * .25f, pnl->dest.y + pnl->dest.height * .5f);

      gui_label_grid("Scale", FONT_TYPE_MEDIUM, 10, label_center, WHITE, true, true, VECTOR2(0.f, -10.f));
      get_slider_by_id(SDR_ID_EDITOR_PROP_SCALE_SLIDER)->options.at(0).no_localized_text = TextFormat("%.2f", (*p_prop)->scale);
      gui_slider(SDR_ID_EDITOR_PROP_SCALE_SLIDER, sdr_center, VECTOR2(0.f, -10.f));

      gui_label_grid("Rotation", FONT_TYPE_MEDIUM, 10, label_center, WHITE, true, true, VECTOR2(0.f, 0.f));
      get_slider_by_id(SDR_ID_EDITOR_PROP_ROTATION_SLIDER)->options.at(0).no_localized_text = TextFormat("%.1f", (*p_prop)->sprite.rotation);
      gui_slider(SDR_ID_EDITOR_PROP_ROTATION_SLIDER, sdr_center, VECTOR2(0.f, 0.f));

      gui_label_grid("Z-Index", FONT_TYPE_MEDIUM, 10, label_center, WHITE, true, true, VECTOR2(0.f, 10.f));
      get_slider_by_id(SDR_ID_EDITOR_PROP_ZINDEX_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", (*p_prop)->zindex);
      gui_slider(SDR_ID_EDITOR_PROP_ZINDEX_SLIDER, sdr_center, VECTOR2(0.f, 10.f));
    });
  }

  switch (state->selection_type) {
    case SLC_TYPE_TILE: {
      if (state->selected_sheet == nullptr) {
        return;
      }
      _render_tile_on_pos(&state->selected_tile, state->mouse_pos_screen, state->selected_sheet);
      break;
    }
    case SLC_TYPE_DROP_PROP_STATIC: {
      if (!state->selected_prop_static_panel_selection_copy.is_initialized) {
        return;
      }
      tilemap_prop_static* p_prop = &state->selected_prop_static_panel_selection_copy;
      if (!p_prop->is_initialized) {
        return;
      }
      gui_draw_texture_id_pro(
        p_prop->tex_id, p_prop->source,
        Rectangle { state->mouse_pos_screen.x, state->mouse_pos_screen.y, p_prop->dest.width, p_prop->dest.height}, p_prop->tint,
        Vector2 { p_prop->dest.width * .5f, p_prop->dest.height * .5f}
      );
      break;
    }
    case SLC_TYPE_DROP_PROP_SPRITE: {
      if (!state->selected_prop_sprite_panel_selection_copy.is_initialized) {
        return;
      }
      tilemap_prop_sprite& slc_prop_sprite = state->selected_prop_sprite_panel_selection_copy;
      ui_play_sprite_on_site(&slc_prop_sprite.sprite, slc_prop_sprite.sprite.tint,
        Rectangle {
          state->mouse_pos_screen.x, state->mouse_pos_screen.y, 
          slc_prop_sprite.sprite.coord.width, slc_prop_sprite.sprite.coord.width
        }
      );
      break;
    }
    case SLC_TYPE_SLC_PROP_STATIC: {
      if (state->selected_prop_static_map_prop_address == nullptr || state->in_camera_metrics == nullptr) {
        return;
      }
      tilemap_prop_static** slc_prop_static = &state->selected_prop_static_map_prop_address;
      Vector2 prop_pos = GetWorldToScreen2D(Vector2{ (*slc_prop_static)->dest.x, (*slc_prop_static)->dest.y}, state->in_camera_metrics->handle);
      f32 relative_width = (*slc_prop_static)->dest.width * (*slc_prop_static)->scale * state->in_camera_metrics->handle.zoom;
      f32 relative_height = (*slc_prop_static)->dest.height * (*slc_prop_static)->scale * state->in_camera_metrics->handle.zoom;
      
      prop_pos.x -= relative_width * 0.5f; // To center to its origin
      prop_pos.y -= relative_height * 0.5f;
      DrawRectangleLines(prop_pos.x, prop_pos.y, relative_width, relative_height, WHITE);
      prop_pos.x += relative_width / 2.f - PROP_DRAG_HANDLE_DIM_DIV2;
      prop_pos.y += relative_height / 2.f - PROP_DRAG_HANDLE_DIM_DIV2;
      DrawRectangle(prop_pos.x, prop_pos.y, PROP_DRAG_HANDLE_DIM, PROP_DRAG_HANDLE_DIM, WHITE);
      break;
    }
    case SLC_TYPE_SLC_PROP_SPRITE: {
      if (state->selected_prop_sprite_map_prop_address == nullptr || state->in_camera_metrics == nullptr) {
        return;
      }
      tilemap_prop_sprite** slc_prop_sprite = &state->selected_prop_sprite_map_prop_address;
      Vector2 prop_pos = GetWorldToScreen2D(Vector2{ (*slc_prop_sprite)->sprite.coord.x, (*slc_prop_sprite)->sprite.coord.y}, state->in_camera_metrics->handle);
      f32 relative_width = (*slc_prop_sprite)->sprite.coord.width * (*slc_prop_sprite)->scale * state->in_camera_metrics->handle.zoom;
      f32 relative_height = (*slc_prop_sprite)->sprite.coord.height * (*slc_prop_sprite)->scale * state->in_camera_metrics->handle.zoom;
      
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

void add_prop(texture_id source_tex, tilemap_prop_types type, Rectangle source, f32 scale, Color tint) {
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
  tilemap_prop_static prop = tilemap_prop_static();

  prop.id = INVALID_IDU32;
  prop.tex_id = source_tex;
  prop.prop_type = type;
  
  prop.source = source;
  prop.dest = Rectangle {0, 0, source.width * scale, source.height * scale};
  prop.scale = scale;
  prop.tint = tint;
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
void add_prop(tilemap_prop_types type, spritesheet_id sprite_id, f32 scale, Color tint) {
  if (sprite_id >= SHEET_ID_SPRITESHEET_TYPE_MAX || sprite_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "scene_editor::add_prop()::Provided sprite id out of bound");
    return;
  }
  tilemap_prop_sprite prop = tilemap_prop_sprite();

  prop.id = INVALID_IDU32;
  prop.prop_type = type;
  prop.scale = scale;
  prop.sprite.sheet_id = sprite_id;
  prop.sprite.tint = tint;
  
  ui_set_sprite(&prop.sprite, true, false);
  prop.sprite.rotation = 0.f;

  prop.is_initialized = true;

  switch (type) {
    case TILEMAP_PROP_TYPE_SPRITE:      { state->tilemap_props_sprite.push_back(prop); break; }
    default: { 
      TraceLog(LOG_WARNING, "scene_editor::add_prop()::Unsupported tilemap sprite type");
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
      } else { return; }
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
      if (get_slider_current_value(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->data.i32[0] == TILEMAP_PROP_TYPE_SPRITE) {
        for (size_t iter=0; iter< state->tilemap_props_sprite_selected->size() && h > 0; ++iter) {
          if(h - state->tilemap_props_sprite_selected->at(iter).sprite.current_frame_rect.height < 0) {
            state->selected_prop_sprite_panel_selection_copy = state->tilemap_props_sprite_selected->at(iter);
            state->selected_prop_sprite_panel_selection_copy.sprite.origin = Vector2 {
              state->selected_prop_sprite_panel_selection_copy.sprite.current_frame_rect.width * .5f,
              state->selected_prop_sprite_panel_selection_copy.sprite.current_frame_rect.height * .5f
            };
            state->selection_type = SLC_TYPE_DROP_PROP_SPRITE;
            break;
          }
          h -= state->tilemap_props_sprite_selected->at(iter).sprite.current_frame_rect.height;
        }
      }
      else {
        for (size_t iter=0; iter < state->tilemap_props_static_selected->size() && h > 0; ++iter) {
          if(h - state->tilemap_props_static_selected->at(iter).source.height < 0) {
            state->selected_prop_static_panel_selection_copy = state->tilemap_props_static_selected->at(iter);
            state->selected_prop_static_panel_selection_copy.origin = Vector2 {
              state->selected_prop_static_panel_selection_copy.dest.width * .5f,
              state->selected_prop_static_panel_selection_copy.dest.height * .5f
            };
            state->selection_type = SLC_TYPE_DROP_PROP_STATIC;
            break;
          }
          h -= state->tilemap_props_static_selected->at(iter).source.height;
        }
      }
    }
  }
  else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->mouse_focus == MOUSE_FOCUS_MAP) 
  {
    switch (state->selection_type) {
      case SLC_TYPE_UNSELECTED: {
        tilemap_prop_address _prop_address = get_map_prop_by_pos(state->mouse_pos_world);
        if (_prop_address.type <= TILEMAP_PROP_TYPE_UNDEFINED || _prop_address.type >= TILEMAP_PROP_TYPE_MAX) break;
        if (_prop_address.type != TILEMAP_PROP_TYPE_SPRITE && _prop_address.data.prop_static != nullptr) {
          state->selected_prop_static_map_prop_address = _prop_address.data.prop_static;
          state->selection_type = SLC_TYPE_SLC_PROP_STATIC;
        }
        if (_prop_address.type == TILEMAP_PROP_TYPE_SPRITE && _prop_address.data.prop_sprite != nullptr) {
          state->selected_prop_sprite_map_prop_address = _prop_address.data.prop_sprite;
          state->selection_type = SLC_TYPE_SLC_PROP_SPRITE;
        }
        break;
      }
      case SLC_TYPE_DROP_PROP_STATIC: {
        state->selected_prop_static_panel_selection_copy.dest.x = state->mouse_pos_world.x;
        state->selected_prop_static_panel_selection_copy.dest.y = state->mouse_pos_world.y;
        add_prop_curr_map(state->selected_prop_static_panel_selection_copy);
        break;
      }
      case SLC_TYPE_DROP_PROP_SPRITE: {
        state->selected_prop_sprite_panel_selection_copy.sprite.coord.x = state->mouse_pos_world.x;
        state->selected_prop_sprite_panel_selection_copy.sprite.coord.y = state->mouse_pos_world.y;
        add_prop_curr_map(state->selected_prop_sprite_panel_selection_copy);
        break;
      }
      case SLC_TYPE_SLC_PROP_STATIC: {
        state->b_dragging_prop = false;
        break;
      }
      case SLC_TYPE_SLC_PROP_SPRITE: {
        state->b_dragging_prop = false;
        break;
      }
      default: break;
    }
  }
  else if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && state->mouse_focus == MOUSE_FOCUS_MAP) 
  {
    switch (state->selection_type) {
      case SLC_TYPE_SLC_PROP_STATIC: {
        tilemap_prop_static* prop = get_map_prop_static_by_id(state->selected_prop_static_map_prop_address->id);
        if (prop == nullptr) {
          TraceLog(LOG_ERROR, "scene_editor::editor_update_mouse_bindings()::Prop static:%d cannot found", state->selected_prop_sprite_map_prop_address->id);
          break;
        }
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
          state->selected_prop_static_map_prop_address->dest = prop->dest;
        }
        break;
      }
      case SLC_TYPE_SLC_PROP_SPRITE: {
        tilemap_prop_sprite* prop = get_map_prop_sprite_by_id(state->selected_prop_sprite_map_prop_address->id);
        if (prop == nullptr) {
          TraceLog(LOG_ERROR, "scene_editor::editor_update_mouse_bindings()::Prop sprite:%d cannot found", state->selected_prop_sprite_map_prop_address->id);
          break;
        }
        Rectangle drag_handle = Rectangle {
          prop->sprite.coord.x - PROP_DRAG_HANDLE_DIM_DIV2, prop->sprite.coord.y - PROP_DRAG_HANDLE_DIM_DIV2, 
          PROP_DRAG_HANDLE_DIM, PROP_DRAG_HANDLE_DIM
        };
        if(!state->b_dragging_prop && CheckCollisionPointRec(state->mouse_pos_world, drag_handle)) {
          state->b_dragging_prop = true;
        }
        if (state->b_dragging_prop) {
          prop->sprite.coord.x = state->mouse_pos_world.x;
          prop->sprite.coord.y = state->mouse_pos_world.y;
          state->selected_prop_sprite_map_prop_address->sprite.coord = prop->sprite.coord;
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
    state->selected_tile = tile();
    state->selected_prop_static_map_prop_address = nullptr;
    state->selected_prop_sprite_map_prop_address = nullptr;
    state->selected_prop_static_panel_selection_copy = tilemap_prop_static();
    state->selected_prop_sprite_panel_selection_copy = tilemap_prop_sprite();
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
    if (state->mouse_focus == MOUSE_FOCUS_MAP && state->selection_type == SLC_TYPE_SLC_PROP_STATIC && !state->b_dragging_prop) {
      if(!_remove_prop_cur_map_by_id(state->selected_prop_static_map_prop_address)) {
        TraceLog(LOG_WARNING, "scene_editor::editor_update_keyboard_bindings()::Removing property failed.");
      }
      state->selected_prop_static_map_prop_address = nullptr;
      state->selection_type = SLC_TYPE_UNSELECTED;
    }
    if (state->mouse_focus == MOUSE_FOCUS_MAP && state->selection_type == SLC_TYPE_SLC_PROP_SPRITE && !state->b_dragging_prop) {
      if(!_remove_prop_cur_map_by_id(state->selected_prop_sprite_map_prop_address)) {
        TraceLog(LOG_WARNING, "scene_editor::editor_update_keyboard_bindings()::Removing property failed.");
      }
      state->selected_prop_sprite_map_prop_address = nullptr;
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
  get_slider_by_id(SDR_ID_EDITOR_MAP_STAGE_SLC_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", state->selected_stage);
  get_slider_by_id(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", state->edit_layer);

  event_fire(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, event_context(data128(state->target.x, state->target.y)));
}
void end_scene_editor(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::end_scene_editor()::State is not valid");
    return;
  }
  state->target = ZEROVEC2;
  state->selection_type = SLC_TYPE_UNSELECTED;
  state->mouse_focus = MOUSE_FOCUS_UNFOCUSED;
  state->edit_layer = 0u;
  state->selected_stage = 0u;
  state->tilemap_props_sprite_selected = nullptr;
  state->tilemap_props_static_selected = nullptr;
  state->b_show_tilesheet_tile_selection_screen = false;
  state->b_prop_selection_screen_update_prop_sprites = false;
  state->b_show_prop_selection_screen = false;
  state->b_show_prop_edit_screen = false;
  state->b_dragging_prop = false;
  state->b_show_pause_menu = false;
  state->selected_tile = tile();
  state->selected_prop_static_map_prop_address = nullptr;
  state->selected_prop_sprite_map_prop_address = nullptr;
  state->selected_prop_sprite_panel_selection_copy = tilemap_prop_sprite();
  state->selected_prop_static_panel_selection_copy = tilemap_prop_static();
  state->selected_sheet = nullptr;
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
  i32 prop_type_value = get_slider_current_value(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->data.i32[0];
  tilemap_prop_types selected_prop_type = static_cast<tilemap_prop_types>(prop_type_value);
  switch (selected_prop_type) {
    case TILEMAP_PROP_TYPE_TREE:      { state->tilemap_props_static_selected = &state->tilemap_props_trees;      break; }
    case TILEMAP_PROP_TYPE_TOMBSTONE: { state->tilemap_props_static_selected = &state->tilemap_props_tombstones; break; }
    case TILEMAP_PROP_TYPE_STONE:     { state->tilemap_props_static_selected = &state->tilemap_props_stones;     break; }
    case TILEMAP_PROP_TYPE_SPIKE:     { state->tilemap_props_static_selected = &state->tilemap_props_spikes;     break; }
    case TILEMAP_PROP_TYPE_SKULL:     { state->tilemap_props_static_selected = &state->tilemap_props_skulls;     break; }
    case TILEMAP_PROP_TYPE_PILLAR:    { state->tilemap_props_static_selected = &state->tilemap_props_pillars;    break; }
    case TILEMAP_PROP_TYPE_LAMP:      { state->tilemap_props_static_selected = &state->tilemap_props_lamps;      break; }
    case TILEMAP_PROP_TYPE_FENCE:     { state->tilemap_props_static_selected = &state->tilemap_props_fence;      break; }
    case TILEMAP_PROP_TYPE_DETAIL:    { state->tilemap_props_static_selected = &state->tilemap_props_details;    break; }
    case TILEMAP_PROP_TYPE_CANDLE:    { state->tilemap_props_static_selected = &state->tilemap_props_candles;    break; }
    case TILEMAP_PROP_TYPE_BUILDING:  { state->tilemap_props_static_selected = &state->tilemap_props_buildings;  break; }
    case TILEMAP_PROP_TYPE_SPRITE:    { state->tilemap_props_sprite_selected = &state->tilemap_props_sprite;     break; }
    default: { 
      TraceLog(LOG_WARNING, "scene_editor::initialize_scene_editor()::Unsupported tilemap prop type");
      ui_set_slider_current_index(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, 1);
      state->tilemap_props_static_selected = &state->tilemap_props_trees;
      break; 
    }
  }
}

bool scene_editor_scale_slider_on_left_button_trigger() {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::scene_editor_scale_slider_on_left_button_trigger():: State is not valid");
    return false;
  }
  if (state->selected_prop_static_map_prop_address != nullptr) {
    state->selected_prop_static_map_prop_address->scale -= .15f;
    return true;
  }
  if (state->selected_prop_sprite_map_prop_address != nullptr) {
    tilemap_prop_sprite * map_prop_ptr = state->selected_prop_sprite_map_prop_address;
    map_prop_ptr->scale -= .15f;
    
    map_prop_ptr->sprite.coord.width = map_prop_ptr->sprite.current_frame_rect.width * map_prop_ptr->scale;
    map_prop_ptr->sprite.coord.height = map_prop_ptr->sprite.current_frame_rect.height * map_prop_ptr->scale;
    map_prop_ptr->sprite.origin.x = map_prop_ptr->sprite.coord.width  * .5f;
    map_prop_ptr->sprite.origin.y = map_prop_ptr->sprite.coord.height * .5f;
    return true;
  }

  return false;
}
bool scene_editor_scale_slider_on_right_button_trigger() {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::scene_editor_scale_slider_on_right_button_trigger():: State is not valid");
    return false;
  }
  if (state->selected_prop_static_map_prop_address != nullptr) {
    state->selected_prop_static_map_prop_address->scale += .15f;
    return true;
  }
  if (state->selected_prop_sprite_map_prop_address != nullptr) {
    tilemap_prop_sprite * map_prop_ptr = state->selected_prop_sprite_map_prop_address;
    map_prop_ptr->scale += .15f;
    
    map_prop_ptr->sprite.coord.width = map_prop_ptr->sprite.current_frame_rect.width * map_prop_ptr->scale;
    map_prop_ptr->sprite.coord.height = map_prop_ptr->sprite.current_frame_rect.height * map_prop_ptr->scale;
    map_prop_ptr->sprite.origin.x = map_prop_ptr->sprite.coord.width  * .5f;
    map_prop_ptr->sprite.origin.y = map_prop_ptr->sprite.coord.height * .5f;
    return true;
  }

  return false;
}
bool scene_editor_rotation_slider_on_left_button_trigger() {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::scene_editor_rotation_slider_on_left_button_trigger():: State is not valid");
    return false;
  }
  if (state->selected_prop_static_map_prop_address != nullptr) {
    state->selected_prop_static_map_prop_address->rotation -= 10.f;
    if(state->selected_prop_static_map_prop_address->rotation < 0.f) {
      state->selected_prop_static_map_prop_address->rotation += 360.f;
    }
    state->selected_prop_static_map_prop_address->rotation = FCLAMP(state->selected_prop_static_map_prop_address->rotation, 0, 360);
    return true;
  }
  if (state->selected_prop_sprite_map_prop_address != nullptr) {
    state->selected_prop_sprite_map_prop_address->sprite.rotation -= 10.f;
    if(state->selected_prop_sprite_map_prop_address->sprite.rotation < 0.f) {
      state->selected_prop_sprite_map_prop_address->sprite.rotation += 360.f;
    }
    state->selected_prop_sprite_map_prop_address->sprite.rotation = FCLAMP(state->selected_prop_sprite_map_prop_address->sprite.rotation, 0, 360);
    return true;
  }

  return false;
}
bool scene_editor_rotation_slider_on_right_button_trigger() {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::scene_editor_rotation_slider_on_right_button_trigger():: State is not valid");
    return false;
  }
  if (state->selected_prop_static_map_prop_address != nullptr) {
    state->selected_prop_static_map_prop_address->rotation += 10.f;
    if(state->selected_prop_static_map_prop_address->rotation > 360.f) {
      state->selected_prop_static_map_prop_address->rotation -= 360.f;
    }
    state->selected_prop_static_map_prop_address->rotation = FCLAMP(state->selected_prop_static_map_prop_address->rotation, 0, 360);
    return true;
  }
  if (state->selected_prop_sprite_map_prop_address != nullptr) {
    state->selected_prop_sprite_map_prop_address->sprite.rotation += 10.f;
    if(state->selected_prop_sprite_map_prop_address->sprite.rotation > 360.f) {
      state->selected_prop_sprite_map_prop_address->sprite.rotation -= 360.f;
    }
    state->selected_prop_sprite_map_prop_address->sprite.rotation = FCLAMP(state->selected_prop_sprite_map_prop_address->sprite.rotation, 0, 360);
    return true;
  }

  return false;
}
bool scene_editor_zindex_slider_on_left_button_trigger() {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::scene_editor_zindex_slider_on_left_button_trigger():: State is not valid");
    return false;
  }
  if (state->selected_prop_static_map_prop_address != nullptr) {
    state->selected_prop_static_map_prop_address->zindex -= 1;
    if(state->selected_prop_static_map_prop_address->zindex < 0) {
      state->selected_prop_static_map_prop_address->zindex = 0;
    }
    refresh_render_queue(state->selected_stage);
    return true;
  }
  if (state->selected_prop_sprite_map_prop_address != nullptr) {
    state->selected_prop_sprite_map_prop_address->zindex -= 1;
    if(state->selected_prop_sprite_map_prop_address->zindex < 0) {
      state->selected_prop_sprite_map_prop_address->zindex = 0;
    }
    refresh_render_queue(state->selected_stage);
    return true;
  }

  return false;
}
bool scene_editor_zindex_slider_on_right_button_trigger() {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::scene_editor_zindex_slider_on_right_button_trigger():: State is not valid");
    return false;
  }
  if (state->selected_prop_static_map_prop_address != nullptr) {
    i16 old_zindex_backup = state->selected_prop_static_map_prop_address->zindex;

    state->selected_prop_static_map_prop_address->zindex += 1;
    if(state->selected_prop_static_map_prop_address->zindex >= MAX_Z_INDEX_SLOT) {
      state->selected_prop_static_map_prop_address->zindex = MAX_Z_INDEX_SLOT-1;
    }
    tilemap_prop_static& map_prop_ref = *state->selected_prop_static_map_prop_address;
    change_prop_zindex(map_prop_ref.prop_type, map_prop_ref.id, old_zindex_backup, map_prop_ref.zindex);
    return true;
  }
  if (state->selected_prop_sprite_map_prop_address != nullptr) {
    i16 old_zindex_backup = state->selected_prop_sprite_map_prop_address->zindex;

    state->selected_prop_sprite_map_prop_address->zindex += 1;
    if(state->selected_prop_sprite_map_prop_address->zindex >= MAX_Z_INDEX_SLOT) {
      state->selected_prop_sprite_map_prop_address->zindex = MAX_Z_INDEX_SLOT-1;
    }
    tilemap_prop_sprite& map_prop_ref = *state->selected_prop_sprite_map_prop_address;
    change_prop_zindex(map_prop_ref.prop_type, map_prop_ref.id, old_zindex_backup, map_prop_ref.zindex);
    return true;
  }

  return false;
}

bool scene_editor_map_layer_slc_slider_on_left_button_trigger() {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::scene_editor_map_layer_slc_slider_on_left_button_trigger():: State is not valid");
    return false;
  }
  state->edit_layer--;
  if (state->edit_layer > MAX_TILEMAP_LAYERS-1) {
    state->edit_layer = MAX_TILEMAP_LAYERS-1;
  }
  FCLAMP(state->edit_layer, 0, MAX_TILEMAP_LAYERS-1);
  get_slider_by_id(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", state->edit_layer);
  return true; 
}
bool scene_editor_map_layer_slc_slider_on_right_button_trigger() { 
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::scene_editor_map_layer_slc_slider_on_right_button_trigger():: State is not valid");
    return false;
  }
  state->edit_layer++;
  if (state->edit_layer > MAX_TILEMAP_LAYERS-1) {
    state->edit_layer = 0;
  }
  FCLAMP(state->edit_layer, 0, MAX_TILEMAP_LAYERS-1);
  get_slider_by_id(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", state->edit_layer);
  return true; 
}
bool scene_editor_map_stage_slc_slider_on_left_button_trigger() {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::scene_editor_map_stage_slc_slider_on_left_button_trigger():: State is not valid");
    return false;
  }
  state->selected_stage--;
  if (state->selected_stage >= MAX_WORLDMAP_LOCATIONS) {
    state->selected_stage = MAX_WORLDMAP_LOCATIONS-1;
  }
  FCLAMP(state->selected_stage, 0, MAX_WORLDMAP_LOCATIONS-1);
  set_worldmap_location(state->selected_stage);
  get_slider_by_id(SDR_ID_EDITOR_MAP_STAGE_SLC_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", state->selected_stage);
  return true; 
}
bool scene_editor_map_stage_slc_slider_on_right_button_trigger() {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::scene_editor_map_stage_slc_slider_on_right_button_trigger():: State is not valid");
    return false;
  }
  state->selected_stage++;
  if (state->selected_stage >= MAX_WORLDMAP_LOCATIONS) {
    state->selected_stage = 0;
  }
  FCLAMP(state->selected_stage, 0, MAX_WORLDMAP_LOCATIONS-1);
  set_worldmap_location(state->selected_stage);
  get_slider_by_id(SDR_ID_EDITOR_MAP_STAGE_SLC_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", state->selected_stage);
  return true; 
}
bool scene_editor_map_prop_type_slc_slider_on_left_button_trigger() {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::scene_editor_map_prop_type_slc_slider_on_left_button_trigger():: State is not valid");
    return false;
  }
  u16& current_index = get_slider_by_id(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->current_value;

  current_index--;
  if (current_index < get_slider_by_id(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->min_value) {
    current_index = get_slider_by_id(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->min_value;
  }

  update_tilemap_prop_type();
  return true; 
}
bool scene_editor_map_prop_type_slc_slider_on_right_button_trigger() {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_editor::scene_editor_map_prop_type_slc_slider_on_right_button_trigger():: State is not valid");
    return false;
  }
  u16& current_index = get_slider_by_id(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->current_value;

  current_index++;
  if (current_index >= get_slider_by_id(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->max_value) {
    current_index = get_slider_by_id(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->max_value-1;
  }

  update_tilemap_prop_type();
  return true;
}

#undef PROP_DRAG_HANDLE_DIM
#undef PROP_DRAG_HANDLE_DIM_DIV2
#undef PROP_PANEL_PROP_DRAW_STARTING_HEIGHT
#undef add_prop_tree
#undef add_prop_tombstone
#undef add_prop_stone
#undef add_prop_spike
#undef add_prop_skull
#undef add_prop_pillar
#undef add_prop_lamp
#undef add_prop_fence
#undef add_prop_detail
#undef add_prop_candle
#undef add_prop_building
#undef add_prop_sprite
