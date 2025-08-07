#include "scene_main_menu.h"
#include <reasings.h>
#include <steam/steam_api.h>
#include "loc_types.h"

#include "core/fmemory.h"
#include "core/event.h"

#include "game/game_manager.h"
#include "game/user_interface.h"
#include "game/world.h"
#include "game/camera.h"
//#include "game/fshader.h"

typedef enum main_menu_scene_type {
  MAIN_MENU_SCENE_DEFAULT,
  MAIN_MENU_SCENE_SETTINGS,
  MAIN_MENU_SCENE_UPGRADE,
  MAIN_MENU_SCENE_EXTRAS,
  MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE,
  MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE,
} main_menu_scene_type;

typedef struct play_scene_info {
  i32 hovered_stage;
  play_scene_info(void) {
    this->hovered_stage = 0;
  }
}play_scene_info;

typedef struct main_menu_scene_state {
  panel upgrade_list_panel;
  panel upgrade_details_panel;
  panel worldmap_selection_panel;
  panel default_panel;
  panel trait_selection_background_panel;
  panel positive_traits_selection_panel;
  panel negative_traits_selection_panel;
  panel chosen_traits_selection_panel;
  panel trait_details_panel;
  std::vector<character_trait> positive_traits;
  std::vector<character_trait> negative_traits;

  const character_stat * hovered_stat;
  const camera_metrics * in_camera_metrics;
  const app_settings   * in_app_settings;
  const ingame_info    * ingame_info;
  std::array<worldmap_stage, MAX_WORLDMAP_LOCATIONS> worldmap_locations;
  
  const Vector2* mouse_pos_screen;
  u32 deny_notify_timer;
  main_menu_scene_type mainmenu_state;
  bool is_dragging_scroll;
  bool is_any_button_down;

  play_scene_info ingame_scene_feed;
  ui_fade_control_system smm_fade;
  std::vector<local_button> general_purpose_buttons;
  i32 next_local_button_id;

  main_menu_scene_state(void) {
    this->upgrade_list_panel = panel();
    this->upgrade_details_panel = panel();
    this->worldmap_selection_panel = panel();
    this->default_panel = panel();
    this->trait_selection_background_panel = panel();
    this->positive_traits_selection_panel = panel();
    this->negative_traits_selection_panel = panel();
    this->chosen_traits_selection_panel = panel();
    this->trait_details_panel = panel();
    this->positive_traits.clear();
    this->negative_traits.clear();

    this->hovered_stat = nullptr;
    this->in_camera_metrics = nullptr;
    this->in_app_settings = nullptr;
    this->worldmap_locations.fill(worldmap_stage());

    this->mouse_pos_screen = nullptr;
    this->deny_notify_timer = 0u;
    this->mainmenu_state = MAIN_MENU_SCENE_DEFAULT;
    this->is_dragging_scroll = false;
    this->is_any_button_down = false;

    this->ingame_scene_feed = play_scene_info();
    this->smm_fade = ui_fade_control_system();
    this->general_purpose_buttons.clear();
    this->next_local_button_id = 0;
  }
} main_menu_scene_state;

static main_menu_scene_state * state = nullptr;

#define DENY_NOTIFY_TIME .8f * TARGET_FPS

#define MAIN_MENU_FADE_DURATION 1 * TARGET_FPS
#define MAIN_MENU_UPGRADE_PANEL_COL 3
#define MAIN_MENU_UPGRADE_PANEL_ROW 3
#define DRAW_UPGRADE_LABEL(VEC2, TEXT, PRE_UPG_VAL, POS_UPG_VAL) gui_label_format(FONT_TYPE_ABRACADABRA, 1, VEC2.x, VEC2.y, WHITE, true, false, TEXT, PRE_UPG_VAL, POS_UPG_VAL);

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

[[__nodiscard__]] bool begin_scene_main_menu(bool fade_in);

void draw_main_menu_upgrade_panel(void);
void draw_main_menu_upgrade_list_panel(void);
void draw_main_menu_upgrade_details_panel(void);
void draw_trait_selection_panel(void);
void trait_selection_panel_list_traits(panel* const pnl, const Rectangle rect, std::vector<character_trait> * const traits, void (*trait_button_on_click_pfn)(size_t index));
void smm_update_mouse_bindings(void);
void smm_update_keyboard_bindings(void);
void smm_update_bindings(void);
void smm_update_local_buttons(void);
void smm_begin_fadeout(data128 data, void(*on_change_complete)(data128));
void smm_begin_fadein(data128 data, void(*on_change_complete)(data128));
local_button* smm_add_local_button(i32 _id, button_type_id _btn_type_id, button_state signal_state);
local_button* smm_get_local_button(i32 _id);
void fade_on_complete_change_main_menu_type(data128 data);
void fade_on_complete_change_scene(data128 data);

void begin_scene_change(main_menu_scene_type mms, event_context context = event_context());
void begin_scene_change(scene_id sid, event_context context = event_context());

void positive_trait_button_on_click(size_t index);
void negative_trait_button_on_click(size_t index);
void chosen_trait_button_on_click(size_t index);

[[__nodiscard__]] bool initialize_scene_main_menu(const app_settings * _in_app_settings, bool fade_in) {
  if (state) {
    return begin_scene_main_menu(fade_in);
  }
  state = (main_menu_scene_state *)allocate_memory_linear(sizeof(main_menu_scene_state), true);
  if (state == nullptr) {
    TraceLog(LOG_ERROR, "scene_main_menu::initialize_scene_main_menu()::State allocation failed");
    return false;
  }
  *state = main_menu_scene_state();
   
  if (!_in_app_settings || _in_app_settings == nullptr) {
    TraceLog(LOG_ERROR, "scene_main_menu::initialize_scene_main_menu()::App setting pointer is invalid");
    return false;
  }
  state->in_app_settings = _in_app_settings;

  if(!create_camera(state->in_app_settings->render_width_div2, state->in_app_settings->render_height_div2,state->in_app_settings->render_width, state->in_app_settings->render_height)) {
    TraceLog(LOG_ERROR, "scene_main_menu::initialize_scene_main_menu()::Creating camera failed");
    return false;
  }
  state->in_camera_metrics = get_in_game_camera();
  if (!state->in_camera_metrics || state->in_camera_metrics == nullptr) {
    TraceLog(LOG_ERROR, "scene_main_menu::initialize_scene_main_menu()::Camera pointer is invalid");
    return false;
  }
  if(!world_system_begin(state->in_camera_metrics)) {
    TraceLog(LOG_ERROR, "scene_main_menu::initialize_scene_main_menu()::World system begin failed");
    return false;
  }

  return begin_scene_main_menu(fade_in);
}
[[__nodiscard__]] bool begin_scene_main_menu(bool fade_in) {
  if (!user_interface_system_initialize()) {
    TraceLog(LOG_ERROR, "scene_main_menu::begin_scene_main_menu()::User interface failed to initialize!");
    return false;
  }
  if (!game_manager_initialize( state->in_camera_metrics, state->in_app_settings, get_active_map_ptr())) { // Inits player & spawns
    TraceLog(LOG_ERROR, "scene_in_game::begin_scene_main_menu()::game_manager_initialize() failed");
    return false;
  }
  state->ingame_info = gm_get_ingame_info();

  set_worldmap_location(WORLDMAP_MAINMENU_MAP); // NOTE: Worldmap index 0 is mainmenu background now

  copy_memory(state->worldmap_locations.data(), get_worldmap_locations(), MAX_WORLDMAP_LOCATIONS * sizeof(worldmap_stage));

  state->mainmenu_state = MAIN_MENU_SCENE_DEFAULT;
  state->default_panel = panel( BTN_STATE_UNDEFINED, ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG, ATLAS_TEX_ID_DARK_FANTASY_PANEL_SELECTED, 
    Vector4 {10, 10, 10, 10}, Color { 30, 39, 46, 245}, Color { 52, 64, 76, 245}
  );
  state->trait_selection_background_panel = state->default_panel;
  state->positive_traits_selection_panel = panel( BTN_STATE_UNDEFINED, ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG, ATLAS_TEX_ID_DARK_FANTASY_PANEL, 
    Vector4 {6, 6, 6, 6}, Color { 30, 39, 46, 245}, Color { 52, 64, 76, 245}
  );
  state->negative_traits_selection_panel = state->positive_traits_selection_panel;
  state->chosen_traits_selection_panel = state->positive_traits_selection_panel;
  state->trait_details_panel = state->positive_traits_selection_panel;

  state->worldmap_selection_panel = state->default_panel;
  state->upgrade_list_panel = panel();
  state->upgrade_details_panel = panel();

  gm_save_game();

  if (fade_in) {
    smm_begin_fadein(data128(), nullptr);
  }
  //event_fire(EVENT_CODE_PLAY_MAIN_MENU_THEME, event_context{}); TODO: Uncomment later
  return true;
}
void end_scene_main_menu(void) {
  event_fire(EVENT_CODE_RESET_MUSIC, event_context((i32)MUSIC_ID_MAIN_MENU_THEME));
}

void update_scene_main_menu(void) {
  update_user_interface();
  state->mouse_pos_screen = ui_get_mouse_pos_screen();
  smm_update_bindings();
  update_camera();
  update_map();
  if(state->smm_fade.fade_animation_playing){
    process_fade_effect(__builtin_addressof(state->smm_fade));
  }
  else if (state->smm_fade.is_fade_animation_played && state->smm_fade.on_change_complete != nullptr) {
    state->smm_fade.on_change_complete(state->smm_fade.data);
  }
  
  switch (state->mainmenu_state) {
    case MAIN_MENU_SCENE_DEFAULT: {
      event_fire(EVENT_CODE_CAMERA_SET_ZOOM, event_context(1.f));
      event_fire(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, event_context(0.f, 0.f));
      event_fire(EVENT_CODE_CAMERA_SET_OFFSET, event_context(state->in_app_settings->render_width * .5f,state->in_app_settings->render_height * .5f));
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE: {
      event_fire(EVENT_CODE_CAMERA_SET_ZOOM, event_context(1.f));
      event_fire(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, event_context(0.f, 0.f));
      event_fire(EVENT_CODE_CAMERA_SET_OFFSET, event_context(0.f, 0.f));
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE: {
      event_fire(EVENT_CODE_CAMERA_SET_ZOOM, event_context(1.f));
      event_fire(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, event_context(0.f, 0.f));
      event_fire(EVENT_CODE_CAMERA_SET_OFFSET, event_context(state->in_app_settings->render_width * .5f,state->in_app_settings->render_height * .5f));

      smm_update_local_buttons();
      break;
    }
    case MAIN_MENU_SCENE_SETTINGS: {
      break;
    }
    case MAIN_MENU_SCENE_EXTRAS: {
      break;
    }
    case MAIN_MENU_SCENE_UPGRADE: {
      break;
    }
    default: {
      break;
    }
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

      //BeginShaderMode(get_shader_by_enum(SHADER_ID_MAP_CHOICE_IMAGE)->handle);
      {
        gui_draw_texture_id(TEX_ID_WORLDMAP_WO_CLOUDS, map_choice_image_dest, ZEROVEC2);
      }
      //EndShaderMode();

      for (i32 itr_000 = 0; itr_000 < MAX_WORLDMAP_LOCATIONS; ++itr_000) {
        if (state->worldmap_locations.at(itr_000).is_active) {
          const Vector2& normalized_pin_location = state->worldmap_locations.at(itr_000).screen_location;
          const Rectangle pin_location = Rectangle {
            map_choice_image_dest.x + normalized_pin_location.x * map_choice_image_dest.width - (map_pin_dim * .5f), 
            map_choice_image_dest.y + normalized_pin_location.y * map_choice_image_dest.height - (map_pin_dim * .5f),
            map_pin_dim, map_pin_dim,
          };
          if(state->ingame_scene_feed.hovered_stage == itr_000) {
            gui_draw_texture_id_pro(TEX_ID_ASSET_ATLAS, SMM_MAP_PIN_SOURCE_LOCATION_HOVER, pin_location); // INFO: MAP PIN TEXTURES
          }
          else {
            gui_draw_texture_id_pro(TEX_ID_ASSET_ATLAS, SMM_MAP_PIN_SOURCE_LOCATION_UP, pin_location);
          }
        }
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
    case MAIN_MENU_SCENE_UPGRADE: {
      render_map();
      break;
    }
    default: { break; }
  }
  
  EndMode2D();
}
void render_interface_main_menu(void) {
  if (state->mainmenu_state == MAIN_MENU_SCENE_DEFAULT) {
      
    gui_label_shader(GAME_TITLE, SHADER_ID_FONT_OUTLINE, FONT_TYPE_ABRACADABRA, 5, VECTOR2(SMM_BASE_RENDER_WIDTH * .5f, SMM_BASE_RENDER_HEIGHT * .25f), WHITE, true, true);
    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_PLAY), BTN_ID_MAINMENU_BUTTON_PLAY, VECTOR2(0.f, -21.f), SMM_BASE_RENDER_DIV2, true)) {
      begin_scene_change(MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE);
    }
    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_UPGRADE), BTN_ID_MAINMENU_BUTTON_UPGRADE, VECTOR2(0.f, -10.5f), SMM_BASE_RENDER_DIV2, true)) {
      state->mainmenu_state = MAIN_MENU_SCENE_UPGRADE;
    }
    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_SETTINGS), BTN_ID_MAINMENU_BUTTON_SETTINGS, VECTOR2(0.f, 0.f), SMM_BASE_RENDER_DIV2, true)) {
      ui_refresh_setting_sliders_to_default();
      state->mainmenu_state = MAIN_MENU_SCENE_SETTINGS;
    }
    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_EDITOR), BTN_ID_MAINMENU_BUTTON_EDITOR, VECTOR2(0.f, 10.5f), SMM_BASE_RENDER_DIV2, true)) {
      smm_begin_fadeout(data128(SCENE_TYPE_EDITOR, true), fade_on_complete_change_scene);
    }
    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_EXIT), BTN_ID_MAINMENU_BUTTON_EXIT, VECTOR2(0.f, 21.f), SMM_BASE_RENDER_DIV2, true)) {
      event_fire(EVENT_CODE_APPLICATION_QUIT, event_context());
    }
  } 
  else if (state->mainmenu_state == MAIN_MENU_SCENE_SETTINGS) {
      gui_draw_settings_screen();
      if (gui_menu_button(lc_txt(LOC_TEXT_SETTINGS_BUTTON_CANCEL), BTN_ID_MAINMENU_SETTINGS_CANCEL, VECTOR2(-35.f, 66.5f), SMM_BASE_RENDER_DIV2, true)) {
        state->mainmenu_state = MAIN_MENU_SCENE_DEFAULT;
      }
  }
  else if (state->mainmenu_state == MAIN_MENU_SCENE_UPGRADE) {
      draw_main_menu_upgrade_panel();
      if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_UPDATE_BUTTON_BACK), BTN_ID_MAINMENU_UPGRADE_BACK, VECTOR2(0.f, 66.5f), SMM_BASE_RENDER_DIV2, true)) {
        state->mainmenu_state = MAIN_MENU_SCENE_DEFAULT;
      }
  }
  else if (state->mainmenu_state == MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE) {
    for (int itr_000 = 0; itr_000 < MAX_WORLDMAP_LOCATIONS; ++itr_000) {
      if (state->ingame_scene_feed.hovered_stage == itr_000) {
        const f32 image_ratio = 3840.f / 2160.f; // INFO: ratio of map texture
        const f32 image_height = static_cast<f32>(SMM_BASE_RENDER_HEIGHT * .85f);
        const Vector2 image_dim = Vector2 { image_height * image_ratio, image_height};
        Rectangle map_choice_image_dest = Rectangle { SMM_BASE_RENDER_DIV2.x, 0.f, image_dim.x, image_dim.y };
        map_choice_image_dest.x -= image_dim.x * .5f;
        const f32 map_pin_dim = map_choice_image_dest.height * .05f;

        panel* pnl = __builtin_addressof(state->worldmap_selection_panel);
        const Vector2& normalized_pin_location = state->worldmap_locations.at(itr_000).screen_location;
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
          gui_label(state->worldmap_locations.at(itr_000).displayname.c_str(), FONT_TYPE_ABRACADABRA, 1, Vector2 {
            pnl->dest.x + pnl->dest.width *.5f, pnl->dest.y + pnl->dest.height*.5f
          }, WHITE, true, true);
        }
        EndScissorMode();
      }
    }

    if(gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_MAP_CHOICE_BACK), BTN_ID_MAINMENU_MAP_CHOICE_BACK, VECTOR2(-35.f, 66.5f), SMM_BASE_RENDER_DIV2, true)) {
      state->mainmenu_state = MAIN_MENU_SCENE_DEFAULT;
    }
  }
  else if(state->mainmenu_state == MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE) {
    draw_trait_selection_panel();

    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_TRAIT_CHOICE_BUTTON_BACK), BTN_ID_MAINMENU_TRAIT_CHOICE_BACK, VECTOR2(-35.f, 66.5f), SMM_BASE_RENDER_DIV2, true)) {
      begin_scene_change(MAIN_MENU_SCENE_DEFAULT);
    }

    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_TRAIT_CHOICE_BUTTON_ACCEPT), BTN_ID_MAINMENU_TRAIT_CHOICE_ACCEPT, VECTOR2(35.f, 66.5f), SMM_BASE_RENDER_DIV2, true)) {
      smm_begin_fadeout(data128(SCENE_TYPE_IN_GAME, true), fade_on_complete_change_scene);
    }
  }
  render_user_interface();
}

void draw_main_menu_upgrade_panel(void) {
  Rectangle header_loc = {0, 0, static_cast<f32>(SMM_BASE_RENDER_WIDTH), SMM_BASE_RENDER_HEIGHT * .1f};
  Rectangle footer_loc = {0, 
    SMM_BASE_RENDER_HEIGHT - SMM_BASE_RENDER_HEIGHT *.1f, 
    static_cast<f32>(SMM_BASE_RENDER_WIDTH), SMM_BASE_RENDER_HEIGHT * .1f};
  DrawRectangleRec(header_loc, Color{0, 0, 0, 50});
  DrawRectangleRec(footer_loc, Color{0, 0, 0, 50});
  
  atlas_texture_id icon_tex_id = ATLAS_TEX_ID_CURRENCY_SOUL_ICON_5000;
  f32 cost_icon_dim = header_loc.height;
  Rectangle cost_icon_pos = Rectangle {
    SMM_BASE_RENDER_WIDTH * .5f - cost_icon_dim * .3f, SMM_BASE_RENDER_HEIGHT * .05f,
    cost_icon_dim, cost_icon_dim
  };
  Vector2 icon_tex_origin = Vector2 {
    cost_icon_pos.width / 2.f,
    cost_icon_pos.height / 2.f,
  };
  gui_draw_atlas_texture_id(icon_tex_id, cost_icon_pos, icon_tex_origin, 0.f);

  Vector2 cost_label_pos = VECTOR2(SMM_BASE_RENDER_WIDTH *.5f + cost_icon_dim * .3f, SMM_BASE_RENDER_HEIGHT * .05f);
  gui_label_format_v(FONT_TYPE_ABRACADABRA, 1, cost_label_pos, WHITE, true, true, "%d", get_currency_souls());

  draw_main_menu_upgrade_list_panel();
  draw_main_menu_upgrade_details_panel();
}
void draw_main_menu_upgrade_list_panel(void) {
  state->upgrade_list_panel.dest = Rectangle{ SMM_BASE_RENDER_WIDTH * .025f, SMM_BASE_RENDER_HEIGHT * .075f, SMM_BASE_RENDER_WIDTH * .65f, SMM_BASE_RENDER_HEIGHT * .85f};
  gui_panel(state->upgrade_list_panel, state->upgrade_list_panel.dest, false);
  f32 showcase_hover_scale = 1.1f;
  f32 showcase_base_dim = SMM_BASE_RENDER_HEIGHT * .25f;

  const f32 showcase_spacing = showcase_base_dim * 1.1f;
  const f32 total_showcases_width = (MAIN_MENU_UPGRADE_PANEL_COL * showcase_base_dim) + ((MAIN_MENU_UPGRADE_PANEL_COL - 1) * (showcase_spacing - showcase_base_dim));
  const f32 total_showcases_height = (MAIN_MENU_UPGRADE_PANEL_ROW * showcase_base_dim) + ((MAIN_MENU_UPGRADE_PANEL_ROW - 1) * (showcase_spacing - showcase_base_dim));
  const Vector2 showcase_start_pos = VECTOR2(
    state->upgrade_list_panel.dest.x + (state->upgrade_list_panel.dest.width / 2.f) - (total_showcases_width / 2.f),
    state->upgrade_list_panel.dest.y + (state->upgrade_list_panel.dest.height / 2.f) - (total_showcases_height / 2.f)
  );

  for (i32 iter = 0; iter < MAIN_MENU_UPGRADE_PANEL_ROW; ++iter) {
    for (i32 j = 0; j < MAIN_MENU_UPGRADE_PANEL_COL; ++j) {
      const character_stat *stat = get_static_player_state_stat(static_cast<character_stats>((MAIN_MENU_UPGRADE_PANEL_COL * iter) + j + 1));
      if (!stat || stat->id >= CHARACTER_STATS_MAX ||
        stat->id <= CHARACTER_STATS_UNDEFINED) {
        continue;
      }
      bool hovered = false;

      Vector2 showcase_position = VECTOR2(showcase_start_pos.x + j * showcase_spacing, showcase_start_pos.y + iter * showcase_spacing);
      f32 showcase_new_dim = showcase_base_dim;
      if (CheckCollisionPointRec( *state->mouse_pos_screen, Rectangle {showcase_position.x, showcase_position.y, showcase_base_dim, showcase_base_dim})) {
        hovered = true;
        showcase_new_dim *= showcase_hover_scale;
        showcase_position.x -= (showcase_new_dim - showcase_base_dim) / 2.f;
        showcase_position.y -= (showcase_new_dim - showcase_base_dim) / 2.f;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
          state->hovered_stat = stat;
        }
      }
      
      // TITLE
      Rectangle header_tex_src_rect = ( * get_atlas_texture_source_rect(ATLAS_TEX_ID_LITTLE_SHOWCASE) );
      header_tex_src_rect.width  *= 2.75f;
      header_tex_src_rect.height *= 3.f;
      Rectangle header_tex_pos = {
        showcase_position.x + showcase_new_dim * .5f, showcase_position.y + (showcase_new_dim * .05f), 
        header_tex_src_rect.width, header_tex_src_rect.height
      };
      if (hovered) {
        header_tex_pos.width *= showcase_hover_scale;
        header_tex_pos.height *= showcase_hover_scale;
      }
      Vector2 header_tex_origin = Vector2 {header_tex_pos.width * .5f, header_tex_pos.height * .5f * (-0.15f)};

      Vector2 title_pos = VECTOR2(showcase_position.x + showcase_new_dim * .5f, header_tex_pos.y + header_tex_pos.height * 0.375f);
      // TITLE
      
      // STARS
      const Rectangle * tier_symbol_src_rect = get_atlas_texture_source_rect(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR);
      f32 star_spacing = tier_symbol_src_rect->width * 1.25f;
      f32 tier_symbols_total_width = tier_symbol_src_rect->width + (MAX_PASSIVE_UPGRADE_TIER - 1.f) * star_spacing;
      f32 tier_symbols_left_edge = showcase_position.x + (showcase_new_dim - tier_symbols_total_width) / 2.f;
      f32 tier_symbols_vertical_center = showcase_position.y + showcase_new_dim * .8f;
      // STARS

      // ICON POS
      Rectangle icon_pos = {
        showcase_position.x + showcase_new_dim * .25f, showcase_position.y + showcase_new_dim * .25f,
        showcase_new_dim * .5f, showcase_new_dim * .5f
      };
      // ICON POS

      gui_draw_atlas_texture_id(ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG, Rectangle {showcase_position.x, showcase_position.y, showcase_new_dim, showcase_new_dim}, VECTOR2(0, 0), 0.f);
      gui_draw_atlas_texture_id(ATLAS_TEX_ID_DARK_FANTASY_PANEL,    Rectangle {showcase_position.x, showcase_position.y, showcase_new_dim, showcase_new_dim}, VECTOR2(0, 0), 0.f);

      for (i32 i = 0; i < MAX_PASSIVE_UPGRADE_TIER; ++i) {
        Vector2 tier_pos = {tier_symbols_left_edge + i * star_spacing, tier_symbols_vertical_center};
        if (i < stat->level-1) {
          gui_draw_atlas_texture_id_scale(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR, tier_pos, 1.f, RED, false);
        } else {
          gui_draw_atlas_texture_id_scale(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR, tier_pos, 1.f, WHITE, false);
        }
      }

      gui_draw_texture_id_pro(TEX_ID_ASSET_ATLAS, stat->passive_icon_src, icon_pos);
      gui_draw_atlas_texture_id(ATLAS_TEX_ID_LITTLE_SHOWCASE, header_tex_pos, header_tex_origin, 0.f);
      gui_label(lc_txt(stat->passive_display_name_symbol), FONT_TYPE_ABRACADABRA, 1, title_pos, WHITE, true, true);
    }
  }
}
void draw_main_menu_upgrade_details_panel(void) {
  state->upgrade_details_panel.dest = Rectangle{
    state->upgrade_list_panel.dest.x + state->upgrade_list_panel.dest.width + SMM_BASE_RENDER_WIDTH * .005f,
    SMM_BASE_RENDER_HEIGHT * .075f,
    SMM_BASE_RENDER_WIDTH * .295f, SMM_BASE_RENDER_HEIGHT * .850f
  };

  gui_panel(state->upgrade_details_panel, state->upgrade_details_panel.dest, false);
  if (!state->hovered_stat) {
    return;
  }
  f32 detail_panel_element_spacing = state->upgrade_details_panel.dest.height * 0.05f;

  Rectangle icon_pos = {
    state->upgrade_details_panel.dest.x + state->upgrade_details_panel.dest.width * .5f - (state->upgrade_details_panel.dest.width * .175f),
    state->upgrade_details_panel.dest.y + detail_panel_element_spacing,
    state->upgrade_details_panel.dest.width * .35f,
    state->upgrade_details_panel.dest.width * .35f
  };
  gui_draw_texture_id_pro(TEX_ID_ASSET_ATLAS, state->hovered_stat->passive_icon_src, icon_pos);

  const Rectangle * tier_symbol_src_rect = get_atlas_texture_source_rect(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR);
  f32 star_spacing = tier_symbol_src_rect->width * 1.25f;
  f32 tier_symbols_total_width = tier_symbol_src_rect->width + (MAX_PASSIVE_UPGRADE_TIER - 1.f) * star_spacing;
  f32 tier_symbols_left_edge = state->upgrade_details_panel.dest.x + (state->upgrade_details_panel.dest.width - tier_symbols_total_width) / 2.f;
  f32 tier_symbols_vertical_position = icon_pos.y + icon_pos.height + detail_panel_element_spacing * .5f;
  for (i32 i = 0; i < MAX_PASSIVE_UPGRADE_TIER; ++i) {
    Vector2 tier_pos = {tier_symbols_left_edge + i * star_spacing, tier_symbols_vertical_position};
    if (i < state->hovered_stat->level-1) {
      gui_draw_atlas_texture_id_scale(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR, tier_pos, 1.f, RED, false);
    } else {
      gui_draw_atlas_texture_id_scale(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR, tier_pos, 1.f, WHITE, false);
    }
  }

  Vector2 title_pos = VECTOR2(
    state->upgrade_details_panel.dest.x + state->upgrade_details_panel.dest.width * .5f,
    tier_symbols_vertical_position + tier_symbol_src_rect->height + detail_panel_element_spacing * .5f
  );
  gui_label(lc_txt(state->hovered_stat->passive_display_name_symbol), FONT_TYPE_ABRACADABRA, 1, title_pos, WHITE, true, true);

  Rectangle description_pos = {
    state->upgrade_details_panel.dest.x +state->upgrade_details_panel.dest.width * .05f, title_pos.y + detail_panel_element_spacing * .75f,
    state->upgrade_details_panel.dest.width * .9f, state->upgrade_details_panel.dest.width * .35f
  };
  gui_label_wrap(lc_txt(state->hovered_stat->passive_desc_symbol), FONT_TYPE_ABRACADABRA, 1, description_pos, WHITE, false);

  character_stat pseudo_update = *state->hovered_stat;
  game_manager_set_stat_value_by_level(&pseudo_update, pseudo_update.level+1);
  Vector2 upg_stat_text_pos = {
    state->upgrade_details_panel.dest.x + state->upgrade_details_panel.dest.width * .5f,
    state->upgrade_details_panel.dest.y + state->upgrade_details_panel.dest.height * .5f,
  };

  switch (state->hovered_stat->id) {
    case CHARACTER_STATS_HEALTH: {
      DRAW_UPGRADE_LABEL(upg_stat_text_pos, "%d -> %d", state->hovered_stat->buffer.u32[0], pseudo_update.buffer.u32[0]) break;
    }
    case CHARACTER_STATS_HP_REGEN: {
      DRAW_UPGRADE_LABEL(upg_stat_text_pos, "%d -> %d", state->hovered_stat->buffer.u32[0], pseudo_update.buffer.u32[0]) break;
    }
    case CHARACTER_STATS_MOVE_SPEED: {
      DRAW_UPGRADE_LABEL(upg_stat_text_pos, "%.1f -> %.1f", state->hovered_stat->buffer.f32[0], pseudo_update.buffer.f32[0]) break;
    }
    case CHARACTER_STATS_AOE: {
      DRAW_UPGRADE_LABEL(upg_stat_text_pos, "%.1f -> %.1f", state->hovered_stat->buffer.f32[0], pseudo_update.buffer.f32[0]) break;
    }
    case CHARACTER_STATS_DAMAGE: {
      DRAW_UPGRADE_LABEL(upg_stat_text_pos, "%d -> %d", state->hovered_stat->buffer.u32[0], pseudo_update.buffer.u32[0]) break;
    }
    case CHARACTER_STATS_ABILITY_CD: {
      DRAW_UPGRADE_LABEL(upg_stat_text_pos, "%.1f -> %.1f", state->hovered_stat->buffer.f32[0], pseudo_update.buffer.f32[0]) break;
    }
    case CHARACTER_STATS_PROJECTILE_AMOUTH: {
      DRAW_UPGRADE_LABEL(upg_stat_text_pos, "%d -> %d", state->hovered_stat->buffer.u16[0], pseudo_update.buffer.u16[0]) break;
    }
    case CHARACTER_STATS_EXP_GAIN: {
      DRAW_UPGRADE_LABEL(upg_stat_text_pos, "%.1f -> %.1f", state->hovered_stat->buffer.f32[0], pseudo_update.buffer.f32[0]) break;
    }
    default: {
      TraceLog(LOG_ERROR, "game_manager::upgrade_player_stat()::Unsuppported stat id");
      break;
    }
  }

  atlas_texture_id icon_tex_id = ATLAS_TEX_ID_CURRENCY_SOUL_ICON_5000;
  Rectangle cost_icon_dest = Rectangle(
    state->upgrade_details_panel.dest.x + state->upgrade_details_panel.dest.width * .425f,
    state->upgrade_details_panel.dest.y + state->upgrade_details_panel.dest.height * .85f,
    state->upgrade_details_panel.dest.width * .25f,
    state->upgrade_details_panel.dest.width * .25f
  );
  Vector2 cost_icon_origin = Vector2(
    cost_icon_dest.width / 2.f,
    cost_icon_dest.height / 2.f
  );
  gui_draw_atlas_texture_id(icon_tex_id, cost_icon_dest, cost_icon_origin, true);

  if (state->deny_notify_timer > DENY_NOTIFY_TIME) {
    state->deny_notify_timer = DENY_NOTIFY_TIME;
  }
  if (state->deny_notify_timer > 0 && state->deny_notify_timer <= DENY_NOTIFY_TIME) {
    state->deny_notify_timer -= 1;
  }
  Color cost_label_color = WHITE;
  cost_label_color.g = EaseSineIn(state->deny_notify_timer, 255, -255, DENY_NOTIFY_TIME);
  cost_label_color.b = EaseSineIn(state->deny_notify_timer, 255, -255, DENY_NOTIFY_TIME);
  Vector2 cost_label_pos = VECTOR2(
    state->upgrade_details_panel.dest.x + state->upgrade_details_panel.dest.width * .575f,
    state->upgrade_details_panel.dest.y + state->upgrade_details_panel.dest.height * .85f
  );
  gui_label_format_v(FONT_TYPE_ABRACADABRA, 1, cost_label_pos, cost_label_color, true, true, "%d", state->hovered_stat->upgrade_cost);

  Vector2 button_location = Vector2 {
    state->upgrade_details_panel.dest.x + state->upgrade_details_panel.dest.width * .5f, 
    state->upgrade_details_panel.dest.y  + state->upgrade_details_panel.dest.height * .95f
  };
  if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_UPDATE_BUTTON_UPGRADE), BTN_ID_MAINMENU_UPGRADE_BUY_UPGRADE, Vector2 {0.f, 0.f}, button_location, false)) {
    if (((i32)get_currency_souls() - state->hovered_stat->upgrade_cost) >= 0) {
      currency_souls_add(-state->hovered_stat->upgrade_cost);
      i32 level = get_static_player_state_stat(state->hovered_stat->id)->level;
      set_static_player_state_stat(state->hovered_stat->id, level+1);
      gm_save_game();
      event_fire(EVENT_CODE_PLAY_BUTTON_ON_CLICK, event_context((u16)true));
    } else {
      event_fire(EVENT_CODE_PLAY_SOUND, event_context((i32)SOUND_ID_DENY));
      state->deny_notify_timer = DENY_NOTIFY_TIME;
    }
  }
}
void draw_trait_selection_panel(void) {
  Rectangle bg_trait_sel_pan_dest = Rectangle {
    static_cast<f32>(SMM_BASE_RENDER_WIDTH) * .1f, static_cast<f32>(SMM_BASE_RENDER_HEIGHT) * .1f, 
    static_cast<f32>(SMM_BASE_RENDER_WIDTH) * .8f, static_cast<f32>(SMM_BASE_RENDER_HEIGHT) * .8f
  };
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
  Rectangle trait_desc_pan_dest = Rectangle {
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
    trait_desc_pan_dest.x + trait_desc_pan_dest.width * .5f,
    bg_trait_sel_pan_dest.y + bg_trait_sel_pan_dest.height * .01f
  };

  state->trait_selection_background_panel.dest = bg_trait_sel_pan_dest;
  gui_panel(state->trait_selection_background_panel, bg_trait_sel_pan_dest, false);

  state->positive_traits_selection_panel.dest = positive_traits_sel_pan_dest;
  gui_panel(state->positive_traits_selection_panel, positive_traits_sel_pan_dest, false);
  state->negative_traits_selection_panel.dest = negative_traits_sel_pan_dest;
  gui_panel(state->negative_traits_selection_panel, negative_traits_sel_pan_dest, false);
  state->chosen_traits_selection_panel.dest = chosen_traits_sel_pan_dest;
  gui_panel(state->chosen_traits_selection_panel, chosen_traits_sel_pan_dest, false);
  state->trait_details_panel.dest = trait_desc_pan_dest;
  gui_panel(state->trait_details_panel, trait_desc_pan_dest, false);

  gui_label(lc_txt(LOC_TEXT_MAINMENU_TRAIT_CHOICE_AVAILABLE_TRAITS_TITLE), FONT_TYPE_ABRACADABRA, 1, available_traits_title_label_dest, WHITE, true, false);
  gui_label(lc_txt(LOC_TEXT_MAINMENU_TRAIT_CHOICE_CHOSEN_TRAITS_TITLE),    FONT_TYPE_ABRACADABRA, 1, chosen_traits_title_label_dest, WHITE, true, false);
  gui_label(lc_txt(LOC_TEXT_MAINMENU_TRAIT_CHOICE_CHOSEN_TRAIT_DESC_TITLE),FONT_TYPE_ABRACADABRA, 1, chosen_trait_desc_title_label_dest, WHITE, true, false);

  Rectangle positive_selection_panel_with_padding = Rectangle {
    positive_traits_sel_pan_dest.x      + (positive_traits_sel_pan_dest.width  * .025f),
    positive_traits_sel_pan_dest.y      + (positive_traits_sel_pan_dest.height * .025f),
    positive_traits_sel_pan_dest.width  - (positive_traits_sel_pan_dest.width  * .05f),
    positive_traits_sel_pan_dest.height - (positive_traits_sel_pan_dest.height * .05f)
  };
  trait_selection_panel_list_traits(
    __builtin_addressof(state->positive_traits_selection_panel), positive_selection_panel_with_padding, __builtin_addressof(state->positive_traits), positive_trait_button_on_click
  );

  Rectangle negative_selection_panel_with_padding = Rectangle {
    negative_traits_sel_pan_dest.x      + (negative_traits_sel_pan_dest.width  * .025f),
    negative_traits_sel_pan_dest.y      + (negative_traits_sel_pan_dest.height * .025f),
    negative_traits_sel_pan_dest.width  - (negative_traits_sel_pan_dest.width  * .05f),
    negative_traits_sel_pan_dest.height - (negative_traits_sel_pan_dest.height * .05f)
  };
  trait_selection_panel_list_traits(
    __builtin_addressof(state->negative_traits_selection_panel), negative_selection_panel_with_padding, __builtin_addressof(state->negative_traits), negative_trait_button_on_click
  );

  Rectangle chosen_panel_with_padding = Rectangle {
    chosen_traits_sel_pan_dest.x      + (chosen_traits_sel_pan_dest.width  * .025f),
    chosen_traits_sel_pan_dest.y      + (chosen_traits_sel_pan_dest.height * .025f),
    chosen_traits_sel_pan_dest.width  - (chosen_traits_sel_pan_dest.width  * .05f),
    chosen_traits_sel_pan_dest.height - (chosen_traits_sel_pan_dest.height * .05f)
  };
  trait_selection_panel_list_traits(
    __builtin_addressof(state->chosen_traits_selection_panel), chosen_panel_with_padding, state->ingame_info->chosen_traits, chosen_trait_button_on_click
  );

}
void trait_selection_panel_list_traits(panel* const pnl, const Rectangle rect, std::vector<character_trait> * const traits, void (*trait_button_on_click_pfn)(size_t index)) {
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
    character_trait * _trait = nullptr;
    for (size_t itr_000 = 0; itr_000 < traits->size(); ++itr_000) {
      _trait = __builtin_addressof(traits->at(itr_000));
      local_button* _lc_btn = smm_get_local_button(_trait->ui_use.i32[0]);
      text_measure = ui_measure_text(_trait->title.c_str(), FONT_TYPE_ABRACADABRA, 1);
      Vector2 _lc_btn_pos = VECTOR2(rect.x, rect.y + trait_list_height_buffer + (pnl->buffer.f32[0] * pnl->scroll));
      Rectangle _lc_btn_dest = Rectangle {
        _lc_btn_pos.x, _lc_btn_pos.y, _lc_btn->btn_type.dest_frame_dim.x, _lc_btn->btn_type.dest_frame_dim.y
      };

      if (gui_draw_local_button(_trait->title.c_str(), _lc_btn, FONT_TYPE_ABRACADABRA, 1, _lc_btn_pos, TEXT_ALIGN_LEFT_CENTER, false)) {
        if (trait_list_height_buffer + (pnl->buffer.f32[0] * pnl->scroll) < rect.height) {
          trait_button_on_click_pfn(itr_000);
          _lc_btn->current_state = BTN_STATE_UP;
          continue;
        }
      }
      switch (_lc_btn->current_state) {
        case BTN_STATE_UP:{
          gui_label_box_format(FONT_TYPE_ABRACADABRA, 1, _lc_btn_dest, _lc_btn->btn_type.forground_color_btn_state_up, TEXT_ALIGN_RIGHT_CENTER, "%d", _trait->point);
          break;
        }
        case BTN_STATE_HOVER:{
          gui_label_box_format(FONT_TYPE_ABRACADABRA, 1, _lc_btn_dest, _lc_btn->btn_type.forground_color_btn_state_hover, TEXT_ALIGN_RIGHT_CENTER, "%d", _trait->point);
          break;
        }
        case BTN_STATE_PRESSED:{
          gui_label_box_format(FONT_TYPE_ABRACADABRA, 1, _lc_btn_dest, _lc_btn->btn_type.forground_color_btn_state_pressed, TEXT_ALIGN_RIGHT_CENTER, "%d", _trait->point);
          break;
        }
        default: break; 
      }
      trait_list_height_buffer += text_measure.y + (text_measure.y * .1f);
    }
    pnl->buffer.f32[0] = trait_list_height_buffer - rect.height - (text_measure.y * .2f);
    pnl->scroll_handle.x = rect.x + rect.width - scroll_handle_width;
    pnl->scroll_handle.width = scroll_handle_width;
    pnl->scroll_handle.height = SMM_SCROLL_HANDLE_HEIGHT;
    pnl->scroll_handle.y = FMAX(pnl->scroll_handle.y, rect.y + panel_scroll_bg_head_dest.height);
    pnl->scroll_handle.y = FMIN(pnl->scroll_handle.y, rect.y + rect.height - panel_scroll_bg_bottom_dest.height - pnl->scroll_handle.height);
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

void smm_update_mouse_bindings(void) { 
  switch (state->mainmenu_state) {
    case MAIN_MENU_SCENE_DEFAULT: {
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE: {
      state->ingame_scene_feed.hovered_stage = U16_MAX;
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
        }
      }
      
      if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->ingame_scene_feed.hovered_stage <= MAX_WORLDMAP_LOCATIONS) {
        set_worldmap_location(state->ingame_scene_feed.hovered_stage);
        begin_scene_change(MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE, event_context());
      }
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE: {
      if (CheckCollisionPointRec( (*state->mouse_pos_screen), state->positive_traits_selection_panel.dest)) {
        panel* pnl = &state->positive_traits_selection_panel;

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
      if (CheckCollisionPointRec( (*state->mouse_pos_screen), state->negative_traits_selection_panel.dest)) {
        panel* pnl = &state->negative_traits_selection_panel;

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
      if (CheckCollisionPointRec( (*state->mouse_pos_screen), state->chosen_traits_selection_panel.dest)) {
        panel* pnl = &state->chosen_traits_selection_panel;

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
        if (state->positive_traits_selection_panel.is_dragging_scroll) {
          panel* pnl = __builtin_addressof(state->positive_traits_selection_panel);
          pnl->scroll_handle.y = state->mouse_pos_screen->y - pnl->scroll_handle.height * .5f;
        }
        if (state->negative_traits_selection_panel.is_dragging_scroll) {
          panel* pnl = __builtin_addressof(state->negative_traits_selection_panel);
          pnl->scroll_handle.y = state->mouse_pos_screen->y - pnl->scroll_handle.height * .5f;
        }
        if (state->chosen_traits_selection_panel.is_dragging_scroll) {
          panel* pnl = __builtin_addressof(state->chosen_traits_selection_panel);
          pnl->scroll_handle.y = state->mouse_pos_screen->y - pnl->scroll_handle.height * .5f;
        }
      }
      if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) 
      {
        if (state->positive_traits_selection_panel.is_dragging_scroll) {
          state->positive_traits_selection_panel.is_dragging_scroll = false;
          state->is_dragging_scroll = false;
        }
        if (state->negative_traits_selection_panel.is_dragging_scroll) {
          state->negative_traits_selection_panel.is_dragging_scroll = false;
          state->is_dragging_scroll = false;
        }
        if (state->chosen_traits_selection_panel.is_dragging_scroll) {
          state->chosen_traits_selection_panel.is_dragging_scroll = false;
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
    case MAIN_MENU_SCENE_UPGRADE: {
      break;
    }
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::smm_update_mouse_bindings()::Unsupported stage");
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
      if (IsKeyReleased(KEY_ESCAPE)) {
        event_fire(EVENT_CODE_TOGGLE_GAME_PAUSE, event_context());
      }
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
    case MAIN_MENU_SCENE_UPGRADE: {
      break;
    }
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::smm_update_keyboard_bindings()::Unsupported stage");
      break;
    }
  }
}
void smm_update_bindings(void) {
  smm_update_mouse_bindings();
  smm_update_keyboard_bindings();
}
void smm_update_local_buttons(void) {
  for (size_t itr_000 = 0; itr_000 < state->general_purpose_buttons.size(); ++itr_000) {
    local_button * const _btn = __builtin_addressof(state->general_purpose_buttons.at(itr_000));
    if (!_btn->on_screen) continue;

    if (CheckCollisionPointRec(*state->mouse_pos_screen, _btn->dest)) {
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        _btn->current_state = BTN_STATE_PRESSED;
      } else {
        if (_btn->current_state == BTN_STATE_PRESSED) { 
          _btn->current_state = BTN_STATE_RELEASED;
        }
        else if (_btn->current_state != BTN_STATE_HOVER) {
          _btn->current_state = BTN_STATE_HOVER;
        }
      }
    } else {
      _btn->current_state = BTN_STATE_UP;
    }
    _btn->on_screen = false;
  }
}

void smm_begin_fadeout(data128 data, void(*on_change_complete)(data128)) {
  if (!state || state == nullptr ) {
    TraceLog(LOG_ERROR, "scene_main_menu::smm_begin_fadeout()::State is not valid");
    return;
  }
  state->smm_fade = ui_fade_control_system();

  state->smm_fade.fade_animation_duration = MAIN_MENU_FADE_DURATION;
  state->smm_fade.fade_type = FADE_TYPE_FADEOUT;
  state->smm_fade.fade_animation_timer = 0.f;
  state->smm_fade.fade_animation_playing = true;
  state->smm_fade.is_fade_animation_played = false;
  state->smm_fade.data = data;
  state->smm_fade.on_change_complete = on_change_complete;
}
void smm_begin_fadein(data128 data, void(*on_change_complete)(data128)) {
  if (!state || state == nullptr ) {
    TraceLog(LOG_ERROR, "scene_main_menu::smm_begin_fadein()::State is not valid");
    return;
  }
  state->smm_fade.fade_animation_duration = MAIN_MENU_FADE_DURATION;
  state->smm_fade.fade_type = FADE_TYPE_FADEIN;
  state->smm_fade.fade_animation_timer = 0.f;
  state->smm_fade.fade_animation_playing = true;
  state->smm_fade.is_fade_animation_played = false;
  state->smm_fade.data = data;
  state->smm_fade.on_change_complete = on_change_complete;
}
local_button* smm_add_local_button(i32 _id, button_type_id _btn_type_id, button_state signal_state) {
  if (!state || state == nullptr) {
    TraceLog(LOG_ERROR, "scene_main_menu::smm_add_local_button()::State is not valid");
    return nullptr;
  }
  for (size_t itr_000 = 0; itr_000 < state->general_purpose_buttons.size(); ++itr_000) {
    const local_button * const _btn = __builtin_addressof(state->general_purpose_buttons.at(itr_000));
    if (_btn->id == _id) {
      TraceLog(LOG_WARNING, "scene_main_menu::smm_add_local_button()::Button ids overlapping");
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
local_button* smm_get_local_button(i32 _id) {
  if (!state || state == nullptr) {
    TraceLog(LOG_ERROR, "scene_main_menu::smm_get_local_button()::State is not valid");
    return nullptr;
  }
  for (size_t itr_000 = 0; itr_000 < state->general_purpose_buttons.size(); ++itr_000) {
    local_button * const _btn = __builtin_addressof(state->general_purpose_buttons.at(itr_000));
    if (_btn->id == _id) {
      return _btn;
    }
  }
  return nullptr;
}
void fade_on_complete_change_main_menu_type(data128 data) {
  if (!state || state == nullptr ) {
    TraceLog(LOG_INFO, "scene_main_menu::fade_on_complete_change_main_menu_type()::State is not valid");
    return;
  }
  main_menu_scene_type scene = static_cast<main_menu_scene_type>(data.i32[0]);
  begin_scene_change(scene, event_context(data.i32[1] ,data.i32[2] ,data.i32[3]));

  smm_begin_fadein(data128(), nullptr);
}
void fade_on_complete_change_scene(data128 data) {
  if (!state || state == nullptr ) {
    TraceLog(LOG_ERROR, "scene_main_menu::fade_on_complete_change_scene()::State is not valid");
    return;
  }
  scene_id scene = static_cast<scene_id>(data.i32[0]);
  state->smm_fade = ui_fade_control_system();
  begin_scene_change(scene, event_context(data.i32[1] ,data.i32[2] ,data.i32[3]));
}

void begin_scene_change(main_menu_scene_type mms, [[__maybe_unused__]] event_context context) {
  switch (mms) {
    case MAIN_MENU_SCENE_DEFAULT: {
      set_worldmap_location(WORLDMAP_MAINMENU_MAP);
      state->mainmenu_state = MAIN_MENU_SCENE_DEFAULT;
      break;
    }
    case MAIN_MENU_SCENE_SETTINGS: {
      state->mainmenu_state = MAIN_MENU_SCENE_SETTINGS;
      break;
    }
    case MAIN_MENU_SCENE_UPGRADE: {
      state->mainmenu_state = MAIN_MENU_SCENE_UPGRADE;
      break;
    }
    case MAIN_MENU_SCENE_EXTRAS: {
      state->mainmenu_state = MAIN_MENU_SCENE_EXTRAS;
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE: {
      state->mainmenu_state = MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE;
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE: {
      state->positive_traits.clear();
      state->negative_traits.clear();
      state->ingame_info->chosen_traits->clear();
      state->general_purpose_buttons.clear();
      const std::vector<character_trait> * const gm_traits = gm_get_character_traits();
      for (size_t itr_000 = 0; itr_000 < gm_traits->size(); ++itr_000) {
        character_trait _trait = gm_traits->at(itr_000);
      
        _trait.ui_use.i32[0] = state->next_local_button_id;
        local_button * lcl_btn_ptr = smm_add_local_button(state->next_local_button_id++, BTN_TYPE_FLAT_BUTTON, BTN_STATE_RELEASED);
        if (_trait.point > 0) {
          lcl_btn_ptr->btn_type.forground_color_btn_state_up = GREEN;
          lcl_btn_ptr->btn_type.forground_color_btn_state_hover = GREEN;
          lcl_btn_ptr->btn_type.forground_color_btn_state_pressed = GREEN;
          state->positive_traits.push_back(_trait);
        }
        else {
          lcl_btn_ptr->btn_type.forground_color_btn_state_up = RED;
          lcl_btn_ptr->btn_type.forground_color_btn_state_hover = RED;
          lcl_btn_ptr->btn_type.forground_color_btn_state_pressed = RED;
          state->negative_traits.push_back(_trait);
        }
      }
      state->mainmenu_state = MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE;
      break;
    }
    default: {
      TraceLog(LOG_WARNING, "scene_main_menu::begin_scene_change()::Unsupported main menu scene type");
      state->mainmenu_state = MAIN_MENU_SCENE_DEFAULT;
      break;
    }
  }
}
void begin_scene_change(scene_id sid, event_context context) {
  if (!state || state == nullptr ) {
    TraceLog(LOG_ERROR, "scene_main_menu::begin_scene_change()::State is not valid");
    return;
  }

  switch (sid) {
    case SCENE_TYPE_MAIN_MENU: {
      event_fire(EVENT_CODE_SCENE_MAIN_MENU, context);
      return; 
    }
    case SCENE_TYPE_IN_GAME: {
      event_fire(EVENT_CODE_SCENE_IN_GAME, context);
      return; 
    }
    case SCENE_TYPE_EDITOR: {
      event_fire(EVENT_CODE_SCENE_EDITOR, context);
      return; 
    }
    default: {
      TraceLog(LOG_WARNING, "scene_main_menu::begin_scene_change()::Unsupported scene type");
      event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context(static_cast<i32>(true)));
      return;
    }
  }

  TraceLog(LOG_ERROR, "scene_main_menu::begin_scene_change()::Function ended unexpectedly");
}

void positive_trait_button_on_click(size_t index) {
  character_trait * _trait_ptr = __builtin_addressof(state->positive_traits.at(index));;

  state->ingame_info->chosen_traits->push_back(*_trait_ptr);
  state->positive_traits.erase(state->positive_traits.begin() + index);
}
void negative_trait_button_on_click(size_t index) {
  character_trait * _trait_ptr = __builtin_addressof(state->negative_traits.at(index));;

  state->ingame_info->chosen_traits->push_back(*_trait_ptr);
  state->negative_traits.erase(state->negative_traits.begin() + index);
}
void chosen_trait_button_on_click(size_t index) {
  character_trait * _trait_ptr = __builtin_addressof(state->ingame_info->chosen_traits->at(index));

  if (_trait_ptr->point < 0) {
    state->negative_traits.push_back(*_trait_ptr);
  }
  else {
    state->positive_traits.push_back(*_trait_ptr);
  }
  state->ingame_info->chosen_traits->erase(state->ingame_info->chosen_traits->begin() + index);
}
