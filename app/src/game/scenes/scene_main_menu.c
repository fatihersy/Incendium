#include "scene_main_menu.h"
#include <reasings.h>

#include "core/fmemory.h"
#include "core/event.h"

#include "game/game_manager.h"
#include "game/user_interface.h"
#include "game/world.h"

typedef enum main_menu_scene_type {
  MAIN_MENU_SCENE_DEFAULT,
  MAIN_MENU_SCENE_SETTINGS,
  MAIN_MENU_SCENE_UPGRADE,
  MAIN_MENU_SCENE_EXTRAS,
} main_menu_scene_type;

typedef struct main_menu_scene_state {
  panel upgrade_list_panel;
  panel upgrade_details_panel;

  character_stat *hovered_stat;
  camera_metrics *in_camera_metrics;
  
  Vector2 mouse_pos;
  u32 deny_notify_timer;
  main_menu_scene_type type;
  scene_id next_scene;
  bool in_scene_changing_process;
  bool scene_changing_process_complete;
} main_menu_scene_state;

static main_menu_scene_state *restrict state;

#define DENY_NOTIFY_TIME .8f * TARGET_FPS

#define MAIN_MENU_FADE_DURATION 1 * TARGET_FPS
#define MAIN_MENU_UPGRADE_PANEL_COL 3
#define MAIN_MENU_UPGRADE_PANEL_ROW 3
#define draw_upgrade_label(VEC2, TEXT, PRE_UPG_VAL, POS_UPG_VAL) \
  gui_label_format(FONT_TYPE_MINI_MOOD, 10, VEC2.x, VEC2.y, WHITE, true, false, TEXT, PRE_UPG_VAL, POS_UPG_VAL);

void begin_scene_main_menu(void);
void draw_main_menu_upgrade_panel(void);
void draw_main_menu_upgrade_list_panel(void);
void draw_main_menu_upgrade_details_panel(void);
Rectangle smm_get_camera_view_rect(Camera2D camera);

void initialize_scene_main_menu(camera_metrics *_camera_metrics) {
  if (state) {
    begin_scene_main_menu();
    return;
  }
  state = (main_menu_scene_state *)allocate_memory_linear(sizeof(main_menu_scene_state), true);
  state->in_camera_metrics = _camera_metrics;

  begin_scene_main_menu();
}

void update_scene_main_menu(void) {
  update_user_interface();
  state->in_camera_metrics->frustum = smm_get_camera_view_rect(state->in_camera_metrics->handle);

  event_fire(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, (event_context) { .data.f32[0] = 0, .data.f32[1] = 0, });
  if (state->in_scene_changing_process && is_ui_fade_anim_about_to_complete()) {
    state->scene_changing_process_complete = true;
  }
  if (state->in_scene_changing_process && is_ui_fade_anim_complete()) {
    switch (state->next_scene) {
        case SCENE_TYPE_UNSPECIFIED:
        break;
      case SCENE_TYPE_IN_GAME: {
        event_fire(EVENT_CODE_SCENE_IN_GAME, (event_context){0});
        break;
      }
      case SCENE_TYPE_EDITOR: {
        event_fire(EVENT_CODE_SCENE_EDITOR, (event_context){0});
        break;
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

      if (gui_menu_button("Play", BTN_ID_MAINMENU_BUTTON_PLAY, VECTOR2(0, 0), 2.7f, true)) {
        state->in_scene_changing_process = true;
        state->next_scene = SCENE_TYPE_IN_GAME;
        event_fire(EVENT_CODE_UI_START_FADEOUT_EFFECT, (event_context){.data.u16[0] = MAIN_MENU_FADE_DURATION});
      }
      if (gui_menu_button("Upgrade", BTN_ID_MAINMENU_BUTTON_UPGRADE, VECTOR2(0, 4), 2.7f, true)) {
        state->type = MAIN_MENU_SCENE_UPGRADE;
      }
      if (gui_menu_button("Settings", BTN_ID_MAINMENU_BUTTON_SETTINGS, VECTOR2(0, 8), 2.7f, true)) {
        state->type = MAIN_MENU_SCENE_SETTINGS;
        event_fire(EVENT_CODE_UI_SHOW_SETTINGS_MENU, (event_context){0});
      }
      if (gui_menu_button("Editor", BTN_ID_MAINMENU_BUTTON_EDITOR, VECTOR2(0, 12), 2.7f, true)) {
        state->in_scene_changing_process = true;
        state->next_scene = SCENE_TYPE_EDITOR;
        event_fire(EVENT_CODE_UI_START_FADEOUT_EFFECT, (event_context){.data.u16[0] = MAIN_MENU_FADE_DURATION});
      }
      if (gui_menu_button("Exit", BTN_ID_MAINMENU_BUTTON_EXIT, VECTOR2(0, 16), 2.7f, true)) {
        event_fire(EVENT_CODE_APPLICATION_QUIT, (event_context){0});
      }
    } else if (state->type == MAIN_MENU_SCENE_SETTINGS) {
      if (gui_menu_button("Cancel", BTN_ID_MAINMENU_SETTINGS_CANCEL, VECTOR2(2, 15), 2.7f, true)) {
        state->type = MAIN_MENU_SCENE_DEFAULT;
        event_fire(EVENT_CODE_UI_SHOW_SETTINGS_MENU, (event_context){0});
      }
    } else if (state->type == MAIN_MENU_SCENE_UPGRADE) {
      draw_main_menu_upgrade_panel();
      if (gui_menu_button("Back", BTN_ID_MAINMENU_UPGRADE_BACK, VECTOR2(0, 21), 2.7f, true)) {
        state->type = MAIN_MENU_SCENE_DEFAULT;
      }
    }
    render_user_interface();
  }
}

void begin_scene_main_menu(void) {
  user_interface_system_initialize();  

  if (!game_manager_initialize( state->in_camera_metrics)) { // Inits player & spawns
    TraceLog(LOG_ERROR, "scene_in_game::initialize_scene_in_game()::game_manager_initialize() failed");
    return;
  }
  set_worldmap_location(WORLDMAP_MAINMENU_MAP); // NOTE: Worldmap index 0 is mainmenu background now

  state->type = MAIN_MENU_SCENE_DEFAULT;
  state->next_scene = SCENE_TYPE_UNSPECIFIED;
  state->in_scene_changing_process = false;
  state->scene_changing_process_complete = false;
  state->upgrade_list_panel = get_default_panel();
  state->upgrade_details_panel = get_default_panel();
  state->upgrade_list_panel.dest = (Rectangle){ BASE_RENDER_SCALE(.025f).x, BASE_RENDER_SCALE(.075f).y, BASE_RENDER_SCALE(.65f).x, BASE_RENDER_SCALE(.850f).y};
  state->upgrade_details_panel.dest = (Rectangle){
    state->upgrade_list_panel.dest.x + state->upgrade_list_panel.dest.width + BASE_RENDER_SCALE(.005f).x,
    BASE_RENDER_SCALE(.075f).y, 
    BASE_RENDER_SCALE(.295f).x, BASE_RENDER_SCALE(.850f).y
  };
  gm_save_game();

  event_fire(EVENT_CODE_PLAY_MAIN_MENU_THEME, (event_context){0});
}
void end_scene_main_menu(void) {
  event_fire(EVENT_CODE_RESET_MUSIC, (event_context){.data.i32[0] = MUSIC_ID_MAIN_MENU_THEME});
}

void draw_main_menu_upgrade_panel(void) {
  Rectangle header_loc = (Rectangle){0, 0, BASE_RENDER_RES.x, BASE_RENDER_SCALE(.1f).y};
  Rectangle footer_loc = (Rectangle){0, BASE_RENDER_RES.y - BASE_RENDER_SCALE(.1f).y, BASE_RENDER_RES.x, BASE_RENDER_SCALE(.1f).y};
  DrawRectangleRec(header_loc, (Color){0, 0, 0, 50});
  DrawRectangleRec(footer_loc, (Color){0, 0, 0, 50});
  
  atlas_texture_id icon_tex_id = ATLAS_TEX_ID_CURRENCY_SOUL_ICON_5000;
  f32 cost_icon_dim = header_loc.height;
  Vector2 cost_icon_pos = VECTOR2(BASE_RENDER_SCALE(.5f).x - cost_icon_dim * .3f, BASE_RENDER_SCALE(.05f).y);
  gui_draw_atlas_texture_id_center(icon_tex_id, cost_icon_pos, VECTOR2(cost_icon_dim, cost_icon_dim), true);

  Vector2 cost_label_pos = VECTOR2(BASE_RENDER_SCALE(.5f).x + cost_icon_dim * .3f, BASE_RENDER_SCALE(.05f).y);
  gui_label_format_v(FONT_TYPE_MOOD, 25, cost_label_pos, WHITE, true, true, "%d", get_currency_souls());

  draw_main_menu_upgrade_list_panel();
  draw_main_menu_upgrade_details_panel();
}

void draw_main_menu_upgrade_list_panel(void) {
  gui_panel(state->upgrade_list_panel, state->upgrade_list_panel.dest, false);
  f32 showcase_hover_scale = 1.1f;
  f32 showcase_base_dim = BASE_RENDER_SCALE(.25f).y;

  const f32 showcase_spacing = showcase_base_dim * 1.1f;
  const f32 total_showcases_width = (MAIN_MENU_UPGRADE_PANEL_COL * showcase_base_dim) + ((MAIN_MENU_UPGRADE_PANEL_COL - 1) * (showcase_spacing - showcase_base_dim));
  const f32 total_showcases_height = (MAIN_MENU_UPGRADE_PANEL_ROW * showcase_base_dim) + ((MAIN_MENU_UPGRADE_PANEL_ROW - 1) * (showcase_spacing - showcase_base_dim));
  const Vector2 showcase_start_pos = VECTOR2(
    state->upgrade_list_panel.dest.x + (state->upgrade_list_panel.dest.width / 2.f) - (total_showcases_width / 2.f),
    state->upgrade_list_panel.dest.y + (state->upgrade_list_panel.dest.height / 2.f) - (total_showcases_height / 2.f)
  );

  for (i32 i = 0; i < MAIN_MENU_UPGRADE_PANEL_ROW; ++i) {
    for (i32 j = 0; j < MAIN_MENU_UPGRADE_PANEL_COL; ++j) {
      character_stat *stat = get_player_stat((MAIN_MENU_UPGRADE_PANEL_COL * i) + j + 1);
      if (!stat || stat->id >= CHARACTER_STATS_MAX ||
        stat->id <= CHARACTER_STATS_UNDEFINED) {
        continue;
      }
      Vector2 showcase_position = VECTOR2(showcase_start_pos.x + j * showcase_spacing, showcase_start_pos.y + i * showcase_spacing);
      f32 showcase_new_dim = showcase_base_dim;
      if (CheckCollisionPointRec( *ui_get_mouse_pos(), (Rectangle){showcase_position.x, showcase_position.y, showcase_base_dim, showcase_base_dim})) {
        showcase_new_dim *= showcase_hover_scale;
        showcase_position.x -= (showcase_new_dim - showcase_base_dim) / 2.f;
        showcase_position.y -= (showcase_new_dim - showcase_base_dim) / 2.f;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
          state->hovered_stat = stat;
        }
      }

      gui_draw_atlas_texture_id(ATLAS_TEX_ID_CRIMSON_FANTASY_SHOWCASE,(Rectangle){showcase_position.x, showcase_position.y, showcase_new_dim, showcase_new_dim});
      Rectangle tier_symbol_src_rect = get_atlas_texture_source_rect(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR);
      f32 star_spacing = tier_symbol_src_rect.width * 1.25f;
      f32 tier_symbols_total_width = tier_symbol_src_rect.width + (MAX_PASSIVE_UPGRADE_TIER - 1.f) * star_spacing;
      f32 tier_symbols_left_edge = showcase_position.x + (showcase_new_dim - tier_symbols_total_width) / 2.f;
      f32 tier_symbols_vertical_center = showcase_position.y + showcase_new_dim / 5.f;

      for (i32 i = 0; i < MAX_PASSIVE_UPGRADE_TIER; ++i) {
        Vector2 tier_pos = (Vector2){tier_symbols_left_edge + i * star_spacing, tier_symbols_vertical_center};
        if (i < stat->level-1) {
          gui_draw_atlas_texture_id_scale(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR, tier_pos, 1.f, RED, false);
        } else {
          gui_draw_atlas_texture_id_scale(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR, tier_pos, 1.f, WHITE, false);
        }
      }

      Rectangle icon_pos = (Rectangle){
        showcase_position.x + showcase_new_dim * .25f, showcase_position.y + showcase_new_dim * .25f,
        showcase_new_dim * .5f, showcase_new_dim * .5f};
      gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_ICON_ATLAS, stat->passive_icon_src, icon_pos, true, false);

      Vector2 title_pos = VECTOR2(showcase_position.x + showcase_new_dim / 2.f, showcase_position.y + showcase_new_dim * 0.8f);
      gui_label(stat->passive_display_name, FONT_TYPE_MINI_MOOD, 8, title_pos, WHITE, true, true);
    }
  }
}
void draw_main_menu_upgrade_details_panel(void) {
  gui_panel(state->upgrade_details_panel, state->upgrade_details_panel.dest, false);
  if (!state->hovered_stat) {
    return;
  }
  f32 detail_panel_element_spacing = state->upgrade_details_panel.dest.height * 0.05f;

  Rectangle icon_pos = (Rectangle){
    state->upgrade_details_panel.dest.x + state->upgrade_details_panel.dest.width * .5f - (state->upgrade_details_panel.dest.width * .175f),
    state->upgrade_details_panel.dest.y + detail_panel_element_spacing,
    state->upgrade_details_panel.dest.width * .35f,
    state->upgrade_details_panel.dest.width * .35f
  };
  gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_ICON_ATLAS, state->hovered_stat->passive_icon_src, icon_pos, true, false);

  Rectangle tier_symbol_src_rect = get_atlas_texture_source_rect(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR);
  f32 star_spacing = tier_symbol_src_rect.width * 1.25f;
  f32 tier_symbols_total_width = tier_symbol_src_rect.width + (MAX_PASSIVE_UPGRADE_TIER - 1.f) * star_spacing;
  f32 tier_symbols_left_edge = state->upgrade_details_panel.dest.x + (state->upgrade_details_panel.dest.width - tier_symbols_total_width) / 2.f;
  f32 tier_symbols_vertical_position = icon_pos.y + icon_pos.height + detail_panel_element_spacing * .5f;
  for (i32 i = 0; i < MAX_PASSIVE_UPGRADE_TIER; ++i) {
    Vector2 tier_pos = (Vector2){tier_symbols_left_edge + i * star_spacing, tier_symbols_vertical_position};
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
  gui_label(state->hovered_stat->passive_display_name, FONT_TYPE_MINI_MOOD, 12, title_pos, WHITE, true, true);

  Rectangle description_pos = (Rectangle) {
    state->upgrade_details_panel.dest.x +state->upgrade_details_panel.dest.width * .05f, title_pos.y + detail_panel_element_spacing * .75f,
    state->upgrade_details_panel.dest.width * .9f, state->upgrade_details_panel.dest.width * .35f
  };
  gui_label_wrap(state->hovered_stat->passive_desc, FONT_TYPE_MINI_MOOD, 8, description_pos, WHITE, false);

  character_stat pseudo_update = *state->hovered_stat;
  upgrade_player_stat(&pseudo_update);
  Vector2 upg_stat_text_pos = {
    state->upgrade_details_panel.dest.x + state->upgrade_details_panel.dest.width * .5f,
    state->upgrade_details_panel.dest.y + state->upgrade_details_panel.dest.height * .5f,
  };

  switch (state->hovered_stat->id) {
    case CHARACTER_STATS_HEALTH: {
      draw_upgrade_label(upg_stat_text_pos, "%d -> %d", state->hovered_stat->buffer.u32[0], pseudo_update.buffer.u32[0]) break;
    }
    case CHARACTER_STATS_HP_REGEN: {
      draw_upgrade_label(upg_stat_text_pos, "%d -> %d", state->hovered_stat->buffer.u32[0], pseudo_update.buffer.u32[0]) break;
    }
    case CHARACTER_STATS_MOVE_SPEED: {
      draw_upgrade_label(upg_stat_text_pos, "%.1f -> %.1f", state->hovered_stat->buffer.f32[0], pseudo_update.buffer.f32[0]) break;
    }
    case CHARACTER_STATS_AOE: {
      draw_upgrade_label(upg_stat_text_pos, "%.1f -> %.1f", state->hovered_stat->buffer.f32[0], pseudo_update.buffer.f32[0]) break;
    }
    case CHARACTER_STATS_DAMAGE: {
      draw_upgrade_label(upg_stat_text_pos, "%d -> %d", state->hovered_stat->buffer.u32[0], pseudo_update.buffer.u32[0]) break;
    }
    case CHARACTER_STATS_ABILITY_CD: {
      draw_upgrade_label(upg_stat_text_pos, "%.1f -> %.1f", state->hovered_stat->buffer.f32[0], pseudo_update.buffer.f32[0]) break;
    }
    case CHARACTER_STATS_PROJECTILE_AMOUTH: {
      draw_upgrade_label(upg_stat_text_pos, "%d -> %d", state->hovered_stat->buffer.u16[0], pseudo_update.buffer.u16[0]) break;
    }
    case CHARACTER_STATS_EXP_GAIN: {
      draw_upgrade_label(upg_stat_text_pos, "%d -> %d", state->hovered_stat->buffer.u32[0], pseudo_update.buffer.u32[0]) break;
    }
    default: {
      TraceLog(LOG_ERROR, "game_manager::upgrade_player_stat()::Unsuppported stat id");
      break;
    }
  }

  atlas_texture_id icon_tex_id = ATLAS_TEX_ID_CURRENCY_SOUL_ICON_5000;
  f32 cost_icon_dim = state->upgrade_details_panel.dest.width * .25f;
  Vector2 cost_icon_pos = VECTOR2(
    state->upgrade_details_panel.dest.x + state->upgrade_details_panel.dest.width * .425f,
    state->upgrade_details_panel.dest.y + state->upgrade_details_panel.dest.height * .85f
  );
  gui_draw_atlas_texture_id_center(icon_tex_id, cost_icon_pos, VECTOR2(cost_icon_dim, cost_icon_dim), true);

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
  gui_label_format_v(FONT_TYPE_MOOD, 25, cost_label_pos, cost_label_color, true, true, "%d", state->hovered_stat->upgrade_cost);

  if (gui_menu_button("Upgrade", BTN_ID_MAINMENU_UPGRADE_BUY_UPGRADE, (Vector2){5.25f, 17.1f}, 2.7f, false)) {
    if (((i32)get_currency_souls() - state->hovered_stat->upgrade_cost) >= 0) {
      currency_souls_add(-state->hovered_stat->upgrade_cost);
      upgrade_player_stat(get_player_stat(state->hovered_stat->id));
      gm_save_game();
      event_fire(EVENT_CODE_PLAY_BUTTON_ON_CLICK, (event_context) { .data.u16[0] = true});
    } else {
      event_fire(EVENT_CODE_PLAY_SOUND, (event_context) { .data.i32[0] = SOUND_ID_DENY});
      state->deny_notify_timer = DENY_NOTIFY_TIME;
    }
  }
}

Rectangle smm_get_camera_view_rect(Camera2D camera) {

  f32 view_width = BASE_RENDER_RES.x / camera.zoom;
  f32 view_height = BASE_RENDER_RES.y / camera.zoom;

  f32 x = camera.target.x;
  f32 y = camera.target.y;
  
  x -= camera.offset.x/camera.zoom;
  y -= camera.offset.y/camera.zoom;
  
  return (Rectangle){ x, y, view_width, view_height };
}
