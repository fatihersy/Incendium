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
  panel trait_selection_panel;

  const character_stat * hovered_stat;
  const camera_metrics * in_camera_metrics;
  const app_settings   * in_app_settings;
  std::array<worldmap_stage, MAX_WORLDMAP_LOCATIONS> worldmap_locations;
  
  const Vector2* mouse_pos_screen;
  u32 deny_notify_timer;
  main_menu_scene_type mainmenu_state;

  play_scene_info ingame_scene_feed;
  ui_fade_control_system smm_fade;

  main_menu_scene_state(void) {
    this->upgrade_list_panel = panel();
    this->upgrade_details_panel = panel();
    this->hovered_stat = nullptr;
    this->in_camera_metrics = nullptr;
    this->in_app_settings = nullptr;
    this->mouse_pos_screen = nullptr;
    this->deny_notify_timer = 0u;
    this->mainmenu_state = MAIN_MENU_SCENE_DEFAULT;
    this->ingame_scene_feed = play_scene_info();
    this->smm_fade = ui_fade_control_system();
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

bool begin_scene_main_menu(void);
void draw_main_menu_upgrade_panel(void);
void draw_main_menu_upgrade_list_panel(void);
void draw_main_menu_upgrade_details_panel(void);
void smm_update_bindings(void);
void smm_update_keyboard_bindings(void);
void smm_update_mouse_bindings(void);
void smm_begin_fadeout(data64 data, void(*on_change_complete)(data64));
void smm_begin_fadein(data64 data, void(*on_change_complete)(data64));

void fade_on_complete_change_main_menu_type(data64 data);
void fade_on_complete_change_scene(data64 data);

bool initialize_scene_main_menu(const app_settings * _in_app_settings) {
  if (state) {
    return begin_scene_main_menu();
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

  if(!create_camera(
    state->in_app_settings->render_width_div2, state->in_app_settings->render_height_div2,
    state->in_app_settings->render_width, state->in_app_settings->render_height
  )) {
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
  
  return begin_scene_main_menu();
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
  else if (state->smm_fade.is_fade_animation_played) {
    if (state->smm_fade.fade_type == FADE_TYPE_FADEOUT) {
      state->smm_fade.on_change_complete(state->smm_fade.data);
      smm_begin_fadein(data64(), nullptr);
    }
    else if(state->smm_fade.fade_type == FADE_TYPE_FADEIN) {
      state->smm_fade = ui_fade_control_system();
    }
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

      if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->ingame_scene_feed.hovered_stage <= MAX_WORLDMAP_LOCATIONS) {
        set_worldmap_location(state->ingame_scene_feed.hovered_stage);
        state->mainmenu_state = MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE;
      }
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE: {
      event_fire(EVENT_CODE_CAMERA_SET_ZOOM, event_context(1.f));
      event_fire(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, event_context(0.f, 0.f));
      event_fire(EVENT_CODE_CAMERA_SET_OFFSET, event_context(state->in_app_settings->render_width * .5f,state->in_app_settings->render_height * .5f));
      
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
      gui_draw_texture_id(TEX_ID_WORLDMAP_WO_CLOUDS, Rectangle {0, 0, static_cast<f32>(SMM_BASE_RENDER_WIDTH), static_cast<f32>(SMM_BASE_RENDER_HEIGHT)});
      for (int i=0; i<MAX_WORLDMAP_LOCATIONS; ++i) {
        if (state->worldmap_locations.at(i).is_active) {
          Vector2 scrloc = Vector2 {
            state->worldmap_locations.at(i).screen_location.x * SMM_BASE_RENDER_WIDTH, 
            state->worldmap_locations.at(i).screen_location.y * SMM_BASE_RENDER_HEIGHT
          };
          gui_draw_map_stage_pin(state->ingame_scene_feed.hovered_stage == i, scrloc);
        }
      }
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE: {
      render_map();
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
    default: { break; }
  }
  
  EndMode2D();
}

void render_interface_main_menu(void) {
  if (state->mainmenu_state == MAIN_MENU_SCENE_DEFAULT) {
      
    gui_label_shader(GAME_TITLE, SHADER_ID_FONT_OUTLINE, FONT_TYPE_ABRACADABRA, 5, VECTOR2(SMM_BASE_RENDER_WIDTH * .5f, SMM_BASE_RENDER_HEIGHT * .25f), WHITE, true, true);
    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_PLAY), BTN_ID_MAINMENU_BUTTON_PLAY, VECTOR2(0.f, -21.f), SMM_BASE_RENDER_DIV2, true)) {
      smm_begin_fadeout(data64(MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE), fade_on_complete_change_main_menu_type);
    }
    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_UPGRADE), BTN_ID_MAINMENU_BUTTON_UPGRADE, VECTOR2(0.f, -10.5f), SMM_BASE_RENDER_DIV2, true)) {
      state->mainmenu_state = MAIN_MENU_SCENE_UPGRADE;
    }
    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_SETTINGS), BTN_ID_MAINMENU_BUTTON_SETTINGS, VECTOR2(0.f, 0.f), SMM_BASE_RENDER_DIV2, true)) {
      ui_refresh_setting_sliders_to_default();
      state->mainmenu_state = MAIN_MENU_SCENE_SETTINGS;
    }
    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_EDITOR), BTN_ID_MAINMENU_BUTTON_EDITOR, VECTOR2(0.f, 10.5f), SMM_BASE_RENDER_DIV2, true)) {
      smm_begin_fadeout(data64(SCENE_TYPE_EDITOR), fade_on_complete_change_scene);
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
          panel* pnl = __builtin_addressof(state->worldmap_selection_panel);
          Rectangle scrloc = Rectangle{
            state->worldmap_locations.at(itr_000).screen_location.x * SMM_BASE_RENDER_WIDTH - WORLDMAP_LOC_PIN_SIZE_DIV2, 
            state->worldmap_locations.at(itr_000).screen_location.y * SMM_BASE_RENDER_HEIGHT - WORLDMAP_LOC_PIN_SIZE_DIV2,
            WORLDMAP_LOC_PIN_SIZE, WORLDMAP_LOC_PIN_SIZE
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
  }
  else if(state->mainmenu_state == MAIN_MENU_SCENE_TO_PLAY_TRAIT_CHOICE) {
    gui_panel(state->trait_selection_panel, Rectangle {
      static_cast<f32>(SMM_BASE_RENDER_WIDTH) * .5f, static_cast<f32>(SMM_BASE_RENDER_HEIGHT) * .5f, 
      static_cast<f32>(SMM_BASE_RENDER_WIDTH) * .75f, static_cast<f32>(SMM_BASE_RENDER_HEIGHT) * .75f}, true
    );
  }
  render_user_interface();
}

[[__nodiscard__]] bool begin_scene_main_menu(void) {
  if (!user_interface_system_initialize()) {
    TraceLog(LOG_ERROR, "scene_main_menu::begin_scene_main_menu()::User interface failed to initialize!");
    return false;
  }

  if (!game_manager_initialize( state->in_camera_metrics, state->in_app_settings, get_active_map_ptr())) { // Inits player & spawns
    TraceLog(LOG_ERROR, "scene_in_game::begin_scene_main_menu()::game_manager_initialize() failed");
    return false;
  }
  set_worldmap_location(WORLDMAP_MAINMENU_MAP); // NOTE: Worldmap index 0 is mainmenu background now

  copy_memory(state->worldmap_locations.data(), get_worldmap_locations(), MAX_WORLDMAP_LOCATIONS * sizeof(worldmap_stage));

  state->mainmenu_state = MAIN_MENU_SCENE_DEFAULT;
  state->default_panel = panel( BTN_STATE_RELEASED, ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG, ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL, 
    Vector4 {6, 6, 6, 6}, Color { 30, 39, 46, 245}, Color { 52, 64, 76, 245}
  );
  state->upgrade_list_panel = panel();
  state->upgrade_details_panel = panel();
  state->upgrade_list_panel.dest = Rectangle{ SMM_BASE_RENDER_WIDTH * .025f, SMM_BASE_RENDER_HEIGHT * .075f, SMM_BASE_RENDER_WIDTH * .65f, SMM_BASE_RENDER_HEIGHT * .85f};
  state->upgrade_details_panel.dest = Rectangle{
    state->upgrade_list_panel.dest.x + state->upgrade_list_panel.dest.width + SMM_BASE_RENDER_WIDTH * .005f,
    SMM_BASE_RENDER_HEIGHT * .075f,
    SMM_BASE_RENDER_WIDTH * .295f, SMM_BASE_RENDER_HEIGHT * .850f
  };  

  state->trait_selection_panel = state->default_panel;
  state->worldmap_selection_panel = state->default_panel;
  gm_save_game();
  //event_fire(EVENT_CODE_PLAY_MAIN_MENU_THEME, event_context{}); TODO: Uncomment later
  return true;
}
void end_scene_main_menu(void) {
  event_fire(EVENT_CODE_RESET_MUSIC, event_context((i32)MUSIC_ID_MAIN_MENU_THEME));
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
      Rectangle header_tex_src_rect = get_atlas_texture_source_rect(ATLAS_TEX_ID_LITTLE_SHOWCASE);
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
      Rectangle tier_symbol_src_rect = get_atlas_texture_source_rect(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR);
      f32 star_spacing = tier_symbol_src_rect.width * 1.25f;
      f32 tier_symbols_total_width = tier_symbol_src_rect.width + (MAX_PASSIVE_UPGRADE_TIER - 1.f) * star_spacing;
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

  Rectangle tier_symbol_src_rect = get_atlas_texture_source_rect(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR);
  f32 star_spacing = tier_symbol_src_rect.width * 1.25f;
  f32 tier_symbols_total_width = tier_symbol_src_rect.width + (MAX_PASSIVE_UPGRADE_TIER - 1.f) * star_spacing;
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
    tier_symbols_vertical_position + tier_symbol_src_rect.height + detail_panel_element_spacing * .5f
  );
  gui_label(lc_txt(state->hovered_stat->passive_display_name_symbol), FONT_TYPE_ABRACADABRA, 1, title_pos, WHITE, true, true);

  Rectangle description_pos = {
    state->upgrade_details_panel.dest.x +state->upgrade_details_panel.dest.width * .05f, title_pos.y + detail_panel_element_spacing * .75f,
    state->upgrade_details_panel.dest.width * .9f, state->upgrade_details_panel.dest.width * .35f
  };
  gui_label_wrap(lc_txt(state->hovered_stat->passive_desc_symbol), FONT_TYPE_ABRACADABRA, 1, description_pos, WHITE, false);

  character_stat pseudo_update = *state->hovered_stat;
  upgrade_stat_pseudo(&pseudo_update);
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
      DRAW_UPGRADE_LABEL(upg_stat_text_pos, "%d -> %d", state->hovered_stat->buffer.u32[0], pseudo_update.buffer.u32[0]) break;
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
      upgrade_static_player_stat(state->hovered_stat->id);
      gm_save_game();
      event_fire(EVENT_CODE_PLAY_BUTTON_ON_CLICK, event_context((u16)true));
    } else {
      event_fire(EVENT_CODE_PLAY_SOUND, event_context((i32)SOUND_ID_DENY));
      state->deny_notify_timer = DENY_NOTIFY_TIME;
    }
  }
}

void smm_update_mouse_bindings(void) { 
  switch (state->mainmenu_state) {
    case MAIN_MENU_SCENE_DEFAULT: {
      break;
    }
    case MAIN_MENU_SCENE_TO_PLAY_MAP_CHOICE: {
      state->ingame_scene_feed.hovered_stage = U16_MAX;
      for (int i=0; i<MAX_WORLDMAP_LOCATIONS; ++i) {
        Rectangle scrloc = Rectangle{
          state->worldmap_locations.at(i).screen_location.x * SMM_BASE_RENDER_WIDTH - WORLDMAP_LOC_PIN_SIZE_DIV2, 
          state->worldmap_locations.at(i).screen_location.y * SMM_BASE_RENDER_HEIGHT - WORLDMAP_LOC_PIN_SIZE_DIV2,
          WORLDMAP_LOC_PIN_SIZE, WORLDMAP_LOC_PIN_SIZE
        };
        if (CheckCollisionPointRec( *state->mouse_pos_screen, scrloc)) {
          state->ingame_scene_feed.hovered_stage = i;
        }
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
      if (IsKeyReleased(KEY_R)) {
        state->trait_selection_panel.buffer.u16[0] = 0;
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
      TraceLog(LOG_WARNING, "scene_in_game::smm_update_keyboard_bindings()::Unsupported stage");
      break;
    }
  }
}
void smm_update_bindings(void) {
  smm_update_mouse_bindings();
  smm_update_keyboard_bindings();
}

void smm_begin_fadeout(data64 data, void(*on_change_complete)(data64)) {
  state->smm_fade.fade_animation_duration = MAIN_MENU_FADE_DURATION;
  state->smm_fade.fade_type = FADE_TYPE_FADEOUT;
  state->smm_fade.fade_animation_timer = 0.f;
  state->smm_fade.fade_animation_playing = true;
  state->smm_fade.is_fade_animation_played = false;
  state->smm_fade.data = data;
  state->smm_fade.on_change_complete = on_change_complete;
}
void smm_begin_fadein(data64 data, void(*on_change_complete)(data64)) {
  state->smm_fade.fade_animation_duration = MAIN_MENU_FADE_DURATION;
  state->smm_fade.fade_type = FADE_TYPE_FADEIN;
  state->smm_fade.fade_animation_timer = 0.f;
  state->smm_fade.fade_animation_playing = true;
  state->smm_fade.is_fade_animation_played = false;
  state->smm_fade.data = data;
  state->smm_fade.on_change_complete = on_change_complete;
}
void fade_on_complete_change_main_menu_type(data64 data) {
  state->mainmenu_state = static_cast<main_menu_scene_type>(data.i32[0]);
}
void fade_on_complete_change_scene(data64 data) {
  scene_id scene = static_cast<scene_id>(data.i32[0]);
  switch (scene) {
    case SCENE_TYPE_MAIN_MENU: {
      event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context());
      return;
    }
    case SCENE_TYPE_IN_GAME: {
      event_fire(EVENT_CODE_SCENE_IN_GAME, event_context());
      return;
    }
    case SCENE_TYPE_EDITOR: {
      event_fire(EVENT_CODE_SCENE_EDITOR, event_context());
      return;
    }
    default: {
      TraceLog(LOG_WARNING, "scene_main_menu::fade_on_complete_change_scene()::Unsupported scene id");
      return;
    }

    TraceLog(LOG_ERROR, "scene_main_menu::fade_on_complete_change_scene()::Function ended unexpectedly");
  }
}
