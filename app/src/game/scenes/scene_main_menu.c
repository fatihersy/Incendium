#include "scene_main_menu.h"

#include "core/event.h"
#include "core/fmemory.h"

#include "defines.h"
#include "game/world.h"
#include "game/game_manager.h"
#include "game/user_interface.h"
#include "raylib.h"

typedef enum main_menu_scene_type {
  MAIN_MENU_SCENE_DEFAULT,
  MAIN_MENU_SCENE_SETTINGS,
  MAIN_MENU_SCENE_UPGRADE,
  MAIN_MENU_SCENE_EXTRAS,
} main_menu_scene_type;

typedef struct main_menu_scene_state {
  main_menu_scene_type type;
  scene_id next_scene;
  panel upgrade_panel;
  Vector2 mouse_pos;
  camera_metrics* in_camera_metrics;

  bool in_scene_changing_process;
  bool scene_changing_process_complete;
} main_menu_scene_state;

static main_menu_scene_state *state;

#define MAIN_MENU_FADE_DURATION 1 * TARGET_FPS
#define MAIN_MENU_UPGRADE_PANEL_COL 5
#define MAIN_MENU_UPGRADE_PANEL_ROW 3

void begin_scene_main_menu(void);
void draw_main_menu_upgrade_panel(void);

void initialize_scene_main_menu(camera_metrics* _camera_metrics) {
  if (state) {
    begin_scene_main_menu();
    return;
  }
  state = (main_menu_scene_state*)allocate_memory_linear(sizeof(main_menu_scene_state), true);
  state->in_camera_metrics = _camera_metrics;

  begin_scene_main_menu();
}

void update_scene_main_menu(void) {
  update_user_interface();

  event_fire(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, (event_context) {
    .data.f32[0] = 0,
    .data.f32[1] = 0,
  });
  if (state->in_scene_changing_process && is_ui_fade_anim_about_to_complete()) {
    state->scene_changing_process_complete = true;
  }
  if (state->in_scene_changing_process && is_ui_fade_anim_complete()) {
    switch (state->next_scene) {
      case SCENE_TYPE_UNSPECIFIED: break;
      case SCENE_TYPE_IN_GAME: {
        event_fire(EVENT_CODE_SCENE_IN_GAME, (event_context){0}); break;
      }
      case SCENE_TYPE_EDITOR:{ 
        event_fire(EVENT_CODE_SCENE_EDITOR, (event_context){0}); break;
      }
      default: {
        TraceLog(LOG_ERROR, "scene_main_menu::update_scene_main_menu::Unknown scene");
        break;
      }
    }
  }
}

void render_scene_main_menu(void) {
  if (!state->scene_changing_process_complete) {
    render_map();
  }
}

void render_interface_main_menu(void) {
  if (!state->scene_changing_process_complete) {
    if (state->type == MAIN_MENU_SCENE_DEFAULT) {
      gui_label(GAME_TITLE, FONT_TYPE_MOOD, 65, VECTOR2(BASE_RENDER_SCALE(.5f).x, BASE_RENDER_SCALE(.25f).y), WHITE, true, true);

      if (gui_menu_button("Play", BTN_ID_MAINMENU_BUTTON_PLAY, VECTOR2(0,  0))) {
        state->in_scene_changing_process = true;
        state->next_scene = SCENE_TYPE_IN_GAME;
        event_fire(EVENT_CODE_UI_START_FADEOUT_EFFECT, (event_context){ .data.u16[0] = MAIN_MENU_FADE_DURATION});
      }
      if (gui_menu_button("Upgrade", BTN_ID_MAINMENU_BUTTON_UPGRADE, VECTOR2(0,  4))) {
        state->type = MAIN_MENU_SCENE_UPGRADE;
      }
      if (gui_menu_button("Settings", BTN_ID_MAINMENU_BUTTON_SETTINGS, VECTOR2(0,  8))) {
        state->type = MAIN_MENU_SCENE_SETTINGS;
        event_fire(EVENT_CODE_UI_SHOW_SETTINGS_MENU, (event_context) {0});
      }
      if (gui_menu_button("Editor", BTN_ID_MAINMENU_BUTTON_EDITOR, VECTOR2(0,  12))) {
        state->in_scene_changing_process = true;
        state->next_scene = SCENE_TYPE_EDITOR;
        event_fire(EVENT_CODE_UI_START_FADEOUT_EFFECT, (event_context){ .data.u16[0] = MAIN_MENU_FADE_DURATION});
      }
      if (gui_menu_button("Exit", BTN_ID_MAINMENU_BUTTON_EXIT, VECTOR2(0,  16))) {
        event_fire(EVENT_CODE_APPLICATION_QUIT, (event_context){0});
      }
    }
    else if (state->type == MAIN_MENU_SCENE_SETTINGS) {
      if(gui_menu_button("Cancel", BTN_ID_MAINMENU_SETTINGS_CANCEL, VECTOR2(2,  15))) {
        state->type = MAIN_MENU_SCENE_DEFAULT;
        event_fire(EVENT_CODE_UI_SHOW_SETTINGS_MENU, (event_context) {0});
      }
    }
    else if (state->type == MAIN_MENU_SCENE_UPGRADE) {
      draw_main_menu_upgrade_panel();
      if(gui_menu_button("Back", BTN_ID_MAINMENU_UPGRADE_BACK, VECTOR2(-2,  15))) {
        state->type = MAIN_MENU_SCENE_DEFAULT;
      }
    }
    render_user_interface();
  }
}

void begin_scene_main_menu(void) {
  if (!game_manager_initialize(state->in_camera_metrics)) { // Inits player & spawns
    TraceLog(LOG_ERROR, "scene_in_game::initialize_scene_in_game()::game_manager_initialize() failed");
    return;
  }

  user_interface_system_initialize();

  set_worldmap_location(WORLDMAP_MAINMENU_MAP); // NOTE: Worldmap index 0 is mainmenu background now 

  state->type = MAIN_MENU_SCENE_DEFAULT;
  state->next_scene = SCENE_TYPE_UNSPECIFIED;
  state->in_scene_changing_process = false;
  state->scene_changing_process_complete = false;
  state->upgrade_panel = get_default_panel();
  state->upgrade_panel.dest = (Rectangle) { 
    BASE_RENDER_SCALE(.5f).x, BASE_RENDER_SCALE(.5f).y, 
    BASE_RENDER_SCALE(.85f).x, BASE_RENDER_SCALE(.85f).y
  }; 

  event_fire(EVENT_CODE_PLAY_MAIN_MENU_THEME, (event_context) {0});
}
void end_scene_main_menu(void) {
  event_fire(EVENT_CODE_RESET_MUSIC, (event_context){ .data.i32[0] = MUSIC_ID_MAIN_MENU_THEME});

}

void draw_main_menu_upgrade_panel(void) {
  DrawRectangle(0, 0, BASE_RENDER_RES.x, BASE_RENDER_SCALE(.1f).y, (Color) {0, 0, 0, 50});
  gui_panel(state->upgrade_panel, state->upgrade_panel.dest, true);
  gui_label_format(
    FONT_TYPE_MOOD, 25, BASE_RENDER_SCALE(.5f).x, 
    0, WHITE, true, false, 
    "Souls:%d", get_currency_souls()
  );

  const Rectangle panel_dest = (Rectangle) { // Centered panel destination adjustment
    state->upgrade_panel.dest.x - state->upgrade_panel.dest.width / 2.f,
    state->upgrade_panel.dest.y - state->upgrade_panel.dest.height / 2.f,
    state->upgrade_panel.dest.width,
    state->upgrade_panel.dest.height
  };
  f32 showcase_hover_scale = 1.1f;
  f32 showcase_base_dim = BASE_RENDER_SCALE(.25f).y;
  const f32 showcase_spacing = showcase_base_dim * 1.1f;

  const f32 total_showcases_width = (MAIN_MENU_UPGRADE_PANEL_COL * showcase_base_dim) + ((MAIN_MENU_UPGRADE_PANEL_COL - 1) * (showcase_spacing - showcase_base_dim));
  const f32 total_showcases_height = (MAIN_MENU_UPGRADE_PANEL_ROW * showcase_base_dim) + ((MAIN_MENU_UPGRADE_PANEL_ROW - 1) * (showcase_spacing - showcase_base_dim));

  const Vector2 showcase_start_pos = VECTOR2(
    panel_dest.x + (panel_dest.width  / 2.f) - (total_showcases_width / 2.f),
    panel_dest.y + (panel_dest.height / 2.f) - (total_showcases_height / 2.f)
  );

  for (i32 i = 0; i < MAIN_MENU_UPGRADE_PANEL_ROW; ++i) {
    for (i32 j = 0; j < MAIN_MENU_UPGRADE_PANEL_COL; ++j) {
      const character_stat* stat = get_player_stat((MAIN_MENU_UPGRADE_PANEL_COL * i) + j + 1);
      if (!stat || stat->id >= CHARACTER_STATS_MAX || stat->id <= CHARACTER_STATS_UNDEFINED) {
        continue;
      }
      Vector2 showcase_position = VECTOR2(
        showcase_start_pos.x + j * showcase_spacing,
        showcase_start_pos.y + i * showcase_spacing
      );
      f32 showcase_new_dim = showcase_base_dim;
      if (CheckCollisionPointRec(*ui_get_mouse_pos(), (Rectangle) {showcase_position.x, showcase_position.y, showcase_base_dim, showcase_base_dim})) {
        showcase_new_dim *= showcase_hover_scale;
        showcase_position.x -= (showcase_new_dim - showcase_base_dim) / 2.f;
        showcase_position.y -= (showcase_new_dim - showcase_base_dim) / 2.f;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
          
        }
      }
      
      gui_draw_atlas_texture_id(ATLAS_TEX_ID_CRIMSON_FANTASY_SHOWCASE, (Rectangle){
        showcase_position.x, showcase_position.y, showcase_new_dim, showcase_new_dim
      });
      Rectangle tier_symbol_src_rect = get_atlas_texture_source_rect(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR);

      f32 star_spacing = tier_symbol_src_rect.width * 1.25f;
      f32 tier_symbols_total_width = tier_symbol_src_rect.width + (MAX_PASSIVE_UPGRADE_TIER - 1.f) * star_spacing;
      f32 tier_symbols_left_edge = showcase_position.x + (showcase_new_dim - tier_symbols_total_width) / 2.f;
      f32 tier_symbols_vertical_center = showcase_position.y + showcase_new_dim / 5.f;

      for (i32 i = 0; i < MAX_PASSIVE_UPGRADE_TIER; ++i) {
        Vector2 tier_pos = (Vector2) { tier_symbols_left_edge + i * star_spacing, tier_symbols_vertical_center };
        gui_draw_atlas_texture_id_scale(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR, tier_pos, 1.f, WHITE, false);
      }

      Rectangle icon_pos = (Rectangle) {
        showcase_position.x + showcase_new_dim * .25f, 
        showcase_position.y + showcase_new_dim * .25f,
        showcase_new_dim * .5f,
        showcase_new_dim * .5f
      };
      gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_ICON_ATLAS, stat->passive_icon_src, icon_pos, true);

      Vector2 title_pos = VECTOR2(
        showcase_position.x + showcase_new_dim / 2.f, 
        showcase_position.y + showcase_new_dim * 0.8f
      );
      gui_label(stat->passive_display_name, FONT_TYPE_MINI_MOOD, 8, title_pos, WHITE, true, true);
    }
  }
}
