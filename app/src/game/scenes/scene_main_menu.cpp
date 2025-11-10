#include "scene_main_menu.h"

#include <cmath>   // For std::cos, std::sin

#include <reasings.h>
#include <steam/steam_api.h>
#include <loc_types.h>
#include <sound.h>

#include "core/fmemory.h"
#include "core/event.h"
#include "core/logger.h"

#include "game/game_manager.h"
#include "game/user_interface.h"
#include "game/world.h"
#include "game/camera.h"

enum main_menu_scene_type {
  MAIN_MENU_SCENE_DEFAULT,
  MAIN_MENU_SCENE_SETTINGS,
  MAIN_MENU_SCENE_CHARACTER,
  MAIN_MENU_SCENE_EXTRAS,
  MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE,
  MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE,
};
enum main_menu_scene_character_subscene_type {
  MAIN_MENU_SCENE_CHARACTER_UNDEFINED,
  MAIN_MENU_SCENE_CHARACTER_UPGRADE,
  MAIN_MENU_SCENE_CHARACTER_INVENTORY,
  MAIN_MENU_SCENE_CHARACTER_GAME_RULE,
  MAIN_MENU_SCENE_CHARACTER_MAX,
};

struct play_scene_info {
  i32 hovered_stage;
  play_scene_info(void) {
    this->hovered_stage = 0;
  }
};
struct mms_trait_ability_selection_context {
  panel ability_selection_panel;
  panel positive_traits_selection_panel;
  panel negative_traits_selection_panel;
  panel chosen_traits_selection_panel;
  std::array<ability, ABILITY_ID_MAX> abilities;
  std::vector<character_trait> chosen_traits;
  ability_id starter_ability;
  std::vector<character_trait> positive_traits;
  std::vector<character_trait> negative_traits;
  i32 available_trait_points;
  const game_rule * selected_rule;

  mms_trait_ability_selection_context(void) {
    this->starter_ability = ABILITY_ID_UNDEFINED ;
    this->available_trait_points = 0;
    this->selected_rule = nullptr;
  }
};
struct mms_default_panels {
  panel panel_dark_fantasy_selected_default;
  panel panel_active_dark_fantasy_default;
  panel panel_dark_fantasy_default;
  mms_default_panels(void) {
  }
};
struct mms_character_game_rule_context {
  panel rule_list_panel;
  panel rule_details_panel;
  const game_rule * selected_rule;
  mms_character_game_rule_context(void) {
    this->selected_rule = nullptr;
  }
};
struct mms_character_inventory_context {
  panel inventory_item_list_panel;
  panel inventory_slots_panel;
  mms_character_inventory_context(void) {
  }
};

#warning "LABEL: upgrade state"
struct mms_character_upgrade_context {
  panel item_list_panel;
  panel upgrade_parent_panel;
  panel left_item_panel;
  panel right_item_panel;
  panel result_item_panel;
  enum character_upgrade_scene_state {
    CHARACTER_UPGRADE_STATE_UNDEFINED,
    CHARACTER_UPGRADE_STATE_IDLE,
    CHARACTER_UPGRADE_STATE_UPGRADE,
    CHARACTER_UPGRADE_STATE_UPGRADE_SUCCESS,
    CHARACTER_UPGRADE_STATE_UPGRADE_FAILURE,
    CHARACTER_UPGRADE_STATE_ERROR_UNSUFFICENT,
    CHARACTER_UPGRADE_STATE_MAX,
  } upgrade_state;
  size_t slot_index;
  atlas_texture_id product_item_tex;
  mms_character_upgrade_context(void) {
    this->upgrade_state = CHARACTER_UPGRADE_STATE_UNDEFINED;
    this->slot_index = I32_MAX;
  }
};

struct main_menu_scene_state {
  panel worldmap_selection_panel;
  playlist_control_system_state playlist;
  mms_default_panels smm_default_panels;
  mms_trait_ability_selection_context mms_trait_choice;
  mms_character_inventory_context mms_character_inventory;
  mms_character_upgrade_context mms_character_upgrade;
  mms_character_game_rule_context mms_character_game_rule;
  const camera_metrics * in_camera_metrics;
  const app_settings   * in_app_settings;
  const ingame_info * in_ingame_info;

  std::array<worldmap_stage, MAX_WORLDMAP_LOCATIONS> worldmap_locations;
  
  const Vector2* mouse_pos_screen;
  f32 deny_notify_timer;
  f32 bg_scroll;
  main_menu_scene_type mainmenu_state;
  main_menu_scene_character_subscene_type mainmenu_scene_character_subscene;
  bool is_dragging_scroll;
  bool is_any_button_down;
  bool is_changing_state;

  play_scene_info ingame_scene_feed;
  ui_fade_control_system smm_fade;
  std::vector<local_button> general_purpose_buttons;
  std::vector<panel> general_purpose_panels;
  i32 next_local_button_id;
  i32 next_local_panel_id;

  main_menu_scene_state(void) {
    this->in_camera_metrics = nullptr;
    this->in_app_settings = nullptr;
    this->in_ingame_info = nullptr;
    
    this->mouse_pos_screen = nullptr;
    this->deny_notify_timer = 0.f;
    this->bg_scroll = 0.f;
    this->mainmenu_state = MAIN_MENU_SCENE_DEFAULT;
    this->mainmenu_scene_character_subscene = MAIN_MENU_SCENE_CHARACTER_GAME_RULE;
    this->is_dragging_scroll = false;
    this->is_any_button_down = false;
    this->is_changing_state = false;

    this->next_local_button_id = 0;
    this->next_local_panel_id = 0;
  }
};

static main_menu_scene_state * state = nullptr;

#define DENY_NOTIFY_TIME .8f

#define MAIN_MENU_FADE_DURATION .6f
#define MAIN_MENU_UPGRADE_PANEL_COL 3
#define MAIN_MENU_UPGRADE_PANEL_ROW 3

#define SMM_BASE_RENDER_SCALE(SCALE) VECTOR2(\
  static_cast<f32>(state->in_app_settings->render_width * SCALE),\
  static_cast<f32>(state->in_app_settings->render_height * SCALE))
#define SMM_BASE_RENDER_DIV2 VECTOR2(static_cast<f32>(state->in_app_settings->render_width_div2), static_cast<f32>(state->in_app_settings->render_height_div2))
#define SMM_BASE_RENDER_RES_VEC VECTOR2(static_cast<f32>(state->in_app_settings->render_width), static_cast<f32>(state->in_app_settings->render_height))
#define SMM_BASE_RENDER_WIDTH state->in_app_settings->render_width
#define SMM_BASE_RENDER_HEIGHT state->in_app_settings->render_height

#define SMM_MAP_PIN_SOURCE_LOCATION_UP (Rectangle{1600, 928, 32, 32})
#define SMM_MAP_PIN_SOURCE_LOCATION_HOVER (Rectangle{1632, 928, 32, 32})
#define SMM_SCROLL_HANDLE_HEIGHT state->in_app_settings->render_height * .05f
#define ZOOM_CAMERA_MAP_ENTRY 2.0f
#define UI_CHARACTER_TRAIT_DISPLAY 0
#define UI_CHARACTER_TRAIT_HIDDEN 1

[[__nodiscard__]] bool begin_scene_main_menu(bool fade_in);

void draw_main_menu_character_panel(void);
void draw_main_menu_character_subscene_upgrade_upgrade_panel(Rectangle parent_dest, f32 padding);
void draw_main_menu_character_subscene_upgrade_item_list_panel(Rectangle parent_dest, f32 padding);
void draw_main_menu_character_subscene_inventory_item_list_panel(Rectangle parent_dest, f32 padding);
void draw_main_menu_character_subscene_inventory_slots_panel(Rectangle parent_dest, f32 padding);
void draw_main_menu_character_subscene_game_rule_list_panel(Rectangle parent_dest, f32 padding);
void draw_main_menu_character_subscene_game_rule_details_panel(Rectangle parent_dest, f32 padding);
void draw_trait_selection_panel(void);
void trait_selection_panel_list_traits(panel* const pnl, const Rectangle rect, std::vector<character_trait>& traits, void (*trait_button_on_click_pfn)(i32 index));
void trait_selection_panel_list_ability_selection_panel(panel* const pnl, const Rectangle rect, Vector2 border_gap);
void smm_update_mouse_bindings(void);
void smm_update_keyboard_bindings(void);
void smm_update_bindings(void);
void smm_update_local_buttons(void);
void smm_begin_fadeout(data128 data, void(*on_change_complete)(data128));
void smm_begin_fadein(data128 data, void(*on_change_complete)(data128));
void smm_clean_character_context(void);
void smm_refresh_character_context(void);
local_button* smm_add_local_button(i32 _id, button_type_id _btn_type_id, button_state signal_state);
panel* smm_add_local_panel(i32 _id, panel pnl);
local_button* smm_get_local_button(i32 _id);
panel* smm_get_local_panel(i32 _id);
void fade_on_complete_change_main_menu_type(data128 data);
void fade_on_complete_change_scene(data128 data);

void begin_scene_change(main_menu_scene_type mms, event_context context = event_context());
void begin_scene_change(scene_id sid, event_context context = event_context());

void positive_trait_button_on_click(i32 index);
void negative_trait_button_on_click(i32 index);
void chosen_trait_button_on_click(i32 index);

[[__nodiscard__]] bool initialize_scene_main_menu(const app_settings * _in_app_settings, bool fade_in) {
  if (state and state != nullptr) {
    return begin_scene_main_menu(fade_in);
  }
  state = (main_menu_scene_state *)allocate_memory_linear(sizeof(main_menu_scene_state), true);
  if (not state or state == nullptr) {
    IERROR("scene_main_menu::initialize_scene_main_menu()::State allocation failed");
    return false;
  }
  *state = main_menu_scene_state();
   
  if (not _in_app_settings or _in_app_settings == nullptr) {
    IERROR("scene_main_menu::initialize_scene_main_menu()::App setting pointer is invalid");
    return false;
  }
  state->in_app_settings = _in_app_settings;

  if(not create_camera(state->in_app_settings->render_width_div2, state->in_app_settings->render_height_div2,state->in_app_settings->render_width, state->in_app_settings->render_height)) {
    IERROR("scene_main_menu::initialize_scene_main_menu()::Creating camera failed");
    return false;
  }
  state->in_camera_metrics = get_in_game_camera();
  if (not state->in_camera_metrics or state->in_camera_metrics == nullptr) {
    IERROR("scene_main_menu::initialize_scene_main_menu()::Camera pointer is invalid");
    return false;
  }
  if(not world_system_begin(state->in_camera_metrics)) {
    IERROR("scene_main_menu::initialize_scene_main_menu()::World system begin failed");
    return false;
  }
  if (not sound_system_initialize()) {
    IERROR("scene_main_menu::initialize_scene_main_menu()::Sound system init failed");
    return false;
  }
  state->playlist = create_playlist(PLAYLIST_PRESET_MAINMENU_LIST);

  return begin_scene_main_menu(fade_in);
}
[[__nodiscard__]] bool begin_scene_main_menu(bool fade_in) {
  if (not user_interface_system_initialize(get_in_game_camera())) {
    IERROR("scene_main_menu::begin_scene_main_menu()::User interface failed to initialize!");
    return false;
  }

  // NOTE: Worldmap index 0 is mainmenu background now
  // NOTE: Also game manager requires a valid active map pointer
  set_worldmap_location(WORLDMAP_MAINMENU_MAP); 

  if (not game_manager_initialize( state->in_camera_metrics, state->in_app_settings, get_active_map_ptr())) { // Inits player & spawns
    IERROR("scene_in_game::begin_scene_main_menu()::game_manager_initialize() failed");
    return false;
  }
  state->in_ingame_info = gm_get_ingame_info();
  state->worldmap_locations = get_worldmap_locations();

  state->mainmenu_state = MAIN_MENU_SCENE_DEFAULT;
  Color bg_black_tint = { 30, 39, 46, 32};
  Color bg_black_tint_hover = { 52, 64, 76, 196};
  Color crimson_fantasy_bg_tint = { 30, 39, 46, 64 };
  Vector4 dark_fantasy_frame_offsets = {6, 6, 6, 6};
  Vector4 crimson_fantasy_frame_offsets = {6, 6, 6, 6};
  Vector4 crimson_fantasy_ornate_frame_offsets = {10, 10, 10, 10};
  state->smm_default_panels.panel_dark_fantasy_selected_default = panel( BTN_STATE_UNDEFINED, ATLAS_TEX_ID_BG_BLACK, ATLAS_TEX_ID_DARK_FANTASY_PANEL_SELECTED, 
    Vector4 {10, 10, 10, 10}, bg_black_tint
  );
  state->smm_default_panels.panel_dark_fantasy_default = panel( BTN_STATE_UNDEFINED, ATLAS_TEX_ID_BG_BLACK, ATLAS_TEX_ID_DARK_FANTASY_PANEL, 
    dark_fantasy_frame_offsets, bg_black_tint
  );
  state->smm_default_panels.panel_active_dark_fantasy_default = panel(BTN_STATE_RELEASED, ATLAS_TEX_ID_BG_BLACK, ATLAS_TEX_ID_DARK_FANTASY_PANEL, 
    dark_fantasy_frame_offsets, bg_black_tint, bg_black_tint_hover
  );
  state->mms_trait_choice.positive_traits_selection_panel = state->smm_default_panels.panel_dark_fantasy_default;
  state->mms_trait_choice.negative_traits_selection_panel = state->smm_default_panels.panel_dark_fantasy_default;
  state->mms_trait_choice.chosen_traits_selection_panel = state->smm_default_panels.panel_dark_fantasy_default;
  state->mms_trait_choice.ability_selection_panel = state->smm_default_panels.panel_dark_fantasy_default;

  state->worldmap_selection_panel = state->smm_default_panels.panel_dark_fantasy_selected_default;

  state->mms_character_game_rule.rule_details_panel = state->smm_default_panels.panel_dark_fantasy_default;

  state->mms_character_inventory.inventory_item_list_panel = state->smm_default_panels.panel_dark_fantasy_default;
  state->mms_character_inventory.inventory_slots_panel = state->smm_default_panels.panel_dark_fantasy_default;

  state->mms_character_upgrade.item_list_panel = state->smm_default_panels.panel_dark_fantasy_default;
  state->mms_character_upgrade.upgrade_parent_panel = state->smm_default_panels.panel_dark_fantasy_default;
  state->mms_character_upgrade.left_item_panel = panel(BTN_STATE_UNDEFINED, ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG, ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL, 
    crimson_fantasy_frame_offsets, crimson_fantasy_bg_tint
  );
  state->mms_character_upgrade.right_item_panel = state->mms_character_upgrade.left_item_panel;
  state->mms_character_upgrade.result_item_panel = panel(BTN_STATE_UNDEFINED, ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG, ATLAS_TEX_ID_CRIMSON_FANTASY_ORNATE_FRAME, 
    crimson_fantasy_ornate_frame_offsets, crimson_fantasy_bg_tint
  );
  if (fade_in) {
    smm_begin_fadein(data128(static_cast<i32>(MAIN_MENU_SCENE_DEFAULT)), fade_on_complete_change_main_menu_type);
  }
  
  return true;
}
void end_scene_main_menu(void) {
  event_fire(EVENT_CODE_RESET_MUSIC, event_context((i32)MUSIC_ID_MAINMENU_THEME1));

  state->mms_trait_choice.positive_traits.clear();
  state->mms_trait_choice.negative_traits.clear();
  state->mms_trait_choice.available_trait_points = 0;
  state->mms_trait_choice.abilities.fill(ability());
  state->mms_trait_choice.chosen_traits.clear();
  state->mms_trait_choice.starter_ability = ABILITY_ID_UNDEFINED;
  state->mms_trait_choice.selected_rule = nullptr ;
  state->worldmap_locations.fill(worldmap_stage());
  state->deny_notify_timer = 0.f;
  state->bg_scroll = 0.f;
  state->mainmenu_state = MAIN_MENU_SCENE_DEFAULT;
  state->mainmenu_scene_character_subscene = MAIN_MENU_SCENE_CHARACTER_UNDEFINED;
  state->is_dragging_scroll = false;
  state->is_any_button_down = false;
  state->is_changing_state = false;
  state->ingame_scene_feed = play_scene_info();
  state->smm_fade = ui_fade_control_system();
  state->general_purpose_buttons.clear();
  state->general_purpose_panels.clear();
  state->next_local_button_id = 0;
  state->next_local_panel_id = 0;

  ui_refresh_setting_sliders_to_default();
}

void update_scene_main_menu(void) {
  
  update_user_interface(GetFrameTime());
  state->mouse_pos_screen = ui_get_mouse_pos_screen();
  smm_update_bindings();
  update_camera(GetFrameTime());
  update_map(GetFrameTime());
  update_sound_system();

  switch (state->mainmenu_state) {
    case MAIN_MENU_SCENE_DEFAULT: {
      event_fire(EVENT_CODE_CAMERA_SET_ZOOM, event_context(1.f));
      event_fire(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, event_context(0.f, 0.f));
      event_fire(EVENT_CODE_CAMERA_SET_OFFSET, event_context(state->in_app_settings->render_width * .5f,state->in_app_settings->render_height * .5f));
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE: {
      event_fire(EVENT_CODE_CAMERA_SET_OFFSET, event_context(state->in_app_settings->render_width * .5f,state->in_app_settings->render_height * .5f));
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE: {
      smm_update_local_buttons();
      break;
    }
    case MAIN_MENU_SCENE_SETTINGS: {
      break;
    }
    case MAIN_MENU_SCENE_EXTRAS: {
      break;
    }
    case MAIN_MENU_SCENE_CHARACTER: {
      break;
    }
    default: {
      break;
    }
  }

  if(state->smm_fade.fade_animation_playing){
    process_fade_effect(__builtin_addressof(state->smm_fade));
  }
  else if (state->smm_fade.is_fade_animation_played and state->smm_fade.on_change_complete != nullptr) {
    state->smm_fade.on_change_complete(state->smm_fade.data);
  }
}
void render_scene_main_menu(void) {
  BeginMode2D(get_in_game_camera()->handle);
  
  switch (state->mainmenu_state) {
    case MAIN_MENU_SCENE_DEFAULT: {
      render_map();
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE: {
      const f32 image_ratio = 3840.f / 2160.f; // INFO: ratio of map texture
      const f32 image_height = static_cast<f32>(SMM_BASE_RENDER_HEIGHT * .85f);
      const Vector2 image_dim = Vector2 { image_height * image_ratio, image_height};
      Rectangle map_choice_image_dest = Rectangle { SMM_BASE_RENDER_DIV2.x, 0.f, image_dim.x, image_dim.y };
      map_choice_image_dest.x -= image_dim.x * .5f;
      const f32 map_pin_dim = map_choice_image_dest.height * .05f;

      state->bg_scroll += static_cast<f32>(GetFrameTime() * 25.f);

      gui_draw_texture_id_pro(TEX_ID_BLACK_BACKGROUND_IMG1, 
        Rectangle {
          state->bg_scroll, state->bg_scroll, 
          static_cast<f32>(state->in_app_settings->render_width), 
          static_cast<f32>(state->in_app_settings->render_height)
        }, 
        Rectangle {0.f, 0.f, 
          static_cast<f32>(state->in_app_settings->render_width), 
          static_cast<f32>(state->in_app_settings->render_height)
        }, 
        WHITE, ZEROVEC2, TEXTURE_WRAP_REPEAT
      );

      //BeginShaderMode(get_shader_by_enum(SHADER_ID_MAP_CHOICE_IMAGE)->handle);
      {
        gui_draw_texture_id(TEX_ID_WORLDMAP_WO_CLOUDS, map_choice_image_dest, ZEROVEC2);
      }
      //EndShaderMode();

      for (i32 itr_000 = 0; itr_000 < MAX_WORLDMAP_LOCATIONS; ++itr_000) {
        if (not state->worldmap_locations.at(itr_000).display_on_screen) {
          continue;
        }
        const Vector2& normalized_pin_location = state->worldmap_locations.at(itr_000).screen_location;
        const Rectangle pin_location = Rectangle {
          map_choice_image_dest.x + normalized_pin_location.x * map_choice_image_dest.width - (map_pin_dim * .5f), 
          map_choice_image_dest.y + normalized_pin_location.y * map_choice_image_dest.height - (map_pin_dim * .5f),
          map_pin_dim, map_pin_dim,
        };

        gui_draw_texture_id_pro(TEX_ID_ASSET_ATLAS, SMM_MAP_PIN_SOURCE_LOCATION_UP, pin_location);
      }

      for (i32 itr_000 = 0; (itr_000 < MAX_WORLDMAP_LOCATIONS and not state->is_changing_state); ++itr_000) {
        if (not state->worldmap_locations.at(itr_000).display_on_screen) {
          continue;
        }
        if(state->ingame_scene_feed.hovered_stage != itr_000) {
          continue;
        }
        const worldmap_stage& worldmap_locations = state->worldmap_locations.at(itr_000);
        const Vector2& normalized_pin_location = state->worldmap_locations.at(itr_000).screen_location;
        const Rectangle pin_location = Rectangle {
          map_choice_image_dest.x + normalized_pin_location.x * map_choice_image_dest.width - (map_pin_dim * .5f), 
          map_choice_image_dest.y + normalized_pin_location.y * map_choice_image_dest.height - (map_pin_dim * .5f),
          map_pin_dim, map_pin_dim,
        };

        gui_draw_texture_id_pro(TEX_ID_ASSET_ATLAS, SMM_MAP_PIN_SOURCE_LOCATION_HOVER, pin_location); // INFO: MAP PIN TEXTURES
        const f32 image_ratio = 3840.f / 2160.f; // INFO: ratio of map texture
        const f32 image_height = static_cast<f32>(SMM_BASE_RENDER_HEIGHT * .85f);
        const Vector2 image_dim = Vector2 { image_height * image_ratio, image_height};
        Rectangle map_choice_image_dest = Rectangle { SMM_BASE_RENDER_DIV2.x, 0.f, image_dim.x, image_dim.y };
        map_choice_image_dest.x -= image_dim.x * .5f;
        const f32 map_pin_dim = map_choice_image_dest.height * .05f;
        panel* pnl = __builtin_addressof(state->worldmap_selection_panel);
        const Rectangle scrloc = Rectangle{
          map_choice_image_dest.x + normalized_pin_location.x * map_choice_image_dest.width - (map_pin_dim * .5f), 
          map_choice_image_dest.y + normalized_pin_location.y * map_choice_image_dest.height - (map_pin_dim * .5f),
          map_pin_dim, map_pin_dim,
        };
        pnl->dest = Rectangle {scrloc.x + WORLDMAP_LOC_PIN_SIZE, scrloc.y + WORLDMAP_LOC_PIN_SIZE_DIV2, SMM_BASE_RENDER_WIDTH * .25f, SMM_BASE_RENDER_HEIGHT * .25f};
        DrawCircleGradient(scrloc.x + WORLDMAP_LOC_PIN_SIZE_DIV2, scrloc.y + WORLDMAP_LOC_PIN_SIZE_DIV2, 100, Color{236,240,241,50}, Color{255, 255, 255, 0});

        gui_panel((*pnl), pnl->dest, false);
        BeginScissorMode(pnl->dest.x, pnl->dest.y, pnl->dest.width, pnl->dest.height);
        {
          if (worldmap_locations.is_playable) {
            gui_label(lc_txt(state->worldmap_locations.at(itr_000).title_txt_id), FONT_TYPE_REGULAR, 1, Vector2 {
              pnl->dest.x + pnl->dest.width *.5f, pnl->dest.y + pnl->dest.height*.5f
            }, WHITE, true, true);
          }
          else {
            gui_label(lc_txt(LOC_TEXT_WORLD_STAGE_WORK_IN_PROGRESS), FONT_TYPE_REGULAR, 1, Vector2 {
              pnl->dest.x + pnl->dest.width *.5f, pnl->dest.y + pnl->dest.height*.5f
            }, WHITE, true, true);
          }
        }
        EndScissorMode();
      }

      if(not state->is_changing_state and 
        gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_MAP_CHOICE_BACK), BTN_ID_MAINMENU_MAP_CHOICE_BACK, VECTOR2(-35.f, 66.5f), SMM_BASE_RENDER_DIV2, true)
      ) {
        begin_scene_change(MAIN_MENU_SCENE_DEFAULT);
      }
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE: {
      render_map();
      break;
    }
    case MAIN_MENU_SCENE_SETTINGS: {
      render_map();
      break;
    }
    case MAIN_MENU_SCENE_EXTRAS: {
      break;
    }
    case MAIN_MENU_SCENE_CHARACTER: {
      render_map();
      break;
    }
    default: { break; }
  }
  
  EndMode2D();
}
void render_interface_main_menu(void) {
  if (state->mainmenu_state == MAIN_MENU_SCENE_DEFAULT) {
    Vector2 screen_location_acc = Vector2 {0.f, -21.f};
    
    gui_label_shader(GAME_TITLE, SHADER_ID_SDF_TEXT, FONT_TYPE_TITLE, 5, VECTOR2(SMM_BASE_RENDER_WIDTH * .5f, SMM_BASE_RENDER_HEIGHT * .25f), WHITE, true, true);

    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_PLAY), BTN_ID_MAINMENU_BUTTON_PLAY, screen_location_acc, SMM_BASE_RENDER_DIV2, true)) {
      begin_scene_change(MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE);
    }
    screen_location_acc.y += 10.5f;

    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_CHARACTER), BTN_ID_MAINMENU_BUTTON_ENTER_STATE_CHARACTER, screen_location_acc, SMM_BASE_RENDER_DIV2, true)) {
      begin_scene_change(MAIN_MENU_SCENE_CHARACTER);
    }
    screen_location_acc.y += 10.5f;

    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_SETTINGS), BTN_ID_MAINMENU_BUTTON_SETTINGS, screen_location_acc, SMM_BASE_RENDER_DIV2, true)) {
      ui_refresh_setting_sliders_to_default();
      begin_scene_change(MAIN_MENU_SCENE_SETTINGS);
    }
    screen_location_acc.y += 10.5f;

    #ifndef _RELEASE
      if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_EDITOR), BTN_ID_MAINMENU_BUTTON_EDITOR, screen_location_acc, SMM_BASE_RENDER_DIV2, true)) {
        smm_begin_fadeout(data128(SCENE_TYPE_EDITOR, true), fade_on_complete_change_scene);
      }
      screen_location_acc.y += 10.5f;
    #endif

    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_EXIT), BTN_ID_MAINMENU_BUTTON_EXIT, screen_location_acc, SMM_BASE_RENDER_DIV2, true)) {
      event_fire(EVENT_CODE_APPLICATION_QUIT, event_context());
    }
    screen_location_acc.y += 10.5f;
  }
  else if (state->mainmenu_state == MAIN_MENU_SCENE_SETTINGS) {
      gui_draw_settings_screen();
      if (gui_menu_button(lc_txt(LOC_TEXT_SETTINGS_BUTTON_CANCEL), BTN_ID_MAINMENU_SETTINGS_CANCEL, VECTOR2(-35.f, 66.5f), SMM_BASE_RENDER_DIV2, true)) {
        ui_refresh_setting_sliders_to_default();
        begin_scene_change(MAIN_MENU_SCENE_DEFAULT);
      }
  }
  else if (state->mainmenu_state == MAIN_MENU_SCENE_CHARACTER) {
      draw_main_menu_character_panel();
      if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_STATE_CHARACTER_BUTTON_BACK), BTN_ID_MAINMENU_STATE_CHARACTER_BACK, VECTOR2(0.f, 66.5f), SMM_BASE_RENDER_DIV2, true)) {
        smm_clean_character_context();
        begin_scene_change(MAIN_MENU_SCENE_DEFAULT);
      }
  }
  else if (state->mainmenu_state == MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE) {
  }
  else if(state->mainmenu_state == MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE) {
    draw_trait_selection_panel();

    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_TRAIT_CHOICE_BUTTON_BACK), BTN_ID_MAINMENU_TRAIT_CHOICE_BACK, VECTOR2(-35.f, 66.5f), SMM_BASE_RENDER_DIV2, true)) {
      begin_scene_change(MAIN_MENU_SCENE_DEFAULT);
    }
    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_TRAIT_CHOICE_BUTTON_ACCEPT), BTN_ID_MAINMENU_TRAIT_CHOICE_ACCEPT, VECTOR2(35.f, 66.5f), SMM_BASE_RENDER_DIV2, true)) 
    {
      if ( state->mms_trait_choice.starter_ability <= ABILITY_ID_UNDEFINED or state->mms_trait_choice.starter_ability >= ABILITY_ID_MAX) 
      {
        gui_fire_display_error(LOC_TEXT_DISPLAY_ERROR_TEXT_STARTER_ABILITY_NOT_SELECTED);
      }
      else {
        begin_scene_change(MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE);
      }
    }
  }
  render_user_interface();
}

void draw_main_menu_character_panel(void) {
  Rectangle dest = gui_draw_default_background_panel();
  Rectangle drawing_area = dest;

  constexpr f32 symmetrical_pad_percent = 0.01f;
  constexpr f32 additional_top_pad_percent = 0.04f;

  const f32 horizontal_symmetrical_pad = dest.height * symmetrical_pad_percent;
  const f32 vertical_symmetrical_pad = dest.height * symmetrical_pad_percent;
  const f32 additional_top_pad = dest.height * additional_top_pad_percent;

  const f32 total_top_pad = vertical_symmetrical_pad + additional_top_pad;
  const f32 total_bottom_pad = vertical_symmetrical_pad;
  const f32 total_left_pad = horizontal_symmetrical_pad;
  const f32 total_right_pad = horizontal_symmetrical_pad;

  dest.x += total_left_pad;
  dest.y += total_top_pad;
  dest.width -= (total_left_pad + total_right_pad);
  dest.height -= (total_top_pad + total_bottom_pad);

  switch (state->mainmenu_scene_character_subscene) {
    case MAIN_MENU_SCENE_CHARACTER_UPGRADE: {
      draw_main_menu_character_subscene_upgrade_upgrade_panel(dest, symmetrical_pad_percent);
      draw_main_menu_character_subscene_upgrade_item_list_panel(dest, symmetrical_pad_percent);
      break;
    }
    case MAIN_MENU_SCENE_CHARACTER_INVENTORY: {
      draw_main_menu_character_subscene_inventory_item_list_panel(dest, symmetrical_pad_percent);
      draw_main_menu_character_subscene_inventory_slots_panel(dest, symmetrical_pad_percent);
      break;
    }
    case MAIN_MENU_SCENE_CHARACTER_GAME_RULE: {
      draw_main_menu_character_subscene_game_rule_list_panel(dest, symmetrical_pad_percent);
      draw_main_menu_character_subscene_game_rule_details_panel(dest, symmetrical_pad_percent);
      break;
    }
    default: {
      state->mainmenu_scene_character_subscene = MAIN_MENU_SCENE_CHARACTER_GAME_RULE;
      break;
    }
  }
  Vector2 tab_btn_grid_loc = Vector2 { drawing_area.x + (drawing_area.width * .5f), drawing_area.y + (drawing_area.height * .025f) };

  #warning "TODO: Localize the display text"
  if(gui_menu_button("Upgrade", BTN_ID_MAINMENU_STATE_CHARACTER_ENTER_TAB_UPGRADE, VECTOR2(-35, 0), tab_btn_grid_loc, true)) {
    state->mainmenu_scene_character_subscene = MAIN_MENU_SCENE_CHARACTER_UPGRADE;
  }
  if(gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_STATE_CHARACTER_BUTTON_ENTER_TAB_INVENTORY), BTN_ID_MAINMENU_STATE_CHARACTER_ENTER_TAB_INVENTORY, VECTOR2(0.f, 0.f), tab_btn_grid_loc, true)) {
    state->mainmenu_scene_character_subscene = MAIN_MENU_SCENE_CHARACTER_INVENTORY;
  }
  if(gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_STATE_CHARACTER_BUTTON_ENTER_TAB_STATS),  BTN_ID_MAINMENU_STATE_CHARACTER_ENTER_TAB_STATS, VECTOR2(35, 0), tab_btn_grid_loc, true)) {
    state->mainmenu_scene_character_subscene = MAIN_MENU_SCENE_CHARACTER_GAME_RULE;
  }
}
void draw_main_menu_character_subscene_upgrade_upgrade_panel(Rectangle parent_dest, [[__maybe_unused__]] f32 padding) {
  mms_character_upgrade_context& scene_ctx = state->mms_character_upgrade;
  scene_ctx.upgrade_parent_panel.dest = Rectangle { parent_dest.x, parent_dest.y, (parent_dest.width * .75f), parent_dest.height};

  const panel_draw_result this_parent = gui_panel(scene_ctx.upgrade_parent_panel, scene_ctx.upgrade_parent_panel.dest, false);

  Rectangle left_item_panel_dest = {
    this_parent.draw_dest.x + (this_parent.draw_dest.width  * .35f), 
    this_parent.draw_dest.y + (this_parent.draw_dest.height * .65f),
    this_parent.draw_dest.height * .2f, 
    this_parent.draw_dest.height * .2f
  };
  const panel_draw_result left_panel_result = gui_panel(scene_ctx.left_item_panel, left_item_panel_dest, true);

  Rectangle right_item_panel_dest = {
    this_parent.draw_dest.x + (this_parent.draw_dest.width  * .65f), 
    this_parent.draw_dest.y + (this_parent.draw_dest.height * .65f),
    this_parent.draw_dest.height  * .2f, 
    this_parent.draw_dest.height * .2f
  };
  const panel_draw_result right_panel_result = gui_panel(scene_ctx.right_item_panel, right_item_panel_dest, true);

  Rectangle result_item_panel_dest = {
    this_parent.draw_dest.x + (this_parent.draw_dest.width  * .50f), 
    this_parent.draw_dest.y + (this_parent.draw_dest.height * .25f),
    this_parent.draw_dest.height  * .2f, 
    this_parent.draw_dest.height * .2f
  };
  const panel_draw_result result_panel_result = gui_panel(scene_ctx.result_item_panel, result_item_panel_dest, true);

  Vector2 left_item_line_start_1 = { 
    left_panel_result.bound_dest.x + left_panel_result.bound_dest.width, 
    left_panel_result.bound_dest.y + (left_panel_result.bound_dest.height * .5f)
  };
  Vector2 left_item_line_end_1 = { 
    result_panel_result.bound_dest.x + result_panel_result.bound_dest.width * .25f, 
    left_item_line_start_1.y
  };
  DrawLineEx(left_item_line_start_1, left_item_line_end_1, 1.f, WHITE);

  Vector2 left_item_line_start_2 = left_item_line_end_1;
  Vector2 left_item_line_end_2 = { 
    left_item_line_start_2.x, 
    result_panel_result.bound_dest.y + result_panel_result.bound_dest.height
  };
  DrawLineEx(left_item_line_start_2, left_item_line_end_2, 1.f, WHITE);

  Vector2 right_item_line_start_1 = { 
    right_panel_result.bound_dest.x, 
    right_panel_result.bound_dest.y + right_panel_result.bound_dest.height * .5f
  };
  Vector2 right_item_line_end_1 = { 
    result_panel_result.bound_dest.x + result_panel_result.bound_dest.width * .75f, 
    right_item_line_start_1.y
  };
  DrawLineEx(right_item_line_start_1, right_item_line_end_1, 1.f, WHITE);
  
  Vector2 right_item_line_start_2 = right_item_line_end_1;
  Vector2 right_item_line_end_2 = { 
    right_item_line_start_2.x, 
    result_panel_result.bound_dest.y + result_panel_result.bound_dest.height
  };
  DrawLineEx(right_item_line_start_2, right_item_line_end_2, 1.f, WHITE);

  if (scene_ctx.upgrade_state <= scene_ctx.CHARACTER_UPGRADE_STATE_UNDEFINED or scene_ctx.upgrade_state >= scene_ctx.CHARACTER_UPGRADE_STATE_MAX) {
    return;
  }
  if (scene_ctx.upgrade_state == scene_ctx.CHARACTER_UPGRADE_STATE_IDLE) {
    return;
  }
  if (scene_ctx.upgrade_state == scene_ctx.CHARACTER_UPGRADE_STATE_UPGRADE_SUCCESS) {
    Vector2 success_label_dest = {this_parent.draw_dest.x + this_parent.draw_dest.width * .5f, this_parent.draw_dest.y + this_parent.draw_dest.height * .8f};
    #warning "TODO: Localization"
    gui_label("SUCCESS!", FONT_TYPE_REGULAR, 1, success_label_dest, GREEN, true, true);
    gui_draw_atlas_texture_id(scene_ctx.product_item_tex, result_panel_result.draw_dest, ZEROVEC2, 0.f, WHITE);
    return;
  }
  if (scene_ctx.upgrade_state == scene_ctx.CHARACTER_UPGRADE_STATE_UPGRADE_FAILURE) {
    Vector2 failure_label_dest = {this_parent.draw_dest.x + this_parent.draw_dest.width * .5f, this_parent.draw_dest.y + this_parent.draw_dest.height * .8f};
    #warning "TODO: Localization"
    gui_label("FAILURE!", FONT_TYPE_REGULAR, 1, failure_label_dest, RED, true, true);
    return;
  }

  const std::vector<player_inventory_slot>& inventory = gm_get_player_state()->inventory;
  if (scene_ctx.slot_index >= inventory.size()) {
    return;
  }
  const player_inventory_slot& slot = inventory[scene_ctx.slot_index];
  if (slot.item_type <= ITEM_TYPE_UNDEFINED or slot.item_type >= ITEM_TYPE_MAX) {
    return;
  }

  const item_data& item = gm_get_default_items()[slot.item_type];
  f32 upgrade_chance = gm_get_ingame_chance(INGAME_CHANCE_UPGRADE, data128(static_cast<i32>(item.type)));
  if (upgrade_chance <= 0.f or upgrade_chance > 1.f) {
    scene_ctx.slot_index = I32_MAX;
    return;
  }
  if (scene_ctx.upgrade_state == scene_ctx.CHARACTER_UPGRADE_STATE_ERROR_UNSUFFICENT) {

    gui_draw_atlas_texture_id(item.tex_id, left_panel_result.draw_dest, ZEROVEC2, 0.f, WHITE);
    return;
  }

  if (scene_ctx.upgrade_state == scene_ctx.CHARACTER_UPGRADE_STATE_UPGRADE) {
    gui_draw_atlas_texture_id(item.tex_id, left_panel_result.draw_dest, ZEROVEC2, 0.f, WHITE);
    gui_draw_atlas_texture_id(item.tex_id, right_panel_result.draw_dest, ZEROVEC2, 0.f, WHITE);

    Vector2 upgrade_button_dest = {this_parent.draw_dest.x + this_parent.draw_dest.width * .5f, this_parent.draw_dest.y + this_parent.draw_dest.height * .8f};
    #warning "TODO: Localization"
    if(gui_menu_button("Upgrade",  BTN_ID_MAINMENU_STATE_CHARACTER_TAB_UPGRADE_UPGRADE, VECTOR2(0.f, 0.f), upgrade_button_dest, true)) {
      if (gm_upgrade_item_by_slot_ref(slot)) {
        gm_remove_from_inventory(slot.item_type, 2);
        gm_refresh_save_slot();
        smm_refresh_character_context();
        scene_ctx.upgrade_state = scene_ctx.CHARACTER_UPGRADE_STATE_UPGRADE_SUCCESS;
        scene_ctx.product_item_tex = gm_get_default_items()[slot.item_type].tex_id;
      }
      else {
        gm_remove_from_inventory(slot.item_type, 2);
        gm_refresh_save_slot();
        scene_ctx.upgrade_state = scene_ctx.CHARACTER_UPGRADE_STATE_UPGRADE_FAILURE;
      }
    }
  }
}

#warning "LABEL: draw upgrade"
void draw_main_menu_character_subscene_upgrade_item_list_panel(Rectangle parent_dest, f32 padding) {
  mms_character_upgrade_context& scene_ctx = state->mms_character_upgrade;
  const Rectangle& slots_panel_dest = scene_ctx.upgrade_parent_panel.dest;
  
  const f32 _padding = parent_dest.height * padding;
  scene_ctx.item_list_panel.dest = Rectangle{
    slots_panel_dest.x + slots_panel_dest.width + _padding,
    slots_panel_dest.y,
    parent_dest.width - slots_panel_dest.width - _padding,
    slots_panel_dest.height
  };
  Rectangle& this_dest = scene_ctx.item_list_panel.dest;
  gui_panel(scene_ctx.item_list_panel, scene_ctx.item_list_panel.dest, false);

  f32 height_buffer = 0.f;
  size_t index = 0u;
  for (const player_inventory_slot& slot : gm_get_player_state()->inventory) {
    panel * _local_panel = smm_get_local_panel(slot.ui_buffer.i32[0]);
    if (not _local_panel or _local_panel == nullptr) {
      state->mainmenu_scene_character_subscene = MAIN_MENU_SCENE_CHARACTER_GAME_RULE;
      begin_scene_change(MAIN_MENU_SCENE_DEFAULT);
    }
    const Rectangle local_panel_dest = Rectangle {
      this_dest.x + _padding,
      this_dest.y + _padding + height_buffer,
      this_dest.width - (_padding * 2.f),
      (this_dest.height * .2f) - (_padding * 2.f)
    };
    height_buffer += local_panel_dest.height + _padding;

    if (gui_panel_active(_local_panel, local_panel_dest, false)) {
      const item_data& item = gm_get_default_items()[slot.item_type];
      f32 upgrade_chance = gm_get_ingame_chance(INGAME_CHANCE_UPGRADE, data128(static_cast<i32>(item.type)));
      if (upgrade_chance <= 0.f or upgrade_chance > 1.f) {
        scene_ctx.slot_index = I32_MAX;
        return;
      }
      if (slot.amount >= 2) {
        scene_ctx.upgrade_state = mms_character_upgrade_context::CHARACTER_UPGRADE_STATE_UPGRADE;
      }
      else {
        scene_ctx.upgrade_state = mms_character_upgrade_context::CHARACTER_UPGRADE_STATE_ERROR_UNSUFFICENT;
      }
      scene_ctx.slot_index = index;
    }
    gui_label(slot.display_name.c_str(), FONT_TYPE_REGULAR, 1, Vector2 {local_panel_dest.x + local_panel_dest.width * .5f, local_panel_dest.y + local_panel_dest.height * .5f}, 
      WHITE, true, true
    );
    gui_label_box_format(FONT_TYPE_LIGHT, 1, local_panel_dest, WHITE, TEXT_ALIGN_BOTTOM_RIGHT, "%d", slot.amount);
    index++;
  }
}
void draw_main_menu_character_subscene_inventory_slots_panel(Rectangle parent_dest, [[__maybe_unused__]] f32 padding) {
  mms_character_inventory_context& scene_ctx = state->mms_character_inventory;
  scene_ctx.inventory_slots_panel.dest = Rectangle { parent_dest.x, parent_dest.y, (parent_dest.width * .75f), parent_dest.height};
  std::vector<Color> slot_palette = {
    Color{ 190u, 30u, 80u, 190u },  // Deep Ruby/Magenta
    Color{ 200u, 80u, 40u, 190u },  // Rich Amber/Orange
    Color{ 180u, 160u, 45u, 190u }, // Gold/Yellow
    Color{ 40u, 150u, 70u, 190u },  // Emerald Green
    Color{ 50u, 80u, 190u, 190u }   // Sapphire Blue
  };
  const Color slot_bg_color = Color{ 22, 18, 28, 168 };
  const panel_draw_result this_parent = gui_panel(scene_ctx.inventory_slots_panel, scene_ctx.inventory_slots_panel.dest, false);

  const f32 head_radius   = this_parent.draw_dest.height * .12f;
  const f32 arch_radius   = this_parent.draw_dest.height * .100f;
  const f32 common_radius = this_parent.draw_dest.height * .08f;

  const Vector2 head_slot_pos = Vector2{ this_parent.draw_dest.x + (this_parent.draw_dest.width * .5f), this_parent.draw_dest.y + (this_parent.draw_dest.height * .17f) };

  const f32 first_column_x  = this_parent.draw_dest.x + (this_parent.draw_dest.width  * .2f);
  const f32 second_column_x = this_parent.draw_dest.x + (this_parent.draw_dest.width  * .4f);
  const f32 third_column_x  = this_parent.draw_dest.x + (this_parent.draw_dest.width  * .6f);
  const f32 forth_column_x  = this_parent.draw_dest.x + (this_parent.draw_dest.width  * .8f);
  
  const f32 arch_slot_height   = this_parent.draw_dest.y + (this_parent.draw_dest.height * .38f); 
  const f32 common_set1_height = this_parent.draw_dest.y + (this_parent.draw_dest.height * .60f);
  const f32 common_set2_height = this_parent.draw_dest.y + (this_parent.draw_dest.height * .78f);

  auto draw_slot = [&slot_palette](Vector2 center, f32 inner_radius, f32 outer_radius, i32 sides, Color fill, bool animate) {
    if (sides < 3) { return; }
    DrawPoly(center, sides, inner_radius, 0.f, fill);
    draw_triangle_strip_star( center, inner_radius, outer_radius, sides, false, slot_palette, animate ? GetTime() : 0.f);
  };

  draw_slot( head_slot_pos, head_radius * 0.9f, head_radius, 14, slot_bg_color, false);
  {
    const i32 sides = 10;
    draw_slot(Vector2{ first_column_x,  arch_slot_height }, arch_radius * .9f, arch_radius, sides, slot_bg_color, false);
    draw_slot(Vector2{ second_column_x, arch_slot_height }, arch_radius * .9f, arch_radius, sides, slot_bg_color, false);
    draw_slot(Vector2{ third_column_x,  arch_slot_height }, arch_radius * .9f, arch_radius, sides, slot_bg_color, false);
    draw_slot(Vector2{ forth_column_x,  arch_slot_height }, arch_radius * .9f, arch_radius, sides, slot_bg_color, false);
  }
  {
    const i32 sides = 6;
    draw_slot(Vector2{ first_column_x,  common_set1_height }, common_radius * .9f, common_radius, sides, slot_bg_color, false);
    draw_slot(Vector2{ second_column_x, common_set1_height }, common_radius * .9f, common_radius, sides, slot_bg_color, false);
    draw_slot(Vector2{ third_column_x,  common_set1_height }, common_radius * .9f, common_radius, sides, slot_bg_color, false);
    draw_slot(Vector2{ forth_column_x,  common_set1_height }, common_radius * .9f, common_radius, sides, slot_bg_color, false);

    draw_slot(Vector2{ first_column_x,  common_set2_height }, common_radius * .9f, common_radius, sides, slot_bg_color, false);
    draw_slot(Vector2{ second_column_x, common_set2_height }, common_radius * .9f, common_radius, sides, slot_bg_color, false);
    draw_slot(Vector2{ third_column_x,  common_set2_height }, common_radius * .9f, common_radius, sides, slot_bg_color, false);
    draw_slot(Vector2{ forth_column_x,  common_set2_height }, common_radius * .9f, common_radius, sides, slot_bg_color, false);
  }
}
void draw_main_menu_character_subscene_inventory_item_list_panel(Rectangle parent_dest, f32 padding) {
  mms_character_inventory_context& scene_ctx = state->mms_character_inventory;
  const Rectangle& slots_panel_dest = scene_ctx.inventory_slots_panel.dest;
  
  const f32 _padding = parent_dest.height * padding;
  scene_ctx.inventory_item_list_panel.dest = Rectangle{
    slots_panel_dest.x + slots_panel_dest.width + _padding,
    slots_panel_dest.y,
    parent_dest.width - slots_panel_dest.width - _padding,
    slots_panel_dest.height
  };

  Rectangle& this_dest = scene_ctx.inventory_item_list_panel.dest;
  gui_panel(scene_ctx.inventory_item_list_panel, scene_ctx.inventory_item_list_panel.dest, false);

  f32 height_buffer = 0.f;
  for (const player_inventory_slot& slot : gm_get_player_state()->inventory) {
    panel * _local_panel = smm_get_local_panel(slot.ui_buffer.i32[0]);
    if (not _local_panel or _local_panel == nullptr) {
      throw std::runtime_error("panel is invalid");
    }
    const Rectangle local_panel_dest = Rectangle {
      this_dest.x + _padding,
      this_dest.y + _padding + height_buffer,
      this_dest.width - (_padding * 2.f),
      (this_dest.height * .2f) - (_padding * 2.f)
    };
    height_buffer += local_panel_dest.height + _padding;

    if (gui_panel_active(_local_panel, local_panel_dest, false)) {
      
    }
    gui_label(
      slot.display_name.c_str(), FONT_TYPE_REGULAR, 1, 
      Vector2 {local_panel_dest.x + local_panel_dest.width * .5f, local_panel_dest.y + local_panel_dest.height * .5f}, 
      WHITE, true, true
    );

    gui_label_box_format(FONT_TYPE_LIGHT, 1, local_panel_dest, WHITE, TEXT_ALIGN_BOTTOM_RIGHT, "%d", slot.amount);
  }
}
void draw_main_menu_character_subscene_game_rule_list_panel(Rectangle parent_dest, [[__maybe_unused__]] f32 padding) {
  const font_type panel_font_type = FONT_TYPE_REGULAR;
  const i32 panel_font_size = 1;
  state->mms_character_game_rule.rule_list_panel.dest = Rectangle { parent_dest.x, parent_dest.y, (parent_dest.width * .75f), parent_dest.height};

  gui_panel(state->mms_character_game_rule.rule_list_panel, state->mms_character_game_rule.rule_list_panel.dest, false);

  f32 showcase_hover_scale = 1.1f;
  f32 showcase_base_dim = SMM_BASE_RENDER_HEIGHT * .225f;
  const f32 showcase_dim_with_spacing = showcase_base_dim * 1.1f;
  const f32 total_showcases_width = (MAIN_MENU_UPGRADE_PANEL_COL * showcase_base_dim) + ((MAIN_MENU_UPGRADE_PANEL_COL - 1) * (showcase_dim_with_spacing - showcase_base_dim));
  const f32 total_showcases_height = (MAIN_MENU_UPGRADE_PANEL_ROW * showcase_base_dim) + ((MAIN_MENU_UPGRADE_PANEL_ROW - 1) * (showcase_dim_with_spacing - showcase_base_dim));
  const Vector2 showcase_start_pos = VECTOR2(
    state->mms_character_game_rule.rule_list_panel.dest.x + (state->mms_character_game_rule.rule_list_panel.dest.width / 2.f) - (total_showcases_width / 2.f),
    state->mms_character_game_rule.rule_list_panel.dest.y + (state->mms_character_game_rule.rule_list_panel.dest.height / 2.f) - (total_showcases_height / 2.f)
  );

  for (i32 iter = 0; iter < MAIN_MENU_UPGRADE_PANEL_ROW; ++iter) {
    for (i32 j = 0; j < MAIN_MENU_UPGRADE_PANEL_COL; ++j) {

      const game_rule * rule = __builtin_addressof(state->in_ingame_info->game_rules->at((MAIN_MENU_UPGRADE_PANEL_COL * iter) + j + 1));

      if (not rule or rule == nullptr or rule->id <= GAME_RULE_UNDEFINED or rule->id >= GAME_RULE_MAX) {
        continue;
      }

      [[__maybe_unused__]] bool hovered = false;
      const Vector2 text_measure = ui_measure_text(lc_txt(rule->display_name_loc_id), panel_font_type, panel_font_size);

      Vector2 showcase_position = VECTOR2(showcase_start_pos.x + j * showcase_dim_with_spacing, showcase_start_pos.y + iter * showcase_dim_with_spacing);
      f32 showcase_new_dim = showcase_base_dim;
      if (CheckCollisionPointRec( *state->mouse_pos_screen, Rectangle {showcase_position.x, showcase_position.y, showcase_base_dim, showcase_base_dim})) {
        hovered = true;
        showcase_new_dim *= showcase_hover_scale;
        showcase_position.x -= (showcase_new_dim - showcase_base_dim) / 2.f;
        showcase_position.y -= (showcase_new_dim - showcase_base_dim) / 2.f;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
          state->mms_character_game_rule.selected_rule = rule;
        }
      }
      
      // TITLE
      const Vector2 title_pos = VECTOR2(showcase_position.x + showcase_new_dim * .5f, showcase_position.y + showcase_new_dim * .125f);
      const Rectangle header_tex_pos = { title_pos.x, title_pos.y, showcase_new_dim, text_measure.y * 1.25f};
      // TITLE
      
      // STARS
      const Rectangle * tier_symbol_src_rect = get_atlas_texture_source_rect(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR);
      f32 star_spacing = tier_symbol_src_rect->width * 1.25f;
      f32 tier_symbols_total_width = tier_symbol_src_rect->width + (MAX_STAT_UPGRADE_TIER - 1.f) * star_spacing;
      f32 tier_symbols_left_edge = showcase_position.x + (showcase_new_dim - tier_symbols_total_width) / 2.f;
      f32 tier_symbols_vertical_center = showcase_position.y + showcase_new_dim * .8f;
      // STARS

      // ICON POS
      Rectangle icon_pos = {
        showcase_position.x + showcase_new_dim * .25f, showcase_position.y + showcase_new_dim * .25f,
        showcase_new_dim * .5f, showcase_new_dim * .5f
      };
      // ICON POS

      gui_draw_atlas_texture_id(ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG, Rectangle {showcase_position.x, showcase_position.y, showcase_new_dim, showcase_new_dim}, VECTOR2(0, 0), 0.f, WHITE);
      gui_draw_atlas_texture_id(ATLAS_TEX_ID_DARK_FANTASY_PANEL,    Rectangle {showcase_position.x, showcase_position.y, showcase_new_dim, showcase_new_dim}, VECTOR2(0, 0), 0.f, WHITE);

      for (i32 itr_000 = 0; itr_000 < MAX_STAT_UPGRADE_TIER; ++itr_000) {
        Vector2 tier_pos = {tier_symbols_left_edge + itr_000 * star_spacing, tier_symbols_vertical_center};
        if (itr_000 < rule->level - rule->base_level) {
          gui_draw_atlas_texture_id_scale(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR, tier_pos, 1.f, RED, false);
        } else {
          gui_draw_atlas_texture_id_scale(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR, tier_pos, 1.f, WHITE, false);
        }
      }
      gui_draw_texture_id_pro(TEX_ID_ASSET_ATLAS, rule->icon_src, icon_pos);
      draw_atlas_texture_stretch(ATLAS_TEX_ID_HEADER, Rectangle {64, 0, 32, 32}, header_tex_pos, true, WHITE);
      gui_label(lc_txt(rule->display_name_loc_id), panel_font_type, panel_font_size, title_pos, WHITE, true, true);
    }
  }
  atlas_texture_id icon_tex_id = ATLAS_TEX_ID_CURRENCY_COIN_ICON_5000;
  const char * total_stock_text = TextFormat("%d", get_currency_coins_total());
  const char * total_stock_desc_text = lc_txt(LOC_TEXT_MAINMENU_STATE_CHARACTER_TAB_STATS_CURRENCY_TEXT_TOTAL);
  Vector2 total_stock_desc_text_measure = ui_measure_text(total_stock_desc_text, panel_font_type, panel_font_size);
  const f32 total_stock_display_dest_dim = showcase_base_dim;
  const f32 total_stock_icon_dim = total_stock_display_dest_dim * .25f;

  Rectangle total_stock_dest = Rectangle {
    showcase_start_pos.x + total_showcases_width + (showcase_base_dim * .1f), 
    showcase_start_pos.y + total_showcases_height - showcase_base_dim,
    total_stock_display_dest_dim, 
    total_stock_display_dest_dim
  };
  Vector2 total_stock_desc_text_total_dest = Vector2 {
    total_stock_dest.x + (total_stock_dest.width * .5f) - total_stock_desc_text_measure.x,
    total_stock_dest.y + (total_stock_dest.height * .7f) - (total_stock_desc_text_measure.y * .5f)
  };
  Rectangle total_stock_icon_pos = Rectangle {
    total_stock_desc_text_total_dest.x + total_stock_desc_text_measure.x,
    total_stock_desc_text_total_dest.y + (total_stock_desc_text_measure.y * .5f),
    total_stock_icon_dim, total_stock_icon_dim
  };

  gui_draw_atlas_texture_id(icon_tex_id, total_stock_icon_pos, VECTOR2(0.f, total_stock_icon_pos.height * .5f), 0.f, WHITE);
  gui_label(total_stock_desc_text, panel_font_type, panel_font_size, total_stock_desc_text_total_dest, WHITE, false, false);
  gui_label_box(total_stock_text, panel_font_type, panel_font_size, total_stock_dest, WHITE, TEXT_ALIGN_BOTTOM_CENTER);
}
void draw_main_menu_character_subscene_game_rule_details_panel(Rectangle parent_dest, f32 padding) {
  const Rectangle& parent_panel_dest = parent_dest;
  const Rectangle& rule_list_panel_dest = state->mms_character_game_rule.rule_list_panel.dest;
  const panel& this_panel = state->mms_character_game_rule.rule_details_panel;
  
  const f32 _padding = parent_dest.height * padding;
  state->mms_character_game_rule.rule_details_panel.dest = Rectangle{
    rule_list_panel_dest.x + rule_list_panel_dest.width + _padding,
    rule_list_panel_dest.y,
    parent_panel_dest.width - rule_list_panel_dest.width - _padding,
    rule_list_panel_dest.height
  };

  gui_panel(this_panel, this_panel.dest, false);
  if (!state->mms_character_game_rule.selected_rule or state->mms_character_game_rule.selected_rule == nullptr) {
    return;
  }
  const game_rule *const rule = state->mms_character_game_rule.selected_rule;

  f32 detail_panel_element_spacing = this_panel.dest.height * 0.05f;

  Rectangle icon_pos = {
    this_panel.dest.x + this_panel.dest.width * .5f - (this_panel.dest.width * .175f),
    this_panel.dest.y + detail_panel_element_spacing,
    this_panel.dest.width * .35f,
    this_panel.dest.width * .35f
  };
  gui_draw_texture_id_pro(TEX_ID_ASSET_ATLAS, rule->icon_src, icon_pos);

  const Rectangle * tier_symbol_src_rect = get_atlas_texture_source_rect(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR);
  f32 star_spacing = tier_symbol_src_rect->width * 1.25f;
  f32 tier_symbols_total_width = tier_symbol_src_rect->width + (MAX_STAT_UPGRADE_TIER - 1.f) * star_spacing;
  f32 tier_symbols_left_edge = this_panel.dest.x + (this_panel.dest.width - tier_symbols_total_width) / 2.f;
  f32 tier_symbols_vertical_position = icon_pos.y + icon_pos.height + detail_panel_element_spacing * .5f;
  for (i32 itr_000 = 0; itr_000 < MAX_STAT_UPGRADE_TIER; ++itr_000) {
    Vector2 tier_pos = {tier_symbols_left_edge + itr_000 * star_spacing, tier_symbols_vertical_position};
    if (itr_000 < rule->level - rule->base_level) {
      gui_draw_atlas_texture_id_scale(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR, tier_pos, 1.f, RED, false);
    } else {
      gui_draw_atlas_texture_id_scale(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR, tier_pos, 1.f, WHITE, false);
    }
  }

  Vector2 title_pos = VECTOR2(
    this_panel.dest.x + this_panel.dest.width * .5f,
    tier_symbols_vertical_position + tier_symbol_src_rect->height + detail_panel_element_spacing * .5f
  );
  gui_label(lc_txt(rule->display_name_loc_id), FONT_TYPE_BOLD, 1, title_pos, WHITE, true, true);

  Rectangle description_pos = {
    this_panel.dest.x +this_panel.dest.width * .05f, title_pos.y + detail_panel_element_spacing * .75f,
    this_panel.dest.width * .9f, this_panel.dest.width * .35f
  };
  gui_label_wrap(lc_txt(rule->desc_loc_id), FONT_TYPE_LIGHT, 1, description_pos, WHITE, false);

  game_rule pseudo_update = *rule;
  gm_refresh_game_rule_by_level(&pseudo_update, pseudo_update.level+1);
  Vector2 upg_stat_text_pos = {
    this_panel.dest.x + this_panel.dest.width * .5f,
    this_panel.dest.y + this_panel.dest.height * .5f,
  };
  font_type details_font_type = FONT_TYPE_REGULAR;
  i32 details_font_size = 1;
  f32 before_float_value = rule->mm_ex.f32[1];
  i32 before_integer_value = rule->mm_ex.i32[1];
  f32 after_float_value = pseudo_update.mm_ex.f32[1];
  i32 after_integer_value = pseudo_update.mm_ex.i32[1];
  Color tint = WHITE;
  bool center_h = true;
  bool center_v = false;

  switch (rule->id) {
    case GAME_RULE_SPAWN_MULTIPLIER:
      gui_label_format_v(details_font_type, details_font_size, upg_stat_text_pos, tint, center_h, center_v, "%.1f -> %.1f", before_float_value, after_float_value);
      break;
    case GAME_RULE_PLAY_TIME_MULTIPLIER:
      gui_label_format_v(details_font_type, details_font_size, upg_stat_text_pos, tint, center_h, center_v, "%.1f -> %.1f", before_float_value, after_float_value);
      break;
    case GAME_RULE_DELTA_TIME_MULTIPLIER:
      gui_label_format_v(details_font_type, details_font_size, upg_stat_text_pos, tint, center_h, center_v, "%.1f -> %.1f", before_float_value, after_float_value);
      break;
    case GAME_RULE_BOSS_MODIFIER:
      gui_label_format_v(details_font_type, details_font_size, upg_stat_text_pos, tint, center_h, center_v, "%d -> %d", before_integer_value, after_integer_value);
      break;
    case GAME_RULE_AREA_UNLOCKER:
      gui_label_format_v(details_font_type, details_font_size, upg_stat_text_pos, tint, center_h, center_v, "%d -> %d", before_integer_value, after_integer_value);
      break;
    case GAME_RULE_TRAIT_POINT_MODIFIER:
      gui_label_format_v(details_font_type, details_font_size, upg_stat_text_pos, tint, center_h, center_v, "%d -> %d", before_integer_value, after_integer_value);
      break;
    case GAME_RULE_BONUS_RESULT_MULTIPLIER:
      gui_label_format_v(details_font_type, details_font_size, upg_stat_text_pos, tint, center_h, center_v, "%.1f -> %.1f", before_float_value, after_float_value);
      break;
    case GAME_RULE_ZOMBIE_LEVEL_MODIFIER:
      gui_label_format_v(details_font_type, details_font_size, upg_stat_text_pos, tint, center_h, center_v, "%d -> %d", before_integer_value, after_integer_value);
      break;
    case GAME_RULE_RESERVED_FOR_FUTURE_USE:
      gui_label_format_v(details_font_type, details_font_size, upg_stat_text_pos, tint, center_h, center_v, "%d -> %d", 0.f, 0.f);
      break;
    default: {
      IWARN("scene_main_menu::draw_main_menu_character_subscene_upgrade_details_panel()::Unsuppported stat id");
      return;
    }
  }
  atlas_texture_id icon_tex_id = ATLAS_TEX_ID_CURRENCY_COIN_ICON_5000;
  Rectangle cost_icon_dest = Rectangle(
    this_panel.dest.x + this_panel.dest.width * .425f,
    this_panel.dest.y + this_panel.dest.height * .85f,
    this_panel.dest.width * .15f,
    this_panel.dest.width * .15f
  );
  Vector2 cost_icon_origin = Vector2(
    cost_icon_dest.width / 2.f,
    cost_icon_dest.height / 2.f
  );
  gui_draw_atlas_texture_id(icon_tex_id, cost_icon_dest, cost_icon_origin, true, WHITE);

  if (state->deny_notify_timer > DENY_NOTIFY_TIME) {
    state->deny_notify_timer = DENY_NOTIFY_TIME;
  }
  if (state->deny_notify_timer > 0 && state->deny_notify_timer <= DENY_NOTIFY_TIME) {
    state->deny_notify_timer -= GetFrameTime();
  }
  Color cost_label_color = WHITE;
  cost_label_color.g = EaseSineIn(state->deny_notify_timer, 255, -255, DENY_NOTIFY_TIME);
  cost_label_color.b = EaseSineIn(state->deny_notify_timer, 255, -255, DENY_NOTIFY_TIME);
  Vector2 cost_label_pos = VECTOR2(
    this_panel.dest.x + this_panel.dest.width * .575f,
    this_panel.dest.y + this_panel.dest.height * .85f
  );
  gui_label_format_v(FONT_TYPE_REGULAR, 1, cost_label_pos, cost_label_color, true, true, "%d", rule->upgrade_cost);

  Vector2 button_location = Vector2 {
    this_panel.dest.x + this_panel.dest.width * .5f, 
    this_panel.dest.y  + this_panel.dest.height * .95f
  };
  if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_STATE_CHARACTER_TAB_STATS_BUTTON_UPGRADE), BTN_ID_MAINMENU_STATE_CHARACTER_BUY_STAT_UPGRADE, ZEROVEC2, button_location, false)) {
    const i32& cost = rule->upgrade_cost;
    const i32 stock = get_currency_coins_total();
    if ((rule->level - rule->base_level) < MAX_STAT_UPGRADE_TIER) {
      if ( stock - cost >= 0) {
        currency_coins_add(-rule->upgrade_cost);
        gm_set_game_rule_level_by_id(rule->id, rule->level+1);
        event_fire(EVENT_CODE_PLAY_SOUND_GROUP, event_context(SOUNDGROUP_ID_BUTTON_ON_CLICK, static_cast<i32>(true)));
        gm_save_game();
      } else {
        event_fire(EVENT_CODE_PLAY_SOUND, event_context(SOUND_ID_DENY1, static_cast<i32>(false)));
        state->deny_notify_timer = DENY_NOTIFY_TIME;
        gui_fire_display_error(LOC_TEXT_DISPLAY_ERROR_TEXT_INSUFFICIENT_FUNDS);
      }
    }
  }
}
void draw_trait_selection_panel(void) {
  Rectangle bg_trait_sel_pan_dest = gui_draw_default_background_panel(); 

  f32 trail_panel_height = bg_trait_sel_pan_dest.height * .4f;
  f32 trail_panel_width  = bg_trait_sel_pan_dest.width * .25f;
  f32 panel_height_gap = (bg_trait_sel_pan_dest.height - (trail_panel_height * 2.f)) * .3f;
  f32 panel_width_gap  = (bg_trait_sel_pan_dest.width  - (trail_panel_width  * 3.f)) * .25f;

  Rectangle positive_traits_sel_pan_dest = Rectangle {
    bg_trait_sel_pan_dest.x + panel_width_gap, 
    bg_trait_sel_pan_dest.y + panel_height_gap, 
    trail_panel_width, 
    bg_trait_sel_pan_dest.height * .4f
  };
  Rectangle negative_traits_sel_pan_dest = Rectangle {
    positive_traits_sel_pan_dest.x, 
    positive_traits_sel_pan_dest.y + positive_traits_sel_pan_dest.height + panel_height_gap, 
    trail_panel_width, 
    positive_traits_sel_pan_dest.height
  };
  Rectangle chosen_traits_sel_pan_dest = Rectangle {
    positive_traits_sel_pan_dest.x + positive_traits_sel_pan_dest.width + panel_width_gap, 
    positive_traits_sel_pan_dest.y,
    trail_panel_width,
    (positive_traits_sel_pan_dest.height * 2.f) + panel_height_gap
  };
  Rectangle ability_sel_pan_dest = Rectangle {
    chosen_traits_sel_pan_dest.x + chosen_traits_sel_pan_dest.width + panel_width_gap, 
    chosen_traits_sel_pan_dest.y,
    trail_panel_width,
    chosen_traits_sel_pan_dest.height
  };
  Vector2 available_traits_title_label_dest = Vector2 {
    positive_traits_sel_pan_dest.x + positive_traits_sel_pan_dest.width * .5f,
    bg_trait_sel_pan_dest.y + bg_trait_sel_pan_dest.height * .01f
  };
  Vector2 chosen_traits_title_label_dest = Vector2 {
    chosen_traits_sel_pan_dest.x + chosen_traits_sel_pan_dest.width * .5f,
    bg_trait_sel_pan_dest.y + bg_trait_sel_pan_dest.height * .01f
  };
  Vector2 chosen_trait_desc_title_label_dest = Vector2 {
    ability_sel_pan_dest.x + ability_sel_pan_dest.width * .5f,
    bg_trait_sel_pan_dest.y + bg_trait_sel_pan_dest.height * .01f
  };

  state->mms_trait_choice.positive_traits_selection_panel.dest = positive_traits_sel_pan_dest;
  gui_panel(state->mms_trait_choice.positive_traits_selection_panel, positive_traits_sel_pan_dest, false);
  state->mms_trait_choice.negative_traits_selection_panel.dest = negative_traits_sel_pan_dest;
  gui_panel(state->mms_trait_choice.negative_traits_selection_panel, negative_traits_sel_pan_dest, false);
  state->mms_trait_choice.chosen_traits_selection_panel.dest = chosen_traits_sel_pan_dest;
  gui_panel(state->mms_trait_choice.chosen_traits_selection_panel, chosen_traits_sel_pan_dest, false);
  state->mms_trait_choice.ability_selection_panel.dest = ability_sel_pan_dest;
  gui_panel(state->mms_trait_choice.ability_selection_panel, ability_sel_pan_dest, false);

  gui_label(lc_txt(LOC_TEXT_MAINMENU_TRAIT_CHOICE_AVAILABLE_TRAITS_TITLE), FONT_TYPE_REGULAR, 1, available_traits_title_label_dest, WHITE, true, false);
  gui_label(lc_txt(LOC_TEXT_MAINMENU_TRAIT_CHOICE_CHOSEN_TRAITS_TITLE),    FONT_TYPE_REGULAR, 1, chosen_traits_title_label_dest, WHITE, true, false);
  gui_label(lc_txt(LOC_TEXT_MAINMENU_TRAIT_ABILITY_CHOICE_PANEL_TITLE),    FONT_TYPE_REGULAR, 1, chosen_trait_desc_title_label_dest, WHITE, true, false);

  Rectangle positive_selection_panel_with_padding = Rectangle {
    positive_traits_sel_pan_dest.x      + (positive_traits_sel_pan_dest.width  * .025f),
    positive_traits_sel_pan_dest.y      + (positive_traits_sel_pan_dest.height * .025f),
    positive_traits_sel_pan_dest.width  - (positive_traits_sel_pan_dest.width  * .05f),
    positive_traits_sel_pan_dest.height - (positive_traits_sel_pan_dest.height * .05f)
  };
  trait_selection_panel_list_traits(
    __builtin_addressof(state->mms_trait_choice.positive_traits_selection_panel), positive_selection_panel_with_padding, state->mms_trait_choice.positive_traits, positive_trait_button_on_click
  );

  Rectangle negative_selection_panel_with_padding = Rectangle {
    negative_traits_sel_pan_dest.x      + (negative_traits_sel_pan_dest.width  * .025f),
    negative_traits_sel_pan_dest.y      + (negative_traits_sel_pan_dest.height * .025f),
    negative_traits_sel_pan_dest.width  - (negative_traits_sel_pan_dest.width  * .05f),
    negative_traits_sel_pan_dest.height - (negative_traits_sel_pan_dest.height * .05f)
  };
  trait_selection_panel_list_traits(
    __builtin_addressof(state->mms_trait_choice.negative_traits_selection_panel), negative_selection_panel_with_padding, state->mms_trait_choice.negative_traits, negative_trait_button_on_click
  );

  Rectangle chosen_panel_with_padding = Rectangle {
    chosen_traits_sel_pan_dest.x      + (chosen_traits_sel_pan_dest.width  * .025f),
    chosen_traits_sel_pan_dest.y      + (chosen_traits_sel_pan_dest.height * .025f),
    chosen_traits_sel_pan_dest.width  - (chosen_traits_sel_pan_dest.width  * .05f),
    chosen_traits_sel_pan_dest.height - (chosen_traits_sel_pan_dest.height * .05f)
  };
  trait_selection_panel_list_traits(
    __builtin_addressof(state->mms_trait_choice.chosen_traits_selection_panel), chosen_panel_with_padding, state->mms_trait_choice.chosen_traits, chosen_trait_button_on_click
  );

  Rectangle ability_selection_panel_padding = Rectangle {
    ability_sel_pan_dest.x      + (ability_sel_pan_dest.width  * .025f),
    ability_sel_pan_dest.y      + (ability_sel_pan_dest.height * .025f),
    ability_sel_pan_dest.width  - (ability_sel_pan_dest.width  * .05f),
    ability_sel_pan_dest.height - (ability_sel_pan_dest.height * .05f)
  };
  trait_selection_panel_list_ability_selection_panel(__builtin_addressof( state->mms_trait_choice.ability_selection_panel), ability_selection_panel_padding, 
    VECTOR2(ability_sel_pan_dest.width  * .025f, ability_sel_pan_dest.height * .025f)
  );

  const char * lc_txt_remaining_trt_pnts = lc_txt(LOC_TEXT_MAINMENU_TRAIT_CHOICE_PANEL_REMANING_TRAIT_POINTS);
  Vector2 txt_measure_remaining_trt_pnts = ui_measure_text(lc_txt_remaining_trt_pnts, FONT_TYPE_REGULAR, 1);
  f32 remaining_traits_dest_width = state->in_app_settings->render_width * .2f;
  Rectangle remaining_traits_dest = Rectangle {
    bg_trait_sel_pan_dest.x + (bg_trait_sel_pan_dest.width * .5f) - (remaining_traits_dest_width * .5f),
    bg_trait_sel_pan_dest.y + bg_trait_sel_pan_dest.height * .99f - txt_measure_remaining_trt_pnts.y,
    remaining_traits_dest_width,
    txt_measure_remaining_trt_pnts.y
  };
  gui_label_box_format(FONT_TYPE_REGULAR, 1, remaining_traits_dest, WHITE, TEXT_ALIGN_TOP_LEFT, "%s:", lc_txt_remaining_trt_pnts);
  gui_label_box_format(FONT_TYPE_REGULAR, 1, remaining_traits_dest, WHITE, TEXT_ALIGN_TOP_RIGHT, "%d", state->mms_trait_choice.available_trait_points);
}
void trait_selection_panel_list_traits(panel* const pnl, const Rectangle rect, std::vector<character_trait>& traits, void (*trait_button_on_click_pfn)(i32 index)) {
  BeginScissorMode(rect.x, rect.y, rect.width, rect.height);
  {
    const f32 scroll_handle_texture_w_ratio = 16.f / 16.f;
    const f32 scroll_handle_width = SMM_SCROLL_HANDLE_HEIGHT * scroll_handle_texture_w_ratio;
    const f32 scroll_handle_background_h_ratio = 16.f / 16.f;
    const f32 scroll_background_height = SMM_SCROLL_HANDLE_HEIGHT * scroll_handle_background_h_ratio;
    const f32 scroll_background_texture_w_ratio = 16.f / 16.f;
    const f32 scroll_background_width = scroll_background_height * scroll_background_texture_w_ratio;

    const f32 panel_scroll_bg_x = rect.x + rect.width - scroll_background_width;

    const Rectangle panel_scroll_bg_head_src  = Rectangle { 0.f, 0.f, 16.f, 11.f };
    const f32 panel_scroll_bg_head_src_scale = panel_scroll_bg_head_src.height / panel_scroll_bg_head_src.width;
    const Rectangle panel_scroll_bg_head_dest = Rectangle { 
      panel_scroll_bg_x, rect.y, 
      scroll_background_width, scroll_background_width * panel_scroll_bg_head_src_scale
    };

    const Rectangle panel_scroll_bg_bottom_src  = Rectangle { 0.f, 36.f, 16.f, 11.f };
    const f32 panel_scroll_bg_bottom_src_scale = panel_scroll_bg_bottom_src.height / panel_scroll_bg_bottom_src.width;
    const Rectangle panel_scroll_bg_bottom_dest = Rectangle {
      panel_scroll_bg_x, rect.y + rect.height - (scroll_background_width * panel_scroll_bg_bottom_src_scale), 
      scroll_background_width, (scroll_background_width * panel_scroll_bg_bottom_src_scale)
    };

    const Rectangle panel_scroll_bg_body_src  = Rectangle { 0.f, 16.f, 16.f, 16.f };
    const Rectangle panel_scroll_bg_body_dest = Rectangle {
      panel_scroll_bg_x, panel_scroll_bg_head_dest.y + panel_scroll_bg_head_dest.height, 
      scroll_background_width, panel_scroll_bg_bottom_dest.y - panel_scroll_bg_head_dest.y - panel_scroll_bg_head_dest.height
    };

    f32 trait_list_height_buffer = 0.f;
    Vector2 text_measure = ZEROVEC2;
    i32 index = 0;
    for (character_trait& trait : traits) {
      local_button* _lc_btn = smm_get_local_button(trait.ui_ops.i32[0]);
      text_measure = ui_measure_text(lc_txt(trait.loc_title), FONT_TYPE_REGULAR, 1);
      Vector2 _lc_btn_pos = VECTOR2(rect.x, rect.y + trait_list_height_buffer + (pnl->buffer.f32[0] * pnl->scroll));
      Rectangle _lc_btn_dest = Rectangle {
        _lc_btn_pos.x, _lc_btn_pos.y, _lc_btn->btn_type.dest_frame_dim.x, _lc_btn->btn_type.dest_frame_dim.y
      };

      if (gui_draw_local_button(lc_txt(trait.loc_title), _lc_btn, FONT_TYPE_REGULAR, 1, _lc_btn_pos, TEXT_ALIGN_LEFT_CENTER, false)) {
        if (trait_list_height_buffer + (pnl->buffer.f32[0] * pnl->scroll) < rect.height) {
          _lc_btn->current_state = BTN_STATE_UP;
          break;
        }
      }
      switch (_lc_btn->current_state) {
        case BTN_STATE_UP:{
          gui_label_box_format(FONT_TYPE_REGULAR, 1, _lc_btn_dest, _lc_btn->btn_type.forground_color_btn_state_up, TEXT_ALIGN_RIGHT_CENTER, "%d", trait.point);
          break;
        }
        case BTN_STATE_HOVER:{
          gui_label_box_format(FONT_TYPE_REGULAR, 1, _lc_btn_dest, _lc_btn->btn_type.forground_color_btn_state_hover, TEXT_ALIGN_RIGHT_CENTER, "%d", trait.point);
          break;
        }
        case BTN_STATE_PRESSED:{
          gui_label_box_format(FONT_TYPE_REGULAR, 1, _lc_btn_dest, _lc_btn->btn_type.forground_color_btn_state_pressed, TEXT_ALIGN_RIGHT_CENTER, "%d", trait.point);
          break;
        }
        default: break; 
      }
      trait_list_height_buffer += text_measure.y + (text_measure.y * .1f);
      index++;
    }
    trait_button_on_click_pfn(index);
    
    pnl->buffer.f32[0] = trait_list_height_buffer - rect.height - (text_measure.y * .2f);
    pnl->scroll_handle.x = rect.x + rect.width - scroll_handle_width;
    pnl->scroll_handle.width = scroll_handle_width;
    pnl->scroll_handle.height = SMM_SCROLL_HANDLE_HEIGHT;
    pnl->scroll_handle.y = std::max(pnl->scroll_handle.y, rect.y + panel_scroll_bg_head_dest.height);
    pnl->scroll_handle.y = std::min(pnl->scroll_handle.y, rect.y + rect.height - panel_scroll_bg_bottom_dest.height - pnl->scroll_handle.height);
    pnl->scroll = 
      (pnl->scroll_handle.y - rect.y - panel_scroll_bg_head_dest.height) / 
      (rect.height - pnl->scroll_handle.height - panel_scroll_bg_bottom_dest.height - panel_scroll_bg_head_dest.height) * -1;

    if (trait_list_height_buffer > rect.height) {
      gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_PANEL_SCROLL, panel_scroll_bg_head_src, panel_scroll_bg_head_dest, false);
      gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_PANEL_SCROLL, panel_scroll_bg_body_src, panel_scroll_bg_body_dest, false, TEXTURE_WRAP_CLAMP);
      gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_PANEL_SCROLL, panel_scroll_bg_bottom_src, panel_scroll_bg_bottom_dest, false);
      gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_PANEL_SCROLL_HANDLE, Rectangle {0.f, 0.f, 16.f, 16.f}, pnl->scroll_handle, false);
      pnl->is_scrolling_active = true;
    }
    else {
      pnl->scroll = 0.f;
      pnl->is_scrolling_active = false;
    }
  }
  EndScissorMode();
}
void trait_selection_panel_list_ability_selection_panel(panel* const pnl, const Rectangle rect, Vector2 border_gap) {
  if (not pnl or pnl == nullptr) {
    IWARN("scene_main_menu::trait_selection_panel_list_ability_selection_panel()::Panel is invalid");
    return;
  }
  const f32 scroll_backgroud_src_width = 16.f;
  const f32 scroll_handle_src_width = 16.f;
  const f32 scroll_handle_texture_w_ratio = scroll_handle_src_width / 16.f;
  const f32 scroll_handle_width = SMM_SCROLL_HANDLE_HEIGHT * scroll_handle_texture_w_ratio;
  const f32 scroll_handle_background_h_ratio = scroll_handle_src_width / 16.f;
  const f32 scroll_background_height = SMM_SCROLL_HANDLE_HEIGHT * scroll_handle_background_h_ratio;
  const f32 scroll_background_texture_w_ratio = scroll_backgroud_src_width / 16.f;
  const f32 scroll_background_width = scroll_background_height * scroll_background_texture_w_ratio;
  const f32 scroll_background_sample_width = ((scroll_background_width / scroll_backgroud_src_width));

  const f32 panel_scroll_bg_x = rect.x + rect.width - (scroll_background_width * .5f) + scroll_background_sample_width;
  const Rectangle panel_scroll_bg_head_src  = Rectangle { 0.f, 0.f, 16.f, 11.f };
  const f32 panel_scroll_bg_head_src_scale = panel_scroll_bg_head_src.height / panel_scroll_bg_head_src.width;
  const Rectangle panel_scroll_bg_head_dest = Rectangle { 
    panel_scroll_bg_x, rect.y, 
    scroll_background_width, scroll_background_width * panel_scroll_bg_head_src_scale
  };

  const Rectangle panel_scroll_bg_bottom_src  = Rectangle { 0.f, 36.f, 16.f, 11.f };
  const f32 panel_scroll_bg_bottom_src_scale = panel_scroll_bg_bottom_src.height / panel_scroll_bg_bottom_src.width;
  const Rectangle panel_scroll_bg_bottom_dest = Rectangle {
    panel_scroll_bg_x, rect.y + rect.height - (scroll_background_width * panel_scroll_bg_bottom_src_scale), 
    scroll_background_width, (scroll_background_width * panel_scroll_bg_bottom_src_scale)
  };
  const Rectangle panel_scroll_bg_body_src  = Rectangle { 0.f, 16.f, 16.f, 16.f };
  const Rectangle panel_scroll_bg_body_dest = Rectangle {
    panel_scroll_bg_x, panel_scroll_bg_head_dest.y + panel_scroll_bg_head_dest.height, 
    scroll_background_width, panel_scroll_bg_bottom_dest.y - panel_scroll_bg_head_dest.y - panel_scroll_bg_head_dest.height
  };
    
  f32 ability_list_height_buffer = 0.f;
  Vector2 text_measure = ZEROVEC2;
  const f32 _lc_pnl_height = rect.height * .15f;
  const Vector2 _lc_pnl_dim = VECTOR2(rect.width * .95f, _lc_pnl_height);
  BeginScissorMode(rect.x, rect.y, rect.width, rect.height);
  {
    for (size_t itr_000 = 0; itr_000 < ABILITY_ID_MAX; ++itr_000) {
      const ability& _abl = state->mms_trait_choice.abilities.at(itr_000);
      if (_abl.id <= ABILITY_ID_UNDEFINED or _abl.id >= ABILITY_ID_MAX) {
        continue;
      }
      panel * _lc_pnl = smm_get_local_panel(_abl.ui_use.i32[0]);
      text_measure = ui_measure_text(lc_txt(_abl.display_name_loc_text_id), FONT_TYPE_REGULAR, 1);
      Vector2 _lc_pnl_pos = VECTOR2(
        rect.x + (rect.width * .5f) - (_lc_pnl_dim.x * .5f), 
        rect.y + ability_list_height_buffer + (pnl->buffer.f32[0] * pnl->scroll)
      );
      Rectangle _lc_pnl_dest = Rectangle {_lc_pnl_pos.x, _lc_pnl_pos.y, _lc_pnl_dim.x, _lc_pnl_dim.y};
      if ( (*state->in_ingame_info->starter_ability) == _abl.id) {
        _lc_pnl->bg_tex_id = ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG;
      }
      if (gui_panel_active(_lc_pnl, _lc_pnl_dest, false)) {
        if (ability_list_height_buffer + (pnl->buffer.f32[0] * pnl->scroll) < rect.height && (*state->in_ingame_info->starter_ability) != _abl.id) {
          const ability& _prev_starter_abl = state->mms_trait_choice.abilities.at( static_cast<size_t>( (*state->in_ingame_info->starter_ability)) );
          panel * const _prev_starter_abls_lc_panel = smm_get_local_panel(_prev_starter_abl.ui_use.i32[0]);

          state->mms_trait_choice.starter_ability = _abl.id;

          if (not _prev_starter_abls_lc_panel or _prev_starter_abls_lc_panel == nullptr) {
            continue;
          }
          const i32 _backup_id = _prev_starter_abls_lc_panel->id;
          (*_prev_starter_abls_lc_panel) = state->smm_default_panels.panel_active_dark_fantasy_default;
          _prev_starter_abls_lc_panel->id = _backup_id;
        }
      }
      const f32 ability_icon_width = _lc_pnl_dest.height;
      const f32 ability_icon_height = _lc_pnl_dest.height;
      const Rectangle ability_icon_dest = Rectangle { _lc_pnl_dest.x,  _lc_pnl_dest.y, ability_icon_width,  ability_icon_height};
      gui_draw_texture_id_pro(TEX_ID_ASSET_ATLAS, _abl.icon_src, ability_icon_dest);

      // � Return here
      gui_label(lc_txt(_abl.display_name_loc_text_id), FONT_TYPE_REGULAR, 1, 
        VECTOR2(_lc_pnl_dest.x + ((_lc_pnl_dest.width + ability_icon_width) * .5f), _lc_pnl_dest.y + _lc_pnl_dest.height * .5f), 
        WHITE, true, true
      );

      switch (_lc_pnl->current_state) {
        case BTN_STATE_UP:{
          
          break;
        }
        case BTN_STATE_HOVER:{
          
          break;
        }
        case BTN_STATE_PRESSED:{
          
          break;
        }
        default: break; 
      }
      ability_list_height_buffer += _lc_pnl_dest.height + border_gap.y;
    }
  }
  EndScissorMode();

  pnl->buffer.f32[0] = ability_list_height_buffer - rect.height - (text_measure.y * .2f);
  pnl->scroll_handle.x = panel_scroll_bg_x;
  pnl->scroll_handle.width = scroll_handle_width;
  pnl->scroll_handle.height = SMM_SCROLL_HANDLE_HEIGHT;
  pnl->scroll_handle.y = std::max(pnl->scroll_handle.y, rect.y + panel_scroll_bg_head_dest.height);
  pnl->scroll_handle.y = std::min(pnl->scroll_handle.y, rect.y + rect.height - panel_scroll_bg_bottom_dest.height - pnl->scroll_handle.height);
  pnl->scroll = 
    (pnl->scroll_handle.y - rect.y - panel_scroll_bg_head_dest.height) / 
    (rect.height - pnl->scroll_handle.height - panel_scroll_bg_bottom_dest.height - panel_scroll_bg_head_dest.height) * -1;
  if (ability_list_height_buffer > rect.height) {
    gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_PANEL_SCROLL, panel_scroll_bg_head_src, panel_scroll_bg_head_dest, false);
    gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_PANEL_SCROLL, panel_scroll_bg_body_src, panel_scroll_bg_body_dest, false, TEXTURE_WRAP_CLAMP);
    gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_PANEL_SCROLL, panel_scroll_bg_bottom_src, panel_scroll_bg_bottom_dest, false);
    gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_PANEL_SCROLL_HANDLE, Rectangle {0.f, 0.f, 16.f, 16.f}, pnl->scroll_handle, false);
    pnl->is_scrolling_active = true;
  }
  else {
    pnl->scroll = 0.f;
    pnl->is_scrolling_active = false;
  }
}

void smm_update_mouse_bindings(void) {
  switch (state->mainmenu_state) {
    case MAIN_MENU_SCENE_DEFAULT: {
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE: {
      state->ingame_scene_feed.hovered_stage = U16_MAX;
      Vector2 hovered_stage = ZEROVEC2;
      for (i32 itr_000 = 0; itr_000 < MAX_WORLDMAP_LOCATIONS; ++itr_000) {
        const f32 image_ratio = 3840.f / 2160.f; // INFO: ratio of map texture
        const f32 image_height = static_cast<f32>(SMM_BASE_RENDER_HEIGHT * .85f);
        const Vector2 image_dim = Vector2 { image_height * image_ratio, image_height};
        Rectangle map_choice_image_dest = Rectangle { SMM_BASE_RENDER_DIV2.x, 0.f, image_dim.x, image_dim.y };
        map_choice_image_dest.x -= image_dim.x * .5f;
        const f32 map_pin_dim = map_choice_image_dest.height * .05f;
        const Vector2& normalized_pin_location = state->worldmap_locations.at(itr_000).screen_location;
        Rectangle scrloc = Rectangle{
          map_choice_image_dest.x + normalized_pin_location.x * map_choice_image_dest.width - (map_pin_dim * .5f), 
          map_choice_image_dest.y + normalized_pin_location.y * map_choice_image_dest.height - (map_pin_dim * .5f),
          map_pin_dim, map_pin_dim,
        };
        if (CheckCollisionPointRec( *state->mouse_pos_screen, scrloc)) {
          state->ingame_scene_feed.hovered_stage = itr_000;
          hovered_stage = Vector2 {scrloc.x + map_pin_dim * .5f, scrloc.y + map_pin_dim * .5f};
        }
      }
      
      if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->ingame_scene_feed.hovered_stage <= MAX_WORLDMAP_LOCATIONS) {
        if (state->ingame_scene_feed.hovered_stage <= 0 or static_cast<size_t>(state->ingame_scene_feed.hovered_stage) >= state->worldmap_locations.size()) {
        }
        else if (not state->worldmap_locations.at(state->ingame_scene_feed.hovered_stage).is_playable) {
          event_fire(EVENT_CODE_PLAY_SOUND, event_context(SOUND_ID_DENY1, static_cast<i32>(false)));
          gui_fire_display_error(LOC_TEXT_DISPLAY_ERROR_TEXT_STAGE_IS_NOT_PLAYABLE);
        }
        else {
          set_worldmap_location(state->ingame_scene_feed.hovered_stage);
          if(not gm_init_game(state->worldmap_locations[state->ingame_scene_feed.hovered_stage], state->mms_trait_choice.chosen_traits, state->mms_trait_choice.starter_ability)) {
            IWARN("scene_in_game::smm_update_mouse_bindings()::Failed to initialize game");
            begin_scene_change(SCENE_TYPE_MAIN_MENU, data128(static_cast<i32>(true)));
          }
          smm_begin_fadeout(data128(SCENE_TYPE_IN_GAME, true), fade_on_complete_change_scene);
          event_fire(EVENT_CODE_CAMERA_SET_ZOOM_TARGET, event_context(static_cast<f32>(ZOOM_CAMERA_MAP_ENTRY), static_cast<f32>(true), hovered_stage.x, hovered_stage.y));
          state->is_changing_state = true;
        }
      }
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE: {
      if (CheckCollisionPointRec( (*state->mouse_pos_screen), state->mms_trait_choice.positive_traits_selection_panel.dest)) {
        panel* pnl = &state->mms_trait_choice.positive_traits_selection_panel;

        if (pnl->is_scrolling_active) {
          pnl->scroll_handle.y += GetMouseWheelMove() * -10.f;
        }
        if (pnl->is_scrolling_active && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
          if (!state->is_any_button_down && !state->is_dragging_scroll && CheckCollisionPointRec( (*state->mouse_pos_screen), pnl->scroll_handle)) {
            pnl->is_dragging_scroll = true;
            state->is_dragging_scroll = true;
          }
        }
      }
      if (CheckCollisionPointRec( (*state->mouse_pos_screen), state->mms_trait_choice.negative_traits_selection_panel.dest)) {
        panel* pnl = &state->mms_trait_choice.negative_traits_selection_panel;

        if (pnl->is_scrolling_active) {
          pnl->scroll_handle.y += GetMouseWheelMove() * -10.f;
        }
        if (pnl->is_scrolling_active && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
          if (!state->is_any_button_down && !state->is_dragging_scroll && CheckCollisionPointRec( (*state->mouse_pos_screen), pnl->scroll_handle)) {
            pnl->is_dragging_scroll = true;
            state->is_dragging_scroll = true;
          }
        }
      }
      if (CheckCollisionPointRec( (*state->mouse_pos_screen), state->mms_trait_choice.chosen_traits_selection_panel.dest)) {
        panel* pnl = &state->mms_trait_choice.chosen_traits_selection_panel;

        if (pnl->is_scrolling_active) {
          pnl->scroll_handle.y += GetMouseWheelMove() * -10.f;
        }
        if (pnl->is_scrolling_active && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
          if (!state->is_any_button_down && !state->is_dragging_scroll && CheckCollisionPointRec( (*state->mouse_pos_screen), pnl->scroll_handle)) {
            pnl->is_dragging_scroll = true;
            state->is_dragging_scroll = true;
          }
        }
      }
      if (CheckCollisionPointRec( (*state->mouse_pos_screen), state->mms_trait_choice.ability_selection_panel.dest)) {
        panel* pnl = &state->mms_trait_choice.ability_selection_panel;

        if (pnl->is_scrolling_active) {
          pnl->scroll_handle.y += GetMouseWheelMove() * -10.f;
        }
        if (pnl->is_scrolling_active && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
          if (!state->is_any_button_down && !state->is_dragging_scroll && CheckCollisionPointRec( (*state->mouse_pos_screen), pnl->scroll_handle)) {
            pnl->is_dragging_scroll = true;
            state->is_dragging_scroll = true;
          }
        }
      }

      if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) 
      {
        if (state->mms_trait_choice.positive_traits_selection_panel.is_dragging_scroll) {
          panel* pnl = __builtin_addressof(state->mms_trait_choice.positive_traits_selection_panel);
          pnl->scroll_handle.y = state->mouse_pos_screen->y - pnl->scroll_handle.height * .5f;
        }
        if (state->mms_trait_choice.negative_traits_selection_panel.is_dragging_scroll) {
          panel* pnl = __builtin_addressof(state->mms_trait_choice.negative_traits_selection_panel);
          pnl->scroll_handle.y = state->mouse_pos_screen->y - pnl->scroll_handle.height * .5f;
        }
        if (state->mms_trait_choice.chosen_traits_selection_panel.is_dragging_scroll) {
          panel* pnl = __builtin_addressof(state->mms_trait_choice.chosen_traits_selection_panel);
          pnl->scroll_handle.y = state->mouse_pos_screen->y - pnl->scroll_handle.height * .5f;
        }
        if (state->mms_trait_choice.ability_selection_panel.is_dragging_scroll) {
          panel* pnl = __builtin_addressof(state->mms_trait_choice.ability_selection_panel);
          pnl->scroll_handle.y = state->mouse_pos_screen->y - pnl->scroll_handle.height * .5f;
        }
      }
      if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) 
      {
        if (state->mms_trait_choice.positive_traits_selection_panel.is_dragging_scroll) {
          state->mms_trait_choice.positive_traits_selection_panel.is_dragging_scroll = false;
          state->is_dragging_scroll = false;
        }
        if (state->mms_trait_choice.negative_traits_selection_panel.is_dragging_scroll) {
          state->mms_trait_choice.negative_traits_selection_panel.is_dragging_scroll = false;
          state->is_dragging_scroll = false;
        }
        if (state->mms_trait_choice.chosen_traits_selection_panel.is_dragging_scroll) {
          state->mms_trait_choice.chosen_traits_selection_panel.is_dragging_scroll = false;
          state->is_dragging_scroll = false;
        }
        if (state->mms_trait_choice.ability_selection_panel.is_dragging_scroll) {
          state->mms_trait_choice.ability_selection_panel.is_dragging_scroll = false;
          state->is_dragging_scroll = false;
        }
      }
      break;
    }
    case MAIN_MENU_SCENE_SETTINGS: {
      break;
    }
    case MAIN_MENU_SCENE_EXTRAS: {
      break;
    }
    case MAIN_MENU_SCENE_CHARACTER: {
      break;
    }
    default: {
      IWARN("scene_in_game::smm_update_mouse_bindings()::Unsupported stage");
      break;
    }
  }
}
void smm_update_keyboard_bindings(void) {
  switch (state->mainmenu_state) {
    case MAIN_MENU_SCENE_DEFAULT: {
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE: {
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE: {
      break;
    }
    case MAIN_MENU_SCENE_SETTINGS: {
      break;
    }
    case MAIN_MENU_SCENE_EXTRAS: {
      break;
    }
    case MAIN_MENU_SCENE_CHARACTER: {
      break;
    }
    default: {
      IWARN("scene_in_game::smm_update_keyboard_bindings()::Unsupported stage");
      break;
    }
  }
}
void smm_update_bindings(void) {
  if (not state or state == nullptr) {
    return;
  }
  if (state->is_changing_state) {
    return;
  }
  smm_update_mouse_bindings();
  smm_update_keyboard_bindings();
}
void smm_update_local_buttons(void) {
  if (state->is_changing_state) {
    return;
  }
  for (size_t itr_000 = 0; itr_000 < state->general_purpose_buttons.size(); ++itr_000) {
    local_button * const _btn = __builtin_addressof(state->general_purpose_buttons.at(itr_000));
    if (!_btn->on_screen) continue;

    if (CheckCollisionPointRec(*state->mouse_pos_screen, _btn->dest)) {
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        _btn->prev_state = _btn->current_state;
        _btn->current_state = BTN_STATE_PRESSED;
      } else {
        if (_btn->current_state == BTN_STATE_PRESSED) { 
          _btn->prev_state = _btn->current_state;
          _btn->current_state = BTN_STATE_RELEASED;
        }
        else if (_btn->current_state != BTN_STATE_HOVER) {
          _btn->prev_state = _btn->current_state;
          _btn->current_state = BTN_STATE_HOVER;
        }
      }
    } else {
      _btn->prev_state = _btn->current_state;
      _btn->current_state = BTN_STATE_UP;
    }
    _btn->on_screen = false;
  }
}

void smm_begin_fadeout(data128 data, void(*on_change_complete)(data128)) {
  if (not state or state == nullptr ) {
    IERROR("scene_main_menu::smm_begin_fadeout()::State is not valid");
    return;
  }
  state->smm_fade = ui_fade_control_system();

  state->smm_fade.fade_animation_duration = MAIN_MENU_FADE_DURATION;
  state->smm_fade.fade_type = FADE_TYPE_FADEOUT;
  state->smm_fade.fade_animation_accumulator = 0.f;
  state->smm_fade.fade_animation_playing = true;
  state->smm_fade.is_fade_animation_played = false;
  state->smm_fade.data = data;
  state->smm_fade.on_change_complete = on_change_complete;
}
void smm_begin_fadein(data128 data, void(*on_change_complete)(data128)) {
  if (not state or state == nullptr ) {
    IERROR("scene_main_menu::smm_begin_fadein()::State is not valid");
    return;
  }
  state->smm_fade.fade_animation_duration = MAIN_MENU_FADE_DURATION;
  state->smm_fade.fade_type = FADE_TYPE_FADEIN;
  state->smm_fade.fade_animation_accumulator = 0.f;
  state->smm_fade.fade_animation_playing = true;
  state->smm_fade.is_fade_animation_played = false;
  state->smm_fade.data = data;
  state->smm_fade.on_change_complete = on_change_complete;
}
local_button* smm_add_local_button(i32 _id, button_type_id _btn_type_id, button_state signal_state) {
  if (not state or state == nullptr) {
    IERROR("scene_main_menu::smm_add_local_button()::State is not valid");
    return nullptr;
  }
  if (_btn_type_id <= BTN_TYPE_UNDEFINED or _btn_type_id >= BTN_TYPE_MAX) {
    IWARN("scene_main_menu::smm_add_local_button()::Button type is out of bound");
    return nullptr;
  }
  for (const auto& _btn : state->general_purpose_buttons) {
    if (_btn.id == _id) {
      IWARN("scene_main_menu::smm_add_local_button()::Button id collision");
      return nullptr;
    }
  }
  button_type trait_on_click_button_type = get_button_types()->at(_btn_type_id);

  return __builtin_addressof(state->general_purpose_buttons.emplace_back(local_button(_id, trait_on_click_button_type, signal_state, 
    Rectangle {0, 0, 
      trait_on_click_button_type.dest_frame_dim.x, trait_on_click_button_type.dest_frame_dim.y
    }))
  );
}
panel* smm_add_local_panel(i32 _id, panel in_pnl) {
  if (not state or state == nullptr) {
    IERROR("scene_main_menu::smm_add_local_panel()::State is not valid");
    return nullptr;
  }
  if (in_pnl.frame_tex_id <= ATLAS_TEX_ID_UNSPECIFIED or in_pnl.frame_tex_id >= ATLAS_TEX_ID_MAX) {
    IERROR("scene_main_menu::smm_add_local_panel()::Frame tex is out of bound");
    return nullptr;
  }
  if (in_pnl.bg_tex_id <= ATLAS_TEX_ID_UNSPECIFIED or in_pnl.bg_tex_id >= ATLAS_TEX_ID_MAX) {
    IERROR("scene_main_menu::smm_add_local_panel()::Bg Tex is out of bound");
    return nullptr;
  }
  for (const auto& itr_pnl : state->general_purpose_panels) {
    if (itr_pnl.id == _id) {
      IWARN("scene_main_menu::smm_add_local_panel()::Panel id collision");
      return nullptr;
    }
  }
  in_pnl.id = _id;
  return __builtin_addressof(state->general_purpose_panels.emplace_back(in_pnl));
}
local_button* smm_get_local_button(i32 _id) {
  if (not state or state == nullptr) {
    IERROR("scene_main_menu::smm_get_local_button()::State is not valid");
    return nullptr;
  }
  for (auto& itr_btn : state->general_purpose_buttons) {
    if (itr_btn.id == _id) {
      return __builtin_addressof(itr_btn);
    }
  }
  return nullptr;
}
panel* smm_get_local_panel(i32 _id) {
  if (not state or state == nullptr) {
    IERROR("scene_main_menu::smm_get_local_panel()::State is not valid");
    return nullptr;
  }
  for (auto& itr_pnl : state->general_purpose_panels) {
    if (itr_pnl.id == _id) {
      return __builtin_addressof(itr_pnl);
    }
  }
  return nullptr;
}
void smm_clean_character_context(void) {
  mms_character_upgrade_context& mms_ctx = state->mms_character_upgrade;

  mms_ctx.upgrade_state = mms_ctx.CHARACTER_UPGRADE_STATE_UNDEFINED;
  mms_ctx.slot_index = I32_MAX;
}
void smm_refresh_character_context(void) {
  smm_clean_character_context();
  state->general_purpose_panels.clear();

  for (const player_inventory_slot& slot : gm_get_player_state()->inventory) {
    smm_add_local_panel(state->next_local_panel_id, state->smm_default_panels.panel_active_dark_fantasy_default);
    set_inventory_ui_ex(slot.slot_id, data128(state->next_local_panel_id++));
  }
}
void fade_on_complete_change_main_menu_type(data128 data) {
  if (not state or state == nullptr ) {
    IERROR("scene_main_menu::fade_on_complete_change_main_menu_type()::State is not valid");
    return;
  }
  main_menu_scene_type scene = static_cast<main_menu_scene_type>(data.i32[0]);
  state->smm_fade = ui_fade_control_system();
  begin_scene_change(scene, event_context(data.i32[1] ,data.i32[2] ,data.i32[3]));
}
void fade_on_complete_change_scene(data128 data) {
  if (not state or state == nullptr ) {
    IERROR("scene_main_menu::fade_on_complete_change_scene()::State is not valid");
    return;
  }
  scene_id scene = static_cast<scene_id>(data.i32[0]);
  state->smm_fade = ui_fade_control_system();
  begin_scene_change(scene, event_context(data.i32[1] ,data.i32[2] ,data.i32[3]));
}

void begin_scene_change(main_menu_scene_type mms, [[__maybe_unused__]] event_context context) {
  state->general_purpose_buttons.clear();
  state->general_purpose_panels.clear();
  state->next_local_button_id = 1;
  state->next_local_panel_id = 1;
  state->is_changing_state = false;

  switch (mms) {
    case MAIN_MENU_SCENE_DEFAULT: {
      set_worldmap_location(WORLDMAP_MAINMENU_MAP);
      state->mainmenu_state = MAIN_MENU_SCENE_DEFAULT;
      state->playlist.media_play(__builtin_addressof(state->playlist));
      break;
    }
    case MAIN_MENU_SCENE_SETTINGS: {
      state->mainmenu_state = MAIN_MENU_SCENE_SETTINGS;
      break;
    }
    case MAIN_MENU_SCENE_CHARACTER: {
      smm_refresh_character_context();

      state->mainmenu_state = MAIN_MENU_SCENE_CHARACTER;
      break;
    }
    case MAIN_MENU_SCENE_EXTRAS: {
      state->mainmenu_state = MAIN_MENU_SCENE_EXTRAS;
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE: {
      event_fire(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, event_context(state->in_app_settings->render_width * .5f,state->in_app_settings->render_height * .5f));
      state->mainmenu_state = MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE;
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE: {
      state->mms_trait_choice.available_trait_points = state->in_ingame_info->player_state_static->stats.at(CHARACTER_STATS_TOTAL_TRAIT_POINTS).buffer.i32[3];
      state->mainmenu_state = MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE;
      state->mms_trait_choice.chosen_traits.clear();
      state->mms_trait_choice.positive_traits.clear();
      state->mms_trait_choice.negative_traits.clear();
      state->mms_trait_choice.abilities = (*_get_all_abilities());

      const std::vector<character_trait>& gm_traits = gm_get_character_traits_all();

      for (character_trait _trait : gm_traits) {
        _trait.ui_ops.i32[0] = state->next_local_button_id;
        local_button * const lcl_btn_ptr = smm_add_local_button(state->next_local_button_id++, BTN_TYPE_FLAT_BUTTON, BTN_STATE_RELEASED);
        if (_trait.point < 0) {
          lcl_btn_ptr->btn_type.forground_color_btn_state_up = GREEN;
          lcl_btn_ptr->btn_type.forground_color_btn_state_hover = GREEN;
          lcl_btn_ptr->btn_type.forground_color_btn_state_pressed = GREEN;
          state->mms_trait_choice.positive_traits.push_back(_trait);
        }
        else {
          lcl_btn_ptr->btn_type.forground_color_btn_state_up = RED;
          lcl_btn_ptr->btn_type.forground_color_btn_state_hover = RED;
          lcl_btn_ptr->btn_type.forground_color_btn_state_pressed = RED;
          state->mms_trait_choice.negative_traits.push_back(_trait);
        }
      }

      for (ability& _abl : state->mms_trait_choice.abilities) {
        if (_abl.id <= ABILITY_ID_UNDEFINED or _abl.id >= ABILITY_ID_MAX) continue;
        
        panel * _lc_pnl = smm_add_local_panel(state->next_local_panel_id, state->smm_default_panels.panel_active_dark_fantasy_default);
        if (not _lc_pnl or _lc_pnl == nullptr) continue;

        _abl.ui_use.i32[0] = state->next_local_panel_id++;
      }
      break;
    }
    default: {
      IWARN("scene_main_menu::begin_scene_change()::Unsupported main menu scene type");
      state->mainmenu_state = MAIN_MENU_SCENE_DEFAULT;
      break;
    }
  }
}
void begin_scene_change(scene_id sid, event_context context) {
  if (not state or state == nullptr ) {
    IERROR("scene_main_menu::begin_scene_change()::State is not valid");
    return;
  }

  switch (sid) {
    case SCENE_TYPE_MAIN_MENU: {
      event_fire(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, event_context(0.f, 0.f));
      event_fire(EVENT_CODE_CAMERA_SET_ZOOM, event_context(1.f));
      event_fire(EVENT_CODE_SCENE_MAIN_MENU, context);
      state->is_changing_state = false;
      return; 
    }
    case SCENE_TYPE_IN_GAME: {
      event_fire(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, event_context(0.f, 0.f));
      event_fire(EVENT_CODE_CAMERA_SET_ZOOM, event_context(1.f));
      event_fire(EVENT_CODE_SCENE_IN_GAME, context);
      state->is_changing_state = false;
      return; 
    }
    case SCENE_TYPE_EDITOR: {
      event_fire(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, event_context(0.f, 0.f));
      event_fire(EVENT_CODE_CAMERA_SET_ZOOM, event_context(1.f));
      event_fire(EVENT_CODE_SCENE_EDITOR, context);
      state->is_changing_state = false;
      return; 
    }
    default: {
      IWARN("scene_main_menu::begin_scene_change()::Unsupported scene type");
      event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context(static_cast<i32>(true)));
      state->is_changing_state = false;
      return;
    }
  }
  IERROR("scene_main_menu::begin_scene_change()::Function ended unexpectedly");
}

void positive_trait_button_on_click(i32 index) {
  if (index < 0 or index > static_cast<i32>(state->mms_trait_choice.positive_traits.size() - 1u)) {
    return;
  }
  character_trait& trait = state->mms_trait_choice.positive_traits[index];

  if (state->mms_trait_choice.available_trait_points >= std::abs(trait.point)) {
    state->mms_trait_choice.available_trait_points += trait.point; // INFO: Positive traits have negative points by default

    state->mms_trait_choice.chosen_traits.push_back(trait);
    state->mms_trait_choice.positive_traits.erase(state->mms_trait_choice.positive_traits.begin() + index);
  }
  else {
    event_fire(EVENT_CODE_PLAY_SOUND, event_context(SOUND_ID_DENY1, static_cast<i32>(false)));
  }
}
void negative_trait_button_on_click(i32 index) {
  if (index < 0 or index > static_cast<i32>(state->mms_trait_choice.negative_traits.size() - 1u)) {
    return;
  }
  character_trait& trait = state->mms_trait_choice.negative_traits[index];

  state->mms_trait_choice.available_trait_points += trait.point;

  state->mms_trait_choice.chosen_traits.push_back(trait);
  state->mms_trait_choice.negative_traits.erase(state->mms_trait_choice.negative_traits.begin() + index);
}
void chosen_trait_button_on_click(i32 index) {
  if (index < 0 or index > static_cast<i32>(state->mms_trait_choice.chosen_traits.size() - 1u)) {
    return;
  }
  character_trait& trait = state->mms_trait_choice.chosen_traits[index];

  if (trait.point < 0) {
    state->mms_trait_choice.positive_traits.push_back(trait);
  }
  else {
    state->mms_trait_choice.negative_traits.push_back(trait);
  }
  state->mms_trait_choice.available_trait_points -= trait.point;
  state->mms_trait_choice.chosen_traits.erase(state->mms_trait_choice.chosen_traits.begin() + index);
}

#undef DENY_NOTIFY_TIME
#undef MAIN_MENU_FADE_DURATION
#undef MAIN_MENU_UPGRADE_PANEL_COL
#undef MAIN_MENU_UPGRADE_PANEL_ROW
#undef SMM_BASE_RENDER_SCALE
#undef SMM_BASE_RENDER_DIV2
#undef SMM_BASE_RENDER_RES_VEC
#undef SMM_BASE_RENDER_WIDTH
#undef SMM_BASE_RENDER_HEIGHT
#undef SMM_MAP_PIN_SOURCE_LOCATION_UP
#undef SMM_MAP_PIN_SOURCE_LOCATION_HOVER
#undef SMM_SCROLL_HANDLE_HEIGHT
#undef ZOOM_CAMERA_MAP_ENTRY
#undef UI_CHARACTER_TRAIT_DISPLAY
#undef UI_CHARACTER_TRAIT_HIDDEN
