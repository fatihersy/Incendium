#include "scene_editor.h"
#include <algorithm>
#include <defines.h>
#include <settings.h>
#include <loc_types.h>

#include "core/event.h"
#include "core/fmemory.h"
#include "core/logger.h"

#include "game/user_interface.h"
#include "game/world.h"
#include "game/camera.h"
#include "game/resource.h"

#define SCROLL_HANDLE_STARTING_HEIGHT 100.f

#define TILESHEET_PANEL_SHEET_DRAW_STARTING_HEIGHT 100.f

#define PROP_DRAG_HANDLE_DIM 16
#define PROP_DRAG_HANDLE_DIM_DIV2 PROP_DRAG_HANDLE_DIM/2.f
#define PROP_PANEL_PROP_DRAW_STARTING_HEIGHT SCROLL_HANDLE_STARTING_HEIGHT

#define MAP_COLLISION_DRAG_HANDLE_DIM 16
#define MAP_COLLISION_DRAG_HANDLE_DIM_DIV2 PROP_DRAG_HANDLE_DIM * .5f
#define MAP_COLLISION_PANEL_COLLISION_DRAW_STARTING_HEIGHT SCROLL_HANDLE_STARTING_HEIGHT

typedef enum editor_state_selection_type {
  SLC_TYPE_UNSELECTED,
  SLC_TYPE_TILE,
  SLC_TYPE_DROP_PROP_STATIC,
  SLC_TYPE_SLC_PROP_STATIC,
  SLC_TYPE_DROP_PROP_SPRITE,
  SLC_TYPE_SLC_PROP_SPRITE,
  SLC_TYPE_SLC_MAP_COLLISION,
  SLC_TYPE_DROP_COLLISION_POSITION,
  SLC_TYPE_DROP_COLLISION_DIMENTIONS
} editor_state_selection_type;

typedef enum editor_state_mouse_focus {
  MOUSE_FOCUS_UNFOCUSED, // While window unfocused or not on the screen
  MOUSE_FOCUS_MAP,
  MOUSE_FOCUS_TILE_SELECTION_PANEL,
  MOUSE_FOCUS_PROP_SELECTION_PANEL,
  MOUSE_FOCUS_COLLISION_PLACEMENT_PANEL,
} editor_state_mouse_focus;

typedef struct scene_editor_state {
  // INFO: Those are will be updated each frame no matter what their default value is
  Vector2 mouse_pos_world {};
  Vector2 mouse_pos_screen {};
  Vector2 target {};
  editor_state_mouse_focus mouse_focus {};
  // INFO: Those are will be updated each frame no matter what their default value is

  // WARN: RESET EVERYTHING IN THERE WHEN CLOSING EDITOR
  bool b_show_tilesheet_tile_selection_screen {};
  bool b_show_prop_selection_screen {};
  bool b_show_prop_edit_screen {};
  bool b_show_collision_placement_screen {};
  bool b_dragging_map_element {};
  bool b_show_pause_menu {};
  bool b_prop_selection_screen_update_prop_sprites {};
  std::vector<tilemap_prop_static> * tilemap_props_static_selected; // INFO: The prop list that shown on props panel
  std::vector<tilemap_prop_sprite> * tilemap_props_sprite_selected;
  tilemap_prop_static* selected_prop_static_map_prop_address; // To manipulate
  tilemap_prop_sprite* selected_prop_sprite_map_prop_address;
  tilemap_prop_static  selected_prop_static_panel_selection_copy; // To place
  tilemap_prop_sprite  selected_prop_sprite_panel_selection_copy;
  map_collision* sel_map_coll_addr_from_map;
  map_collision map_collision_buffer_to_place;
  tile selected_tile;
  u16 edit_layer {};
  u16 selected_stage {};
  editor_state_selection_type selection_type;
  ui_fade_control_system se_fade;
  // WARN: RESET EVERYTHING IN THERE WHEN CLOSING EDITOR

  // WARN: Those variables will be initialized one time in the initialize function
  tilesheet_type default_sheet;
  std::vector<tilemap_prop_static> * tilemap_props_trees;
  std::vector<tilemap_prop_static> * tilemap_props_tombstones;
  std::vector<tilemap_prop_static> * tilemap_props_stones;
  std::vector<tilemap_prop_static> * tilemap_props_spikes;
  std::vector<tilemap_prop_static> * tilemap_props_skulls;
  std::vector<tilemap_prop_static> * tilemap_props_pillars;
  std::vector<tilemap_prop_static> * tilemap_props_lamps;
  std::vector<tilemap_prop_static> * tilemap_props_fence;
  std::vector<tilemap_prop_static> * tilemap_props_details;
  std::vector<tilemap_prop_static> * tilemap_props_candles;
  std::vector<tilemap_prop_static> * tilemap_props_buildings;
  std::vector<tilemap_prop_sprite> * tilemap_props_sprite;
  tilemap ** active_map_ptr;
  const camera_metrics * in_camera_metrics;
  const app_settings   * in_app_settings;
  std::array<worldmap_stage, MAX_WORLDMAP_LOCATIONS> worldmap_locations;
  panel prop_selection_panel;
  panel tile_selection_panel;
  panel collision_placement_panel;
  panel prop_edit_panel;
  // WARN: This variables will be initialized one time in the initialize function

  scene_editor_state(void) {
    this->mouse_focus = MOUSE_FOCUS_UNFOCUSED;
    this->tilemap_props_static_selected = nullptr;
    this->tilemap_props_sprite_selected = nullptr;
    this->selected_prop_static_map_prop_address = nullptr;
    this->selected_prop_sprite_map_prop_address = nullptr;
    this->sel_map_coll_addr_from_map = nullptr;
    this->selection_type = SLC_TYPE_UNSELECTED;
    this->default_sheet = TILESHEET_TYPE_UNSPECIFIED;
    this->tilemap_props_trees = nullptr;
    this->tilemap_props_tombstones = nullptr;
    this->tilemap_props_stones = nullptr;
    this->tilemap_props_spikes = nullptr;
    this->tilemap_props_skulls = nullptr;
    this->tilemap_props_pillars = nullptr;
    this->tilemap_props_lamps = nullptr;
    this->tilemap_props_fence = nullptr;
    this->tilemap_props_details = nullptr;
    this->tilemap_props_candles = nullptr;
    this->tilemap_props_buildings = nullptr;
    this->tilemap_props_sprite = nullptr;
    this->active_map_ptr = nullptr;
    this->in_camera_metrics = nullptr;
    this->in_app_settings = nullptr;
  }
} scene_editor_state;

static scene_editor_state * state = nullptr;

[[__nodiscard__]] bool begin_scene_editor(bool fade_in);

constexpr void editor_update_bindings(void);
constexpr void editor_update_movement(void);
constexpr void editor_update_mouse_bindings(void);
constexpr void editor_update_keyboard_bindings(void);
constexpr i32 map_prop_id_to_index(u16 id);
constexpr void update_tilemap_prop_type(void);
constexpr void scene_editor_selection_cleanup(void);

constexpr bool scene_editor_scale_slider_on_left_button_trigger(void);
constexpr bool scene_editor_scale_slider_on_right_button_trigger(void);
constexpr bool scene_editor_rotation_slider_on_left_button_trigger(void);
constexpr bool scene_editor_rotation_slider_on_right_button_trigger(void);
constexpr bool scene_editor_zindex_slider_on_left_button_trigger(void);
constexpr bool scene_editor_zindex_slider_on_right_button_trigger(void);
constexpr bool scene_editor_map_layer_slc_slider_on_left_button_trigger(void);
constexpr bool scene_editor_map_layer_slc_slider_on_right_button_trigger(void);
constexpr bool scene_editor_map_stage_slc_slider_on_left_button_trigger(void);
constexpr bool scene_editor_map_stage_slc_slider_on_right_button_trigger(void);
constexpr bool scene_editor_map_prop_type_slc_slider_on_left_button_trigger(void);
constexpr bool scene_editor_map_prop_type_slc_slider_on_right_button_trigger(void);

constexpr bool scene_editor_is_map_prop_y_based_checkbox_on_change_trigger(void);
void se_begin_fadeout(data128 data, void(*on_change_complete)(data128));
void se_begin_fadein(data128 data, void(*on_change_complete)(data128));

#define SE_BASE_RENDER_WIDTH state->in_app_settings->render_width
#define SE_BASE_RENDER_HEIGHT state->in_app_settings->render_height
#define EDITOR_FADE_DURATION .6f

[[__nodiscard__]] bool initialize_scene_editor(const app_settings *const _in_app_settings, bool fade_in) {
  if (state and state != nullptr) {
    return begin_scene_editor(fade_in);
  }
  state = (scene_editor_state*)allocate_memory_linear(sizeof(scene_editor_state), true);
  if (not state or state == nullptr) {
    IERROR("scene_editor::initialize_scene_editor()::State allocation failed!");
    return false;
  }
  *state = scene_editor_state();

  if (not _in_app_settings or _in_app_settings == nullptr) {
    IERROR("scene_editor::initialize_scene_editor()::App settings pointer is invalid");
    return false;
  }
  state->in_app_settings = _in_app_settings;

  if(not create_camera(state->in_app_settings->render_width_div2, state->in_app_settings->render_height_div2, state->in_app_settings->render_width, state->in_app_settings->render_height)) {
    IERROR("scene_editor::initialize_scene_editor()::Creating camera failed");
    return false;
  }
  state->in_camera_metrics = get_in_game_camera();
  if (not state->in_camera_metrics or state->in_camera_metrics == nullptr) {
    IERROR("scene_editor::initialize_scene_editor()::Camera pointer is invalid");
    return false;
  }
  if (not world_system_begin(state->in_camera_metrics)) {
    IERROR("scene_editor::initialize_scene_editor()::World system begin failed");
    return false;
  }
  state->active_map_ptr = get_active_map_ptr();

  if(not user_interface_system_initialize(get_in_game_camera())) {
    IERROR("scene_editor::initialize_scene_editor()::User interface failed to initialize!");
    return false;
  }
  state->worldmap_locations = get_worldmap_locations();
  state->default_sheet = TILESHEET_TYPE_MAP;
  state->tile_selection_panel = panel();
  state->tile_selection_panel.signal_state = BTN_STATE_HOVER;
  state->tile_selection_panel.dest = Rectangle {0, 0, SE_BASE_RENDER_WIDTH * .45f, static_cast<f32>(SE_BASE_RENDER_WIDTH)};

  state->collision_placement_panel = panel();
  state->collision_placement_panel.signal_state = BTN_STATE_HOVER;
  state->collision_placement_panel.dest = Rectangle {0, 0, SE_BASE_RENDER_WIDTH * .25f, static_cast<f32>(SE_BASE_RENDER_HEIGHT)};
  state->collision_placement_panel.scroll_handle = Rectangle{
    state->collision_placement_panel.dest.x + state->collision_placement_panel.dest.width - PROP_DRAG_HANDLE_DIM - 10, 
    SCROLL_HANDLE_STARTING_HEIGHT,
    PROP_DRAG_HANDLE_DIM, 
    PROP_DRAG_HANDLE_DIM * 5,
  };
  state->collision_placement_panel.buffer.f32[1] = MAP_COLLISION_PANEL_COLLISION_DRAW_STARTING_HEIGHT;
  
  state->prop_selection_panel = panel();
  state->prop_selection_panel.signal_state = BTN_STATE_HOVER;
  state->prop_selection_panel.dest = Rectangle {0, 0, SE_BASE_RENDER_WIDTH * .35f, static_cast<f32>(SE_BASE_RENDER_HEIGHT)};
  state->prop_selection_panel.scroll_handle = Rectangle{
    state->prop_selection_panel.dest.x + state->prop_selection_panel.dest.width - PROP_DRAG_HANDLE_DIM - 10, 
    SCROLL_HANDLE_STARTING_HEIGHT,
    PROP_DRAG_HANDLE_DIM, 
    PROP_DRAG_HANDLE_DIM * 5,
  };
  state->prop_selection_panel.buffer.f32[1] = PROP_PANEL_PROP_DRAW_STARTING_HEIGHT;

  state->prop_edit_panel = panel();
  state->prop_edit_panel.signal_state = BTN_STATE_HOVER;
  Vector2 _prop_edit_panel_dim = Vector2 {SE_BASE_RENDER_WIDTH * .3f, static_cast<f32>(SE_BASE_RENDER_HEIGHT)};
  state->prop_edit_panel.dest = Rectangle { 
    SE_BASE_RENDER_WIDTH - _prop_edit_panel_dim.x, 0.f, 
    _prop_edit_panel_dim.x, _prop_edit_panel_dim.y
  };

  state->tilemap_props_trees = resource_get_tilemap_props_static(TILEMAP_PROP_TYPE_TREE);
  state->tilemap_props_tombstones = resource_get_tilemap_props_static(TILEMAP_PROP_TYPE_TOMBSTONE);
  state->tilemap_props_stones = resource_get_tilemap_props_static(TILEMAP_PROP_TYPE_STONE);
  state->tilemap_props_spikes = resource_get_tilemap_props_static(TILEMAP_PROP_TYPE_SPIKE);
  state->tilemap_props_skulls = resource_get_tilemap_props_static(TILEMAP_PROP_TYPE_SKULL);
  state->tilemap_props_pillars = resource_get_tilemap_props_static(TILEMAP_PROP_TYPE_PILLAR);
  state->tilemap_props_lamps = resource_get_tilemap_props_static(TILEMAP_PROP_TYPE_LAMP);
  state->tilemap_props_fence = resource_get_tilemap_props_static(TILEMAP_PROP_TYPE_FENCE);
  state->tilemap_props_details = resource_get_tilemap_props_static(TILEMAP_PROP_TYPE_DETAIL);
  state->tilemap_props_candles = resource_get_tilemap_props_static(TILEMAP_PROP_TYPE_CANDLE);
  state->tilemap_props_buildings = resource_get_tilemap_props_static(TILEMAP_PROP_TYPE_BUILDING);
  state->tilemap_props_sprite = resource_get_tilemap_props_sprite();

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
    
    slider *const scale_slider = get_slider_by_id(SDR_ID_EDITOR_PROP_SCALE_SLIDER);
    if (scale_slider == nullptr) {
      IERROR("scene_editor::initialize_scene_editor()::Cannot found Scale slider");
      return false;
    }
    scale_slider->on_left_button_trigger  = scene_editor_scale_slider_on_left_button_trigger;
    scale_slider->on_right_button_trigger = scene_editor_scale_slider_on_right_button_trigger;
    scale_slider->on_click =                nullptr;

    slider *const rotation_slider = get_slider_by_id(SDR_ID_EDITOR_PROP_ROTATION_SLIDER);
    if (rotation_slider == nullptr) {
      IERROR("scene_editor::initialize_scene_editor()::Cannot found Rotation slider");
      return false;
    }
    rotation_slider->on_left_button_trigger  = scene_editor_rotation_slider_on_left_button_trigger;
    rotation_slider->on_right_button_trigger = scene_editor_rotation_slider_on_right_button_trigger;
    rotation_slider->on_click =                nullptr;

    slider *const zindex_slider = get_slider_by_id(SDR_ID_EDITOR_PROP_ZINDEX_SLIDER);
    if (zindex_slider == nullptr) {
      IERROR("scene_editor::initialize_scene_editor()::Cannot found Z-index slider");
      return false;
    }
    zindex_slider->on_left_button_trigger  = scene_editor_zindex_slider_on_left_button_trigger;
    zindex_slider->on_right_button_trigger = scene_editor_zindex_slider_on_right_button_trigger;
    zindex_slider->on_click =                nullptr;

    slider *const layer_slider = get_slider_by_id(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER);
    if (layer_slider == nullptr) {
      IERROR("scene_editor::initialize_scene_editor()::Cannot found map layer slider");
      return false;
    }
    layer_slider->on_left_button_trigger  = scene_editor_map_layer_slc_slider_on_left_button_trigger;
    layer_slider->on_right_button_trigger = scene_editor_map_layer_slc_slider_on_right_button_trigger;
    layer_slider->on_click =                nullptr;

    slider *const stage_slider = get_slider_by_id(SDR_ID_EDITOR_MAP_STAGE_SLC_SLIDER);
    if (stage_slider == nullptr) {
      IERROR("scene_editor::initialize_scene_editor()::Cannot found map stage slider");
      return false;
    }
    stage_slider->on_left_button_trigger =  scene_editor_map_stage_slc_slider_on_left_button_trigger;
    stage_slider->on_right_button_trigger = scene_editor_map_stage_slc_slider_on_right_button_trigger;
    stage_slider->on_click =                nullptr;

    slider *const prop_type_slider = get_slider_by_id(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER);
    if (prop_type_slider == nullptr) {
      IERROR("scene_editor::initialize_scene_editor()::Cannot found map prop type slider");
      return false;
    }
    prop_type_slider->on_left_button_trigger =  scene_editor_map_prop_type_slc_slider_on_left_button_trigger;
    prop_type_slider->on_right_button_trigger = scene_editor_map_prop_type_slc_slider_on_right_button_trigger;
    prop_type_slider->on_click =                nullptr;
  }
  // Checkboxes
  {
    checkbox *const is_prop_y_based_checkbox = get_checkbox_by_id(CHECKBOX_ID_IS_PROP_YBASED);
    if (not is_prop_y_based_checkbox or is_prop_y_based_checkbox == nullptr) {
      IERROR("scene_editor::initialize_scene_editor()::y-based checkbox is not valid");
      return false;
    }
    is_prop_y_based_checkbox->pfn_on_change = scene_editor_is_map_prop_y_based_checkbox_on_change_trigger;
  }
  return begin_scene_editor(fade_in);
}
[[__nodiscard__]] bool begin_scene_editor(bool fade_in) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::begin_scene_editor()::State is not valid");
    return false;
  }
  state->b_show_tilesheet_tile_selection_screen = false;
  state->b_show_prop_selection_screen = false;
  state->b_show_prop_edit_screen = false;
  state->b_show_collision_placement_screen = false;
  state->b_dragging_map_element = false;
  state->b_show_pause_menu = false;
  state->b_prop_selection_screen_update_prop_sprites = false;
  state->tilemap_props_static_selected = nullptr;
  state->tilemap_props_sprite_selected = nullptr;
  state->selected_prop_static_map_prop_address = nullptr;
  state->selected_prop_sprite_map_prop_address = nullptr;
  state->selected_prop_static_panel_selection_copy = tilemap_prop_static();
  state->selected_prop_sprite_panel_selection_copy = tilemap_prop_sprite();
  state->sel_map_coll_addr_from_map = nullptr;
  state->map_collision_buffer_to_place = map_collision();
  state->selected_tile = tile();
  state->edit_layer = 0u;
  state->selected_stage = 0u;
  state->selection_type = SLC_TYPE_UNSELECTED;

  slider *const sdr_stage = get_slider_by_id(SDR_ID_EDITOR_MAP_STAGE_SLC_SLIDER);
  slider *const sdr_layer = get_slider_by_id(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER);
  sdr_stage->options.at(0).no_localized_text = TextFormat("%d", state->selected_stage);
  sdr_layer->options.at(0).no_localized_text = TextFormat("%d", state->edit_layer);

  event_fire(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, event_context(data128(state->target.x, state->target.y)));

  if (fade_in) {
    se_begin_fadein(data128(), nullptr);
  }
  update_tilemap_prop_type();
  return true;
}
void end_scene_editor(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::end_scene_editor()::State is not valid");
    return;
  }
  state->target = ZEROVEC2;
}

// UPDATE / RENDER
void update_scene_editor(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::update_scene_editor()::State is not valid");
    return;
  }
  if(not IsWindowFocused()) {
    state->mouse_focus = MOUSE_FOCUS_UNFOCUSED;
  }
  else {
    state->mouse_focus = MOUSE_FOCUS_MAP;
  }

  editor_update_bindings();
  event_fire(EVENT_CODE_CAMERA_SET_TARGET, event_context(data128(state->target.x, state->target.y)));

  update_map(GetFrameTime());
  update_camera(GetFrameTime());
  
  if(state->se_fade.fade_animation_playing){
    process_fade_effect(__builtin_addressof(state->se_fade));
  }
  else if (state->se_fade.is_fade_animation_played) {
    if (state->se_fade.fade_type == FADE_TYPE_FADEOUT) {
      if (state->se_fade.on_change_complete != nullptr) {
        state->se_fade.on_change_complete(state->se_fade.data);
      }
      se_begin_fadein(data128(), nullptr);
    }
    else if(state->se_fade.fade_type == FADE_TYPE_FADEIN) {
      state->se_fade = ui_fade_control_system();
    }
  }

  state->mouse_pos_screen.x = GetMousePosition().x * get_app_settings()->scale_ratio.at(0);
  state->mouse_pos_screen.y = GetMousePosition().y * get_app_settings()->scale_ratio.at(1);
  state->mouse_pos_world = GetScreenToWorld2D(state->mouse_pos_screen, state->in_camera_metrics->handle);

  if (state->b_prop_selection_screen_update_prop_sprites) {
    for (size_t itr_000 = 0u; itr_000 < state->tilemap_props_sprite->size(); itr_000++) {
      spritesheet *const sprite = __builtin_addressof(state->tilemap_props_sprite->at(itr_000).sprite);
      ui_update_sprite(sprite, GetFrameTime());
    }
    state->b_prop_selection_screen_update_prop_sprites = false;
  }
  if (state->selected_prop_sprite_panel_selection_copy.is_initialized and state->selection_type == SLC_TYPE_DROP_PROP_SPRITE) {
    ui_update_sprite(__builtin_addressof(state->selected_prop_sprite_panel_selection_copy.sprite), GetFrameTime());
  }
  update_user_interface(GetFrameTime());
}
void render_scene_editor(void) {
  BeginMode2D(get_in_game_camera()->handle);
  
  if (state->selected_stage == WORLDMAP_MAINMENU_MAP) {
    render_map();
  }
  else {
    i32 map_top = (*state->active_map_ptr)->position.y;
    i32 map_bottom = (*state->active_map_ptr)->position.y + ((*state->active_map_ptr)->map_dim * (*state->active_map_ptr)->tile_size);
    
    render_map();
    _render_props_y_based(map_top, map_bottom); // INFO: Top to bottom order
  }
  const std::vector<map_collision>& _map_collisions = (*state->active_map_ptr)->collisions;
  for (size_t itr_000 = 0u; itr_000 < _map_collisions.size(); ++itr_000) {
    const Rectangle& _map_coll = _map_collisions.at(itr_000).dest;

    DrawRectangleLinesEx(_map_coll, 2.f, BLUE);    
  }
  DrawPixel(0, 0, RED);
  
  EndMode2D();
}
void render_interface_editor(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::render_interface_editor()::State is not valid");
    return;
  }
  if(state->b_show_tilesheet_tile_selection_screen) 
  {
    const panel *const pnl = __builtin_addressof(state->tile_selection_panel);
    Rectangle pnl_dest = pnl->dest;
    pnl_dest.y += TILESHEET_PANEL_SHEET_DRAW_STARTING_HEIGHT;

    gui_panel((*pnl), pnl->dest, false);
    {
      Vector2 label_anchor  = VECTOR2(pnl->dest.x + pnl->dest.width * .5f, pnl->dest.y + pnl->dest.height * .025f);
      Vector2 slider_anchor = VECTOR2(pnl->dest.x + pnl->dest.width * .5f, pnl->dest.y + pnl->dest.height * .025f);
      
      gui_label_grid("Layer", FONT_TYPE_REGULAR, 1, label_anchor, WHITE, true, true, VECTOR2(-25.f, -3.f));
      gui_slider(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER, slider_anchor, VECTOR2(0.f,-3.f));
      gui_label_grid("Stage", FONT_TYPE_REGULAR, 1, label_anchor, WHITE, true, true, VECTOR2(-25.f, 5.f));
      gui_slider(SDR_ID_EDITOR_MAP_STAGE_SLC_SLIDER, slider_anchor, VECTOR2(0.f,5.f));
    }
    BeginScissorMode(pnl_dest.x, pnl_dest.y, pnl_dest.width, pnl_dest.height);
    {
      render_map_palette(state->tile_selection_panel.zoom);
    }
    EndScissorMode();
  }
  else if(state->b_show_prop_selection_screen)
  {
    panel *const pnl = __builtin_addressof(state->prop_selection_panel);
    Rectangle pnl_dest = pnl->dest;
    pnl_dest.y += PROP_PANEL_PROP_DRAW_STARTING_HEIGHT;
    gui_panel((*pnl), pnl->dest, false);
    {
      Vector2 label_anchor = VECTOR2(pnl->dest.x + pnl->dest.width * .25f, pnl->dest.y + pnl->dest.height * .05f);
      Vector2 slider_anchor = VECTOR2(pnl->dest.x + pnl->dest.width * .675f, pnl->dest.y + pnl->dest.height * .05f);
      gui_label_grid("Prop Type", FONT_TYPE_REGULAR, 1, label_anchor, WHITE, true, true, VECTOR2(0.f, 0.f));
      gui_slider(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, slider_anchor, VECTOR2(0.f, 0.f));
    }  
    BeginScissorMode(pnl_dest.x, pnl_dest.y, pnl_dest.width, pnl_dest.height);
    {
      f32 prop_height_count = pnl->buffer.f32[1];
      if (get_slider_current_value(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->data.i32[0] == TILEMAP_PROP_TYPE_SPRITE) {
        state->b_prop_selection_screen_update_prop_sprites = true;
        for (size_t iter = 0u; iter < state->tilemap_props_sprite_selected->size(); ++iter) {
          tilemap_prop_sprite& prop = state->tilemap_props_sprite_selected->at(iter);
          Rectangle dest = prop.sprite.coord;
          dest.x = 0.f;
          dest.y = (pnl->scroll * pnl->buffer.f32[0]) + prop_height_count;
          ui_play_sprite_on_site(__builtin_addressof(prop.sprite), dest, ZEROVEC2, 0.f, prop.sprite.tint);
          prop_height_count += dest.height;
        }
      } else {
        for (size_t iter = 0u; iter < state->tilemap_props_static_selected->size(); ++iter) {
          const tilemap_prop_static& prop = state->tilemap_props_static_selected->at(iter);
          Rectangle dest = prop.dest;
          dest.x = 0.f;
          dest.y = (pnl->scroll * pnl->buffer.f32[0]) + prop_height_count;
          gui_draw_texture_id_pro(prop.tex_id, prop.source, dest, prop.tint);
          prop_height_count += prop.dest.height;
        }
      }
      pnl->buffer.f32[0] = prop_height_count;
      pnl->scroll_handle.y = std::max(pnl->scroll_handle.y, pnl->dest.y + SCROLL_HANDLE_STARTING_HEIGHT);
      pnl->scroll_handle.y = std::min(pnl->scroll_handle.y, pnl->dest.y + pnl->dest.height - pnl->scroll_handle.height);
      pnl->scroll = (pnl->dest.y + pnl->scroll_handle.y - SCROLL_HANDLE_STARTING_HEIGHT) / (pnl->dest.height - pnl->scroll_handle.height) * -1;
      DrawRectangleRec(pnl->scroll_handle, WHITE);
    }
    EndScissorMode();
  }
  else if(state->b_show_collision_placement_screen) 
  {
    panel *const pnl = &state->collision_placement_panel;
    Rectangle pnl_dest = pnl->dest;
    pnl_dest.y += MAP_COLLISION_PANEL_COLLISION_DRAW_STARTING_HEIGHT;
    pnl_dest.height -= MAP_COLLISION_PANEL_COLLISION_DRAW_STARTING_HEIGHT;
    gui_panel((*pnl), pnl->dest, false);
    {
      Vector2 button_location_pivot = VECTOR2(pnl->dest.x + pnl->dest.width * .5f,pnl->dest.y + pnl->dest.height * .05f);
      if(gui_menu_button("Add", BTN_ID_EDITOR_ADD_MAP_COLLISION, VECTOR2(0, 0), button_location_pivot, false)) {
        scene_editor_selection_cleanup();

        state->selection_type = SLC_TYPE_DROP_COLLISION_POSITION;
      }
    }
    BeginScissorMode(pnl_dest.x, pnl_dest.y, pnl_dest.width, pnl_dest.height);
    {
      f32 collision_height_count = pnl_dest.y;
      for (size_t itr_000 = 0u; itr_000 < (*state->active_map_ptr)->collisions.size(); ++itr_000) {
        const map_collision& _map_coll = (*state->active_map_ptr)->collisions.at(itr_000);
        Vector2 dest = VECTOR2(
          pnl->dest.x + (pnl->dest.width * .5f), 
          (pnl->scroll * pnl->buffer.f32[0]) + collision_height_count
        );

        if (state->sel_map_coll_addr_from_map and state->sel_map_coll_addr_from_map != nullptr and state->sel_map_coll_addr_from_map->coll_id == _map_coll.coll_id) {
          gui_label_format(FONT_TYPE_REGULAR, 1, dest.x, dest.y, WHITE, true, false, "> %.0f, %.0f, %.0f, %.0f <", 
            _map_coll.dest.x, _map_coll.dest.y, _map_coll.dest.width, _map_coll.dest.height
          );
        }
        else {
          gui_label_format(FONT_TYPE_REGULAR, 1, dest.x, dest.y, WHITE, true, false, "%.0f, %.0f, %.0f, %.0f", 
            _map_coll.dest.x, _map_coll.dest.y, _map_coll.dest.width, _map_coll.dest.height
          );
        }
        collision_height_count += 30.f;
      }
      pnl->buffer.f32[0] = collision_height_count;
      pnl->scroll_handle.y = std::max(pnl->scroll_handle.y, pnl->dest.y + SCROLL_HANDLE_STARTING_HEIGHT);
      pnl->scroll_handle.y = std::min(pnl->scroll_handle.y, pnl->dest.y + pnl->dest.height - pnl->scroll_handle.height);
      pnl->scroll = (pnl->dest.y + pnl->scroll_handle.y - SCROLL_HANDLE_STARTING_HEIGHT) / (pnl->dest.height - pnl->scroll_handle.height) * -1;
      DrawRectangleRec(pnl->scroll_handle, WHITE);
    }
    EndScissorMode();
  }

  switch (state->selection_type) {
    case SLC_TYPE_TILE: {
      if (state->default_sheet <= TILESHEET_TYPE_UNSPECIFIED or state->default_sheet >= TILESHEET_TYPE_MAX) {
        return;
      }
      const tilesheet *const default_sheet = get_tilesheet_by_enum(state->default_sheet);
      if (default_sheet and default_sheet != nullptr) {
        _render_tile_on_pos(state->selected_tile, state->mouse_pos_screen, default_sheet);
      }
      break;
    }
    case SLC_TYPE_DROP_PROP_STATIC: {
      if (not state->selected_prop_static_panel_selection_copy.is_initialized) {
        return;
      }
      const tilemap_prop_static *const p_prop = __builtin_addressof(state->selected_prop_static_panel_selection_copy);
      if (not p_prop->is_initialized) {
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
      if (not state->selected_prop_sprite_panel_selection_copy.is_initialized) {
        return;
      }
      tilemap_prop_sprite& slc_prop_sprite = state->selected_prop_sprite_panel_selection_copy;
      ui_play_sprite_on_site(__builtin_addressof(slc_prop_sprite.sprite), 
        Rectangle { 
          state->mouse_pos_screen.x, state->mouse_pos_screen.y,
          slc_prop_sprite.sprite.coord.width, slc_prop_sprite.sprite.coord.width
        }, 
        ZEROVEC2, 0.f, slc_prop_sprite.sprite.tint
      );
      break;
    }
    case SLC_TYPE_SLC_PROP_STATIC: { // SEE SLC_TYPE_SLC_PROP_SPRITE EITHER
      if (state->selected_prop_static_map_prop_address == nullptr or state->in_camera_metrics == nullptr) {
        return;
      }
      panel *const pnl = __builtin_addressof(state->prop_edit_panel);
      
      tilemap_prop_static** p_prop = __builtin_addressof(state->selected_prop_static_map_prop_address);
      if ((*p_prop) == nullptr) {
        return;
      }

      gui_panel((*pnl), pnl->dest, false);
      BeginScissorMode(pnl->dest.x, pnl->dest.y, pnl->dest.width, pnl->dest.height);
      {
        ui_set_slider_current_value(SDR_ID_EDITOR_PROP_SCALE_SLIDER, slider_option(TextFormat("%.2f", (*p_prop)->scale), data_pack()));
        Vector2 sdr_center = VECTOR2( pnl->dest.x + pnl->dest.width * .75f, pnl->dest.y + pnl->dest.height * .5f);
        Vector2 label_center = VECTOR2( pnl->dest.x + pnl->dest.width * .25f, pnl->dest.y + pnl->dest.height * .5f);
      
        gui_label_grid("Scale", FONT_TYPE_REGULAR, 1, label_center, WHITE, true, true, VECTOR2(0.f, -10.f));
        get_slider_by_id(SDR_ID_EDITOR_PROP_SCALE_SLIDER)->options.at(0).no_localized_text = TextFormat("%.2f", (*p_prop)->scale);
        gui_slider(SDR_ID_EDITOR_PROP_SCALE_SLIDER, sdr_center, VECTOR2(0.f, -10.f));
      
        gui_label_grid("Rotation", FONT_TYPE_REGULAR, 1, label_center, WHITE, true, true, VECTOR2(0.f, 0.f));
        get_slider_by_id(SDR_ID_EDITOR_PROP_ROTATION_SLIDER)->options.at(0).no_localized_text = TextFormat("%.1f", (*p_prop)->rotation);
        gui_slider(SDR_ID_EDITOR_PROP_ROTATION_SLIDER, sdr_center, VECTOR2(0.f, 0.f));
      
        gui_label_grid("Z-Index", FONT_TYPE_REGULAR, 1, label_center, WHITE, true, true, VECTOR2(0.f, 10.f));
        get_slider_by_id(SDR_ID_EDITOR_PROP_ZINDEX_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", (*p_prop)->zindex);
        gui_slider(SDR_ID_EDITOR_PROP_ZINDEX_SLIDER, sdr_center, VECTOR2(0.f, 10.f));

        gui_label_grid("Y-Based", FONT_TYPE_REGULAR, 1, label_center, WHITE, true, true, VECTOR2(0.f, 20.f));
        get_checkbox_by_id(CHECKBOX_ID_IS_PROP_YBASED)->state = (*p_prop)->use_y_based_zindex ? CHECKBOX_STATE_CHECKED : CHECKBOX_STATE_UNCHECKED;
        gui_checkbox_grid(CHECKBOX_ID_IS_PROP_YBASED, VECTOR2(0.f, 20.f), sdr_center);
      }
      EndScissorMode();

      Rectangle prop_dest = (*p_prop)->dest;
      if (state->selected_stage == WORLDMAP_MAINMENU_MAP) {
        prop_dest = wld_calc_mainmenu_prop_dest((*state->active_map_ptr), prop_dest, (*p_prop)->scale);
      }

      Vector2 prop_pos = GetWorldToScreen2D(Vector2{ prop_dest.x, prop_dest.y}, state->in_camera_metrics->handle);
      f32 relative_width = prop_dest.width * state->in_camera_metrics->handle.zoom;
      f32 relative_height = prop_dest.height * state->in_camera_metrics->handle.zoom;
      
      prop_pos.x -= relative_width * 0.5f; // To center to its origin
      prop_pos.y -= relative_height * 0.5f;
      DrawRectangleLines(prop_pos.x, prop_pos.y, relative_width, relative_height, WHITE);
      prop_pos.x += relative_width / 2.f - PROP_DRAG_HANDLE_DIM_DIV2;
      prop_pos.y += relative_height / 2.f - PROP_DRAG_HANDLE_DIM_DIV2;
      DrawRectangle(prop_pos.x, prop_pos.y, PROP_DRAG_HANDLE_DIM, PROP_DRAG_HANDLE_DIM, WHITE);
      break;
    }
    case SLC_TYPE_SLC_PROP_SPRITE: { // SEE SLC_TYPE_SLC_PROP_STATIC EITHER
      if (state->selected_prop_sprite_map_prop_address == nullptr or state->in_camera_metrics == nullptr) {
        return;
      }

      panel *const pnl = &state->prop_edit_panel;
      tilemap_prop_sprite** p_prop = __builtin_addressof(state->selected_prop_sprite_map_prop_address);
      if ((*p_prop) == nullptr) {
        return;
      }
      gui_panel((*pnl), pnl->dest, false);
      BeginScissorMode(pnl->dest.x, pnl->dest.y, pnl->dest.width, pnl->dest.height);
      {
        ui_set_slider_current_value(SDR_ID_EDITOR_PROP_SCALE_SLIDER, slider_option(TextFormat("%.2f", (*p_prop)->scale), data_pack()));
        Vector2 sdr_center = VECTOR2( pnl->dest.x + pnl->dest.width * .75f, pnl->dest.y + pnl->dest.height * .5f);
        Vector2 label_center = VECTOR2( pnl->dest.x + pnl->dest.width * .25f, pnl->dest.y + pnl->dest.height * .5f);

        gui_label_grid("Scale", FONT_TYPE_REGULAR, 1, label_center, WHITE, true, true, VECTOR2(0.f, -10.f));
        get_slider_by_id(SDR_ID_EDITOR_PROP_SCALE_SLIDER)->options.at(0).no_localized_text = TextFormat("%.2f", (*p_prop)->scale);
        gui_slider(SDR_ID_EDITOR_PROP_SCALE_SLIDER, sdr_center, VECTOR2(0.f, -10.f));

        gui_label_grid("Rotation", FONT_TYPE_REGULAR, 1, label_center, WHITE, true, true, VECTOR2(0.f, 0.f));
        get_slider_by_id(SDR_ID_EDITOR_PROP_ROTATION_SLIDER)->options.at(0).no_localized_text = TextFormat("%.1f", (*p_prop)->sprite.rotation);
        gui_slider(SDR_ID_EDITOR_PROP_ROTATION_SLIDER, sdr_center, VECTOR2(0.f, 0.f));

        gui_label_grid("Z-Index", FONT_TYPE_REGULAR, 1, label_center, WHITE, true, true, VECTOR2(0.f, 10.f));
        get_slider_by_id(SDR_ID_EDITOR_PROP_ZINDEX_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", (*p_prop)->zindex);
        gui_slider(SDR_ID_EDITOR_PROP_ZINDEX_SLIDER, sdr_center, VECTOR2(0.f, 10.f));

        gui_label_grid("Y-Based", FONT_TYPE_REGULAR, 1, label_center, WHITE, true, true, VECTOR2(0.f, 20.f));
        get_checkbox_by_id(CHECKBOX_ID_IS_PROP_YBASED)->state = (*p_prop)->use_y_based_zindex ? CHECKBOX_STATE_CHECKED : CHECKBOX_STATE_UNCHECKED;
        gui_checkbox_grid(CHECKBOX_ID_IS_PROP_YBASED, VECTOR2(0.f, 20.f), sdr_center);
      }
      EndScissorMode();

      tilemap_prop_sprite** slc_prop_sprite = __builtin_addressof(state->selected_prop_sprite_map_prop_address);
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
    case SLC_TYPE_DROP_COLLISION_POSITION: {
      state->map_collision_buffer_to_place.dest = Rectangle {state->mouse_pos_world.x, state->mouse_pos_world.y, 0.f, 0.f};
      gui_label_format(FONT_TYPE_REGULAR, 1, SE_BASE_RENDER_WIDTH * .5f, SE_BASE_RENDER_HEIGHT * .9f, WHITE, true, true, "%.0f, %.0f, %.0f, %.0f",
        state->map_collision_buffer_to_place.dest.x, state->map_collision_buffer_to_place.dest.y, state->map_collision_buffer_to_place.dest.width, state->map_collision_buffer_to_place.dest.height
      );
      break;
    }
    case SLC_TYPE_DROP_COLLISION_DIMENTIONS: {
      if (state->map_collision_buffer_to_place.dest.width > 0 and state->map_collision_buffer_to_place.dest.height > 0) {
        gui_label_format(FONT_TYPE_REGULAR, 1, SE_BASE_RENDER_WIDTH * .5f, SE_BASE_RENDER_HEIGHT * .9f, WHITE, true, true, 
          "%.1f, %.1f, %.1f, %.1f", 
          state->map_collision_buffer_to_place.dest.x, state->map_collision_buffer_to_place.dest.y, 
          state->map_collision_buffer_to_place.dest.width, state->map_collision_buffer_to_place.dest.height
        );
        Vector2 map_to_screen_coord = GetWorldToScreen2D(VECTOR2(state->map_collision_buffer_to_place.dest.x, state->map_collision_buffer_to_place.dest.y), state->in_camera_metrics->handle);
        DrawRectangleLinesEx(Rectangle {
          map_to_screen_coord.x, map_to_screen_coord.y, 
          state->map_collision_buffer_to_place.dest.width * state->in_camera_metrics->handle.zoom, 
          state->map_collision_buffer_to_place.dest.height * state->in_camera_metrics->handle.zoom}, 2.f, BLUE
        );
      }
      else gui_label_format(FONT_TYPE_REGULAR, 1, SE_BASE_RENDER_WIDTH * .5f, SE_BASE_RENDER_HEIGHT * .9f, RED, true, true, 
        "%.1f, %.1f, %.1f, %.1f \\ Rectangle cannot have negative dimention", 
        state->map_collision_buffer_to_place.dest.x, state->map_collision_buffer_to_place.dest.y, 
        state->map_collision_buffer_to_place.dest.width, state->map_collision_buffer_to_place.dest.height
      );
      break;
    }
    case SLC_TYPE_SLC_MAP_COLLISION: {
      if (state->sel_map_coll_addr_from_map == nullptr or state->in_camera_metrics == nullptr) {
        return;
      }
      const map_collision *const map_coll = state->sel_map_coll_addr_from_map;

      Vector2 prop_pos = GetWorldToScreen2D(Vector2{ map_coll->dest.x, map_coll->dest.y}, state->in_camera_metrics->handle);
      f32 relative_width = map_coll->dest.width * state->in_camera_metrics->handle.zoom;
      f32 relative_height = map_coll->dest.height * state->in_camera_metrics->handle.zoom;
      
      DrawRectangleLinesEx(Rectangle {prop_pos.x, prop_pos.y, relative_width, relative_height}, 2.f, WHITE);
      prop_pos.x += relative_width / 2.f - PROP_DRAG_HANDLE_DIM_DIV2;
      prop_pos.y += relative_height / 2.f - PROP_DRAG_HANDLE_DIM_DIV2;
      DrawRectangle(prop_pos.x, prop_pos.y, PROP_DRAG_HANDLE_DIM, PROP_DRAG_HANDLE_DIM, WHITE); // See also editor_update_mouse_bindings
    }
    default: break;
  }
  
  if (state->b_show_pause_menu) {
    gui_draw_pause_screen(false);
  }

  render_user_interface();
}
// UPDATE / RENDER

// BINDINGS
constexpr void editor_update_bindings(void) {
  editor_update_mouse_bindings();
  editor_update_keyboard_bindings();
}
constexpr void editor_update_mouse_bindings(void) {
  if(state->b_show_tilesheet_tile_selection_screen and CheckCollisionPointRec(state->mouse_pos_screen, state->tile_selection_panel.dest))
  {
    state->mouse_focus = MOUSE_FOCUS_TILE_SELECTION_PANEL;
    state->tile_selection_panel.zoom += ((float)GetMouseWheelMove()*0.05f);

    if (state->tile_selection_panel.zoom > 3.0f) state->tile_selection_panel.zoom = 3.0f;
    else if (state->tile_selection_panel.zoom < 0.1f) state->tile_selection_panel.zoom = 0.1f;

    Rectangle tilesheet_drawing_bounds = state->tile_selection_panel.dest;
    tilesheet_drawing_bounds.y += TILESHEET_PANEL_SHEET_DRAW_STARTING_HEIGHT;
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->selection_type == SLC_TYPE_UNSELECTED && CheckCollisionPointRec(state->mouse_pos_screen, tilesheet_drawing_bounds)) {
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
  else if(state->b_show_prop_selection_screen and CheckCollisionPointRec(state->mouse_pos_screen, state->prop_selection_panel.dest))
  {
    state->mouse_focus = MOUSE_FOCUS_PROP_SELECTION_PANEL;
    panel *const pnl = __builtin_addressof(state->prop_selection_panel);
    pnl->scroll_handle.y += GetMouseWheelMove() * -10.f;

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) and state->selection_type == SLC_TYPE_UNSELECTED and CheckCollisionPointRec(state->mouse_pos_screen, pnl->scroll_handle)) {
      pnl->is_dragging_scroll = true;
    }
    else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) and state->selection_type == SLC_TYPE_UNSELECTED and not pnl->is_dragging_scroll) {
      i32 h = (pnl->scroll * pnl->buffer.f32[0] * (-1)) + state->mouse_pos_screen.y - pnl->buffer.f32[1];
      if (get_slider_current_value(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->data.i32[0] == TILEMAP_PROP_TYPE_SPRITE) {
        for (size_t itr_000 = 0u; itr_000< state->tilemap_props_sprite_selected->size() and h > 0; ++itr_000) {
          if(h - state->tilemap_props_sprite_selected->at(itr_000).sprite.current_frame_rect.height < 0) {
            state->selected_prop_sprite_panel_selection_copy = state->tilemap_props_sprite_selected->at(itr_000);
            state->selected_prop_sprite_panel_selection_copy.sprite.origin = Vector2 {
              state->selected_prop_sprite_panel_selection_copy.sprite.current_frame_rect.width * .5f,
              state->selected_prop_sprite_panel_selection_copy.sprite.current_frame_rect.height * .5f
            };
            state->selection_type = SLC_TYPE_DROP_PROP_SPRITE;
            break;
          }
          h -= state->tilemap_props_sprite_selected->at(itr_000).sprite.current_frame_rect.height;
        }
      }
      else {
        for (size_t itr_000 = 0u; itr_000 < state->tilemap_props_static_selected->size() and h > 0; ++itr_000) {
          if(h - state->tilemap_props_static_selected->at(itr_000).source.height < 0) {
            state->selected_prop_static_panel_selection_copy = state->tilemap_props_static_selected->at(itr_000);
            state->selected_prop_static_panel_selection_copy.origin = Vector2 {
              state->selected_prop_static_panel_selection_copy.dest.width * .5f,
              state->selected_prop_static_panel_selection_copy.dest.height * .5f
            };
            state->selection_type = SLC_TYPE_DROP_PROP_STATIC;
            break;
          }
          h -= state->tilemap_props_static_selected->at(itr_000).source.height;
        }
      }
    }
  }
  else if(state->b_show_collision_placement_screen and CheckCollisionPointRec(state->mouse_pos_screen, state->collision_placement_panel.dest)) {
    state->mouse_focus = MOUSE_FOCUS_COLLISION_PLACEMENT_PANEL;
    panel *const pnl = __builtin_addressof(state->collision_placement_panel);
    pnl->scroll_handle.y += GetMouseWheelMove() * -10.f;

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) and state->selection_type == SLC_TYPE_UNSELECTED and CheckCollisionPointRec(state->mouse_pos_screen, pnl->scroll_handle)) {
      pnl->is_dragging_scroll = true;
    }
    else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) and state->selection_type == SLC_TYPE_UNSELECTED and not pnl->is_dragging_scroll) {
      i32 h = (pnl->scroll * pnl->buffer.f32[0] * (-1)) + state->mouse_pos_screen.y - pnl->buffer.f32[1];
      for (size_t itr_000 = 0u; itr_000 < (*state->active_map_ptr)->collisions.size() and h > 0; ++itr_000) {
        if(h - 30 < 0) {
          state->sel_map_coll_addr_from_map = __builtin_addressof((*state->active_map_ptr)->collisions.at(itr_000));
          state->selection_type = SLC_TYPE_SLC_MAP_COLLISION;
          state->target = Vector2 {
            state->sel_map_coll_addr_from_map->dest.x + (state->sel_map_coll_addr_from_map->dest.width * .5f),
            state->sel_map_coll_addr_from_map->dest.y + (state->sel_map_coll_addr_from_map->dest.height * .5f)
          };
          break;
        }
        h -= 30;
      }
    }
  }
  else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) and state->mouse_focus == MOUSE_FOCUS_MAP) {
    switch (state->selection_type) {
      case SLC_TYPE_UNSELECTED: {
        map_collision *const _map_coll = get_map_collision_by_pos(state->mouse_pos_world);
        if (_map_coll != nullptr) {
          state->sel_map_coll_addr_from_map = _map_coll;
          state->selection_type = SLC_TYPE_SLC_MAP_COLLISION;
        };
        tilemap_prop_address _prop_address = get_map_prop_by_pos(state->mouse_pos_world);        
        if (_prop_address.type > TILEMAP_PROP_TYPE_UNDEFINED and _prop_address.type < TILEMAP_PROP_TYPE_MAX){
          if (_prop_address.type != TILEMAP_PROP_TYPE_SPRITE and _prop_address.data.prop_static != nullptr) {
            state->selected_prop_static_map_prop_address = _prop_address.data.prop_static;
            state->selection_type = SLC_TYPE_SLC_PROP_STATIC;
          }
          else if (_prop_address.type == TILEMAP_PROP_TYPE_SPRITE and _prop_address.data.prop_sprite != nullptr) {
            state->selected_prop_sprite_map_prop_address = _prop_address.data.prop_sprite;
            state->selection_type = SLC_TYPE_SLC_PROP_SPRITE;
          }
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
        state->b_dragging_map_element = false;
        if (state->selected_prop_static_map_prop_address and state->selected_prop_static_map_prop_address != nullptr) {
          if (state->selected_prop_static_map_prop_address->use_y_based_zindex) {
            _sort_render_y_based_queue();
          }
        }
        break;
      }
      case SLC_TYPE_SLC_PROP_SPRITE: {
        state->b_dragging_map_element = false;
        if (state->selected_prop_sprite_map_prop_address and state->selected_prop_sprite_map_prop_address != nullptr) {
          if (state->selected_prop_sprite_map_prop_address->use_y_based_zindex) {
            _sort_render_y_based_queue();
          }
        }
        break;
      }
      case SLC_TYPE_DROP_COLLISION_POSITION: {
        state->selection_type = SLC_TYPE_DROP_COLLISION_DIMENTIONS;
        break;
      }
      case SLC_TYPE_DROP_COLLISION_DIMENTIONS: {
        if (state->map_collision_buffer_to_place.dest.width != -1) {
          add_map_coll_curr_map(state->map_collision_buffer_to_place.dest);
        }
        break;
      }
      case SLC_TYPE_SLC_MAP_COLLISION: {
        state->b_dragging_map_element = false;
        _sort_render_y_based_queue();
      }
      default: break;
    }
  }
  else if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && state->mouse_focus == MOUSE_FOCUS_MAP) {
    switch (state->selection_type) {
      case SLC_TYPE_SLC_PROP_STATIC: {
        tilemap_prop_static *const prop = state->selected_prop_static_map_prop_address;
        if (prop == nullptr) { break; }

        Rectangle drag_handle = Rectangle { prop->dest.x - PROP_DRAG_HANDLE_DIM_DIV2, prop->dest.y - PROP_DRAG_HANDLE_DIM_DIV2,  PROP_DRAG_HANDLE_DIM, PROP_DRAG_HANDLE_DIM};
        if(not state->b_dragging_map_element and CheckCollisionPointRec(state->mouse_pos_world, drag_handle)) {
          state->b_dragging_map_element = true;
        }
        if (state->b_dragging_map_element) {
          prop->dest.x = state->mouse_pos_world.x;
          prop->dest.y = state->mouse_pos_world.y;
        }
        break;
      }
      case SLC_TYPE_SLC_PROP_SPRITE: {
        tilemap_prop_sprite *const prop = get_map_prop_sprite_by_id(state->selected_prop_sprite_map_prop_address->map_id);
        if (prop == nullptr) {
          IERROR("scene_editor::editor_update_mouse_bindings()::Prop sprite:%d cannot found", state->selected_prop_sprite_map_prop_address->map_id);
          break;
        }
        Rectangle drag_handle = Rectangle {prop->sprite.coord.x - PROP_DRAG_HANDLE_DIM_DIV2, prop->sprite.coord.y - PROP_DRAG_HANDLE_DIM_DIV2, PROP_DRAG_HANDLE_DIM, PROP_DRAG_HANDLE_DIM};

        if(not state->b_dragging_map_element and CheckCollisionPointRec(state->mouse_pos_world, drag_handle)) {
          state->b_dragging_map_element = true;
        }
        if (state->b_dragging_map_element) {
          prop->sprite.coord.x = state->mouse_pos_world.x;
          prop->sprite.coord.y = state->mouse_pos_world.y;
          state->selected_prop_sprite_map_prop_address->sprite.coord = prop->sprite.coord;
        }
        break;
      }
      case SLC_TYPE_TILE: { 
        tile _tile = _get_tile_from_map_by_mouse_pos(state->edit_layer, state->mouse_pos_screen);
        if (_tile.position.x < MAX_TILEMAP_TILESLOT_X or _tile.position.y < MAX_TILEMAP_TILESLOT_Y ) {
          set_map_tile(state->edit_layer, _tile, state->selected_tile);
        }
        break; 
      }
      case SLC_TYPE_SLC_MAP_COLLISION: {
        if (not state->sel_map_coll_addr_from_map or state->sel_map_coll_addr_from_map == nullptr) {
          return;
        }
        map_collision *const map_coll = state->sel_map_coll_addr_from_map;

        Rectangle drag_handle = Rectangle {
          map_coll->dest.x + (map_coll->dest.width * .5f ) - PROP_DRAG_HANDLE_DIM_DIV2, 
          map_coll->dest.y + (map_coll->dest.height * .5f) - PROP_DRAG_HANDLE_DIM_DIV2, 
          PROP_DRAG_HANDLE_DIM, PROP_DRAG_HANDLE_DIM
        };
        if (not state->b_dragging_map_element and CheckCollisionPointRec(state->mouse_pos_world, drag_handle)) {
          state->b_dragging_map_element = true;
        }
        if (state->b_dragging_map_element) {
          map_coll->dest.x = state->mouse_pos_world.x - (map_coll->dest.width * .5f );
          map_coll->dest.y = state->mouse_pos_world.y - (map_coll->dest.height * .5f);
        }
      }
      default: break;
    }
  }
  if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) 
  {
    if (state->prop_selection_panel.is_dragging_scroll and state->b_show_prop_selection_screen) {
      panel *const pnl = __builtin_addressof(state->prop_selection_panel);
      i32 mouse_pos_y = state->mouse_pos_screen.y;
      pnl->scroll_handle.y = mouse_pos_y - (pnl->scroll_handle.height * .5f);
    } 
    else if(state->prop_selection_panel.is_dragging_scroll and not state->b_show_prop_selection_screen) {
      state->prop_selection_panel.is_dragging_scroll = false;
    }

    if (state->collision_placement_panel.is_dragging_scroll and state->b_show_collision_placement_screen) {
      panel *const pnl = __builtin_addressof(state->collision_placement_panel);
      i32 mouse_pos_y = state->mouse_pos_screen.y;
      pnl->scroll_handle.y = mouse_pos_y - (pnl->scroll_handle.height * .5f);
    }
    else if(state->collision_placement_panel.is_dragging_scroll and not state->b_show_collision_placement_screen) {
      state->collision_placement_panel.is_dragging_scroll = false;
    }
  }
  if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) 
  {
    if (state->prop_selection_panel.is_dragging_scroll) {
      state->prop_selection_panel.is_dragging_scroll = false;
    }
    if (state->collision_placement_panel.is_dragging_scroll) {
      state->collision_placement_panel.is_dragging_scroll = false;
    }
  }
  if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) 
  {
    scene_editor_selection_cleanup();
  }
  if (state->mouse_focus == MOUSE_FOCUS_MAP) {
    event_fire(EVENT_CODE_CAMERA_ADD_ZOOM, event_context((GetMouseWheelMove() * 0.05f)));

    switch (state->selection_type) {
      case SLC_TYPE_DROP_COLLISION_POSITION: {
        state->map_collision_buffer_to_place.dest = Rectangle {state->mouse_pos_world.x, state->mouse_pos_world.y, 0.f, 0.f};
        break;
      }
      case SLC_TYPE_DROP_COLLISION_DIMENTIONS: {
        f32 width = state->mouse_pos_world.x - state->map_collision_buffer_to_place.dest.x;
        f32 height = state->mouse_pos_world.y - state->map_collision_buffer_to_place.dest.y;
        if (width > 0.f and height > 0.f) {
          state->map_collision_buffer_to_place.dest.width  = width;
          state->map_collision_buffer_to_place.dest.height = height;
        } else {
          state->map_collision_buffer_to_place.dest.width = -1;
          state->map_collision_buffer_to_place.dest.height = -1;
        }
        break;
      }
      default: break;
    }
  }
}
constexpr void editor_update_keyboard_bindings(void) {
  editor_update_movement();

  if (IsKeyReleased(KEY_ESCAPE)) {
    state->b_show_pause_menu = not state->b_show_pause_menu;
  }
  if (IsKeyPressed(KEY_F5)) {
    save_current_map();
  }
  if (IsKeyPressed(KEY_F8)) {
    load_current_map();
  }
  if (IsKeyReleased(KEY_TAB)) {
    state->b_show_tilesheet_tile_selection_screen = not state->b_show_tilesheet_tile_selection_screen;
    state->b_show_prop_selection_screen = false;
    state->b_show_collision_placement_screen = false;
    scene_editor_selection_cleanup();
  }
  if (IsKeyPressed(KEY_I)) {
    state->b_show_prop_selection_screen = not state->b_show_prop_selection_screen;
    state->b_show_tilesheet_tile_selection_screen = false;
    state->b_show_collision_placement_screen = false;
    scene_editor_selection_cleanup();
  }
  if (IsKeyPressed(KEY_U)) {
    state->b_show_collision_placement_screen = not state->b_show_collision_placement_screen;
    state->b_show_tilesheet_tile_selection_screen = false;
    state->b_show_prop_selection_screen = false;
    scene_editor_selection_cleanup();
  }
  if (IsKeyReleased(KEY_BACKSPACE)) {
    if (state->mouse_focus == MOUSE_FOCUS_MAP and state->selection_type == SLC_TYPE_SLC_PROP_STATIC and not state->b_dragging_map_element) {
      if(not _remove_prop_cur_map_by_id(state->selected_prop_static_map_prop_address)) {
        IWARN("scene_editor::editor_update_keyboard_bindings()::Removing property failed.");
      }
      state->selected_prop_static_map_prop_address = nullptr;
      state->selection_type = SLC_TYPE_UNSELECTED;
    }
    else if (state->mouse_focus == MOUSE_FOCUS_MAP and state->selection_type == SLC_TYPE_SLC_PROP_SPRITE and not state->b_dragging_map_element) {
      if(not _remove_prop_cur_map_by_id(state->selected_prop_sprite_map_prop_address)) {
        IWARN("scene_editor::editor_update_keyboard_bindings()::Removing property failed.");
      }
      state->selected_prop_sprite_map_prop_address = nullptr;
      state->selection_type = SLC_TYPE_UNSELECTED;
    }
    else if (state->selection_type == SLC_TYPE_SLC_MAP_COLLISION and not state->b_dragging_map_element) {
      if (not state->sel_map_coll_addr_from_map or state->sel_map_coll_addr_from_map == nullptr or not remove_map_collision_by_id(state->sel_map_coll_addr_from_map->coll_id)) {
        IWARN("scene_editor::editor_update_keyboard_bindings()::No collision found");
      }
      state->sel_map_coll_addr_from_map = nullptr;
    }
  }
}
constexpr void editor_update_movement(void) {
  f32 speed = 10.f;

  if (IsKeyDown(KEY_W)) {
    state->target.y -= speed;
  }
  if (IsKeyDown(KEY_A)) {
    state->target.x -= speed;
  }
  if (IsKeyDown(KEY_S)) {
    state->target.y += speed;
  }
  if (IsKeyDown(KEY_D)) {
    state->target.x += speed;
  }
}
// BINDINGS

constexpr void update_tilemap_prop_type(void) {
  if (not state or state == nullptr) {
    IWARN("scene_editor::update_tilemap_prop_type()::State is not valid");
    return;
  }
  i32 prop_type_value = get_slider_current_value(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->data.i32[0];
  tilemap_prop_types selected_prop_type = static_cast<tilemap_prop_types>(prop_type_value);
  switch (selected_prop_type) {
    case TILEMAP_PROP_TYPE_TREE:      { state->tilemap_props_static_selected = state->tilemap_props_trees;      break; }
    case TILEMAP_PROP_TYPE_TOMBSTONE: { state->tilemap_props_static_selected = state->tilemap_props_tombstones; break; }
    case TILEMAP_PROP_TYPE_STONE:     { state->tilemap_props_static_selected = state->tilemap_props_stones;     break; }
    case TILEMAP_PROP_TYPE_SPIKE:     { state->tilemap_props_static_selected = state->tilemap_props_spikes;     break; }
    case TILEMAP_PROP_TYPE_SKULL:     { state->tilemap_props_static_selected = state->tilemap_props_skulls;     break; }
    case TILEMAP_PROP_TYPE_PILLAR:    { state->tilemap_props_static_selected = state->tilemap_props_pillars;    break; }
    case TILEMAP_PROP_TYPE_LAMP:      { state->tilemap_props_static_selected = state->tilemap_props_lamps;      break; }
    case TILEMAP_PROP_TYPE_FENCE:     { state->tilemap_props_static_selected = state->tilemap_props_fence;      break; }
    case TILEMAP_PROP_TYPE_DETAIL:    { state->tilemap_props_static_selected = state->tilemap_props_details;    break; }
    case TILEMAP_PROP_TYPE_CANDLE:    { state->tilemap_props_static_selected = state->tilemap_props_candles;    break; }
    case TILEMAP_PROP_TYPE_BUILDING:  { state->tilemap_props_static_selected = state->tilemap_props_buildings;  break; }
    case TILEMAP_PROP_TYPE_SPRITE:    { state->tilemap_props_sprite_selected = state->tilemap_props_sprite;     break; }
    default: { 
      IWARN("scene_editor::initialize_scene_editor()::Unsupported tilemap prop type");
      ui_set_slider_current_index(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER, 1);
      state->tilemap_props_static_selected = state->tilemap_props_trees;
      break; 
    }
  }
}

constexpr bool scene_editor_scale_slider_on_left_button_trigger(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::scene_editor_scale_slider_on_left_button_trigger():: State is not valid");
    return false;
  }
  if (state->selected_prop_static_map_prop_address != nullptr) {
    tilemap_prop_static *const map_prop_ptr = state->selected_prop_static_map_prop_address;
    map_prop_ptr->scale -= .15f;
    
    return true;
  }
  if (state->selected_prop_sprite_map_prop_address != nullptr) {
    tilemap_prop_sprite *const map_prop_ptr = state->selected_prop_sprite_map_prop_address;
    map_prop_ptr->scale -= .15f;
    
    map_prop_ptr->sprite.coord.width = map_prop_ptr->sprite.current_frame_rect.width * map_prop_ptr->scale;
    map_prop_ptr->sprite.coord.height = map_prop_ptr->sprite.current_frame_rect.height * map_prop_ptr->scale;
    map_prop_ptr->sprite.origin.x = map_prop_ptr->sprite.coord.width  * .5f;
    map_prop_ptr->sprite.origin.y = map_prop_ptr->sprite.coord.height * .5f;
    return true;
  }

  return false;
}
constexpr bool scene_editor_scale_slider_on_right_button_trigger(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::scene_editor_scale_slider_on_right_button_trigger():: State is not valid");
    return false;
  }
  if (state->selected_prop_static_map_prop_address != nullptr) {
    tilemap_prop_static *const map_prop_ptr = state->selected_prop_static_map_prop_address;
    map_prop_ptr->scale += .15f;
    return true;
  }
  if (state->selected_prop_sprite_map_prop_address != nullptr) {
    tilemap_prop_sprite *const map_prop_ptr = state->selected_prop_sprite_map_prop_address;
    map_prop_ptr->scale += .15f;
    
    map_prop_ptr->sprite.coord.width = map_prop_ptr->sprite.current_frame_rect.width * map_prop_ptr->scale;
    map_prop_ptr->sprite.coord.height = map_prop_ptr->sprite.current_frame_rect.height * map_prop_ptr->scale;
    map_prop_ptr->sprite.origin.x = map_prop_ptr->sprite.coord.width  * .5f;
    map_prop_ptr->sprite.origin.y = map_prop_ptr->sprite.coord.height * .5f;
    return true;
  }

  return false;
}
constexpr bool scene_editor_rotation_slider_on_left_button_trigger(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::scene_editor_rotation_slider_on_left_button_trigger():: State is not valid");
    return false;
  }
  if (state->selected_prop_static_map_prop_address != nullptr) {
    state->selected_prop_static_map_prop_address->rotation -= 10.f;
    if(state->selected_prop_static_map_prop_address->rotation < 0.f) {
      state->selected_prop_static_map_prop_address->rotation += 360.f;
    }
    state->selected_prop_static_map_prop_address->rotation = std::clamp(state->selected_prop_static_map_prop_address->rotation, 0.f, 360.f);
    return true;
  }
  if (state->selected_prop_sprite_map_prop_address != nullptr) {
    state->selected_prop_sprite_map_prop_address->sprite.rotation -= 10.f;
    if(state->selected_prop_sprite_map_prop_address->sprite.rotation < 0.f) {
      state->selected_prop_sprite_map_prop_address->sprite.rotation += 360.f;
    }
    state->selected_prop_sprite_map_prop_address->sprite.rotation = std::clamp(state->selected_prop_sprite_map_prop_address->sprite.rotation, 0.f, 360.f);
    return true;
  }

  return false;
}
constexpr bool scene_editor_rotation_slider_on_right_button_trigger(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::scene_editor_rotation_slider_on_right_button_trigger():: State is not valid");
    return false;
  }
  if (state->selected_prop_static_map_prop_address != nullptr) {
    state->selected_prop_static_map_prop_address->rotation += 10.f;
    if(state->selected_prop_static_map_prop_address->rotation > 360.f) {
      state->selected_prop_static_map_prop_address->rotation -= 360.f;
    }
    state->selected_prop_static_map_prop_address->rotation = std::clamp(state->selected_prop_static_map_prop_address->rotation, 0.f, 360.f);
    return true;
  }
  if (state->selected_prop_sprite_map_prop_address != nullptr) {
    state->selected_prop_sprite_map_prop_address->sprite.rotation += 10.f;
    if(state->selected_prop_sprite_map_prop_address->sprite.rotation > 360.f) {
      state->selected_prop_sprite_map_prop_address->sprite.rotation -= 360.f;
    }
    state->selected_prop_sprite_map_prop_address->sprite.rotation = std::clamp(state->selected_prop_sprite_map_prop_address->sprite.rotation, 0.f, 360.f);
    return true;
  }

  return false;
}
constexpr bool scene_editor_zindex_slider_on_left_button_trigger(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::scene_editor_zindex_slider_on_left_button_trigger():: State is not valid");
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
constexpr bool scene_editor_zindex_slider_on_right_button_trigger(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::scene_editor_zindex_slider_on_right_button_trigger():: State is not valid");
    return false;
  }
  if (state->selected_prop_static_map_prop_address != nullptr) {
    state->selected_prop_static_map_prop_address->zindex += 1;
    if(state->selected_prop_static_map_prop_address->zindex >= MAX_Z_INDEX_SLOT) {
      state->selected_prop_static_map_prop_address->zindex = MAX_Z_INDEX_SLOT-1;
    }
    refresh_render_queue(state->selected_stage);
    return true;
  }
  if (state->selected_prop_sprite_map_prop_address != nullptr) {
    state->selected_prop_sprite_map_prop_address->zindex += 1;
    if(state->selected_prop_sprite_map_prop_address->zindex >= MAX_Z_INDEX_SLOT) {
      state->selected_prop_sprite_map_prop_address->zindex = MAX_Z_INDEX_SLOT-1;
    }
    refresh_render_queue(state->selected_stage);
    return true;
  }

  return false;
}

constexpr bool scene_editor_map_layer_slc_slider_on_left_button_trigger(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::scene_editor_map_layer_slc_slider_on_left_button_trigger():: State is not valid");
    return false;
  }
  state->edit_layer--;
  if (state->edit_layer > MAX_TILEMAP_LAYERS - 1) {
    state->edit_layer = MAX_TILEMAP_LAYERS - 1;
  }
  state->edit_layer = std::clamp(static_cast<i32>(state->edit_layer), 0, MAX_TILEMAP_LAYERS - 1);
  get_slider_by_id(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", state->edit_layer);
  return true; 
}
constexpr bool scene_editor_map_layer_slc_slider_on_right_button_trigger(void) { 
  if (not state or state == nullptr) {
    IERROR("scene_editor::scene_editor_map_layer_slc_slider_on_right_button_trigger():: State is not valid");
    return false;
  }
  state->edit_layer++;
  if (state->edit_layer > MAX_TILEMAP_LAYERS - 1) {
    state->edit_layer = 0;
  }
  state->edit_layer = std::clamp(static_cast<i32>(state->edit_layer), 0, MAX_TILEMAP_LAYERS - 1);
  get_slider_by_id(SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", state->edit_layer);
  return true; 
}
constexpr bool scene_editor_map_stage_slc_slider_on_left_button_trigger(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::scene_editor_map_stage_slc_slider_on_left_button_trigger():: State is not valid");
    return false;
  }
  state->selected_stage--;
  if (state->selected_stage >= MAX_WORLDMAP_LOCATIONS) {
    state->selected_stage = MAX_WORLDMAP_LOCATIONS - 1;
  }
  state->selected_stage = std::clamp(static_cast<i32>(state->selected_stage), 0, MAX_WORLDMAP_LOCATIONS - 1);
  set_worldmap_location(state->selected_stage);
  get_slider_by_id(SDR_ID_EDITOR_MAP_STAGE_SLC_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", state->selected_stage);
  return true; 
}
constexpr bool scene_editor_map_stage_slc_slider_on_right_button_trigger(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::scene_editor_map_stage_slc_slider_on_right_button_trigger():: State is not valid");
    return false;
  }
  state->selected_stage++;
  if (state->selected_stage >= MAX_WORLDMAP_LOCATIONS) {
    state->selected_stage = 0;
  }
  state->selected_stage = std::clamp(static_cast<i32>(state->selected_stage), 0, MAX_WORLDMAP_LOCATIONS - 1);
  set_worldmap_location(state->selected_stage);
  get_slider_by_id(SDR_ID_EDITOR_MAP_STAGE_SLC_SLIDER)->options.at(0).no_localized_text = TextFormat("%d", state->selected_stage);
  return true; 
}
constexpr bool scene_editor_map_prop_type_slc_slider_on_left_button_trigger(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::scene_editor_map_prop_type_slc_slider_on_left_button_trigger():: State is not valid");
    return false;
  }
  i32 & current_index = get_slider_by_id(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->current_value;

  current_index--;
  if (current_index < 0) {
    current_index = 0;
  }
  update_tilemap_prop_type();
  return true; 
}
constexpr bool scene_editor_map_prop_type_slc_slider_on_right_button_trigger(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::scene_editor_map_prop_type_slc_slider_on_right_button_trigger():: State is not valid");
    return false;
  }
  i32& current_index = get_slider_by_id(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->current_value;

  current_index++;
  if (static_cast<size_t>(current_index) >= get_slider_by_id(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->options.size()) {
    current_index = get_slider_by_id(SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER)->options.size() - 1;
  }
  update_tilemap_prop_type();
  return true;
}

constexpr bool scene_editor_is_map_prop_y_based_checkbox_on_change_trigger(void) {
  if (not state or state == nullptr) {
    IERROR("scene_editor::scene_editor_is_map_prop_y_based_checkbox_on_change_trigger()::State is not valid");
    return false;
  }
  bool is_checked = get_checkbox_by_id(CHECKBOX_ID_IS_PROP_YBASED)->state == CHECKBOX_STATE_CHECKED;

  if (state->selected_prop_sprite_map_prop_address != nullptr) {
    state->selected_prop_sprite_map_prop_address->use_y_based_zindex = is_checked;
    refresh_render_queue(state->selected_stage);
    return true;
  }
  if (state->selected_prop_static_map_prop_address != nullptr) {
    state->selected_prop_static_map_prop_address->use_y_based_zindex = is_checked;
    refresh_render_queue(state->selected_stage);
    return true;
  }
  return false;
}

constexpr void scene_editor_selection_cleanup(void) {
  state->selected_tile = tile();
  state->selected_prop_static_map_prop_address = nullptr;
  state->selected_prop_sprite_map_prop_address = nullptr;
  state->selected_prop_static_panel_selection_copy = tilemap_prop_static();
  state->selected_prop_sprite_panel_selection_copy = tilemap_prop_sprite();
  state->selection_type = SLC_TYPE_UNSELECTED;
  state->map_collision_buffer_to_place = map_collision();
  state->sel_map_coll_addr_from_map = nullptr;
  state->b_dragging_map_element = false;
}
void se_begin_fadeout(data128 data, void(*on_change_complete)(data128)) {
  if (not state or state == nullptr ) {
    IERROR("scene_editor::se_begin_fadeout()::State is not valid");
    return;
  }
  state->se_fade.fade_animation_duration = EDITOR_FADE_DURATION;
  state->se_fade.fade_type = FADE_TYPE_FADEOUT;
  state->se_fade.fade_animation_accumulator = 0.f;
  state->se_fade.fade_animation_playing = true;
  state->se_fade.is_fade_animation_played = false;
  state->se_fade.data = data;
  state->se_fade.on_change_complete = on_change_complete;
}
void se_begin_fadein(data128 data, void(*on_change_complete)(data128)) {
  if (not state or state == nullptr ) {
    IERROR("scene_editor::se_begin_fadein()::State is not valid");
    return;
  }
  state->se_fade.fade_animation_duration = EDITOR_FADE_DURATION;
  state->se_fade.fade_type = FADE_TYPE_FADEIN;
  state->se_fade.fade_animation_accumulator = 0.f;
  state->se_fade.fade_animation_playing = true;
  state->se_fade.is_fade_animation_played = false;
  state->se_fade.data = data;
  state->se_fade.on_change_complete = on_change_complete;
}

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
#undef SE_BASE_RENDER_WIDTH 
#undef SE_BASE_RENDER_HEIGHT
#undef PROP_DRAG_HANDLE_DIM
#undef PROP_DRAG_HANDLE_DIM_DIV2
#undef PROP_PANEL_PROP_DRAW_STARTING_HEIGHT
#undef MAP_COLLISION_DRAG_HANDLE_DIM
#undef MAP_COLLISION_DRAG_HANDLE_DIM_DIV2
#undef MAP_COLLISION_PANEL_COLLISION_DRAW_STARTING_HEIGHT
#undef EDITOR_FADE_DURATION
