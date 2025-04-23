#include "scene_in_game.h"
#include <reasings.h>
#include <settings.h>

#include <core/fmath.h>
#include <core/event.h>
#include <core/fmemory.h>
#include <core/ftime.h>

#include "game/world.h"
#include "game/game_manager.h"
#include "game/user_interface.h"

typedef enum in_game_stages {
  IN_GAME_STAGE_UNDEFINED,
  IN_GAME_STAGE_MAP_CHOICE,
  IN_GAME_STAGE_PASSIVE_CHOICE,
  IN_GAME_STAGE_PLAY,
  IN_GAME_STAGE_PLAY_DEBUG,
  IN_GAME_STAGE_PLAY_RESULTS,
  IN_GAME_STAGE_MAX,
} in_game_stages;

typedef struct end_game_results{
  u16 collected_souls;
  f32 play_time;
  bool is_win;
}end_game_results;

typedef struct scene_in_game_state {
  worldmap_stage worldmap_locations[MAX_WORLDMAP_LOCATIONS];
  panel ability_upg_panels[MAX_UPDATE_ABILITY_PANEL_COUNT];
  panel passive_selection_panels[MAX_UPDATE_PASSIVE_PANEL_COUNT];
  panel worldmap_selection_panel;
  panel debug_info_panel;
  panel default_panel;
  
  camera_metrics* in_camera_metrics;
  
  in_game_stages stage;
  end_game_results end_game_result;
  u16 hovered_stage;
  u16 hovered_spawn;
  bool has_game_started;
} scene_in_game_state;

static scene_in_game_state *restrict state;

#define STATE_ASSERT(FUNCTION) if (!state) {                                              \
  TraceLog(LOG_ERROR, "scene_in_game::" FUNCTION "::In game state was not initialized");  \
  event_fire(EVENT_CODE_SCENE_MAIN_MENU, (event_context){0});                             \
  return;                                                                                 \
}
#define DRAW_ABL_UPG_STAT_PNL(ABL, UPG, TEXT, STAT){ \
if (UPG.level == 1) {\
  gui_label_format(FONT_TYPE_MINI_MOOD, upgr_font_size, start_upgradables_x_exis, upgradables_height_buffer, WHITE, false, false, TEXT"%d", UPG.base_damage);\
  upgradables_height_buffer += btw_space_gap;\
} else if(UPG.level>1 && UPG.level <= MAX_ABILITY_LEVEL) {\
  gui_label_format(FONT_TYPE_MINI_MOOD, upgr_font_size, start_upgradables_x_exis, upgradables_height_buffer, WHITE, false, false, TEXT"%d -> %d", ABL->base_damage, UPG.base_damage);\
  upgradables_height_buffer += btw_space_gap;\
}}

#define ABILITY_UPG_PANEL_ICON_SIZE BASE_RENDER_SCALE(.25f).x*.5f
#define PASSIVE_SELECTION_PANEL_ICON_SIZE ABILITY_UPG_PANEL_ICON_SIZE
#define CLOUDS_ANIMATION_DURATION TARGET_FPS * 1.5f // second

bool scene_in_game_on_event(u16 code, event_context context);
void begin_scene_in_game(void);
void in_game_update_bindings(void);
void in_game_update_mouse_bindings(void);
void in_game_update_keyboard_bindings(void);
void initialize_worldmap_locations(void);
void draw_in_game_upgrade_panel(ability* abl, ability upg, Rectangle panel_dest);
void draw_passive_selection_panel(character_stat* stat, Rectangle panel_dest);
void draw_end_game_panel();
void start_game(character_stat* stat);
void reset_game();
Rectangle sig_get_camera_view_rect(Camera2D camera);

/**
 * @brief Requires world system, world init moved to app, as well as its loading time
 */
bool initialize_scene_in_game(camera_metrics* _camera_metrics) {
  if (state) {
    begin_scene_in_game();
    return false;
  }
  state = (scene_in_game_state *)allocate_memory_linear(sizeof(scene_in_game_state), true);
  state->in_camera_metrics = _camera_metrics;

  if (!game_manager_initialize(_camera_metrics)) { // Inits player & spawns
    TraceLog(LOG_ERROR, "scene_in_game::initialize_scene_in_game()::game_manager_initialize() failed");
    return false;
  }

  event_register(EVENT_CODE_ADD_CURRENCY_SOULS, scene_in_game_on_event);

  begin_scene_in_game();
  return true;
}

void begin_scene_in_game(void) {
  
  state->default_panel = (panel) {
    .signal_state  = BTN_STATE_RELEASED,
    .bg_tex_id     = ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG,
    .frame_tex_id  = ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL,
    .bg_tint       = (Color) { 30, 39, 46, 245},
    .bg_hover_tint = (Color) { 52, 64, 76, 245},
    .offsets = (Vector4) {6, 6, 6, 6},
  };

  copy_memory(&state->worldmap_locations, get_worldmap_locations(), sizeof(state->worldmap_locations));
  _set_player_position(BASE_RENDER_SCALE(.5f));
  set_is_game_paused(true);
  
  for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
    state->ability_upg_panels[i] = state->default_panel;
  }
  for (int i=0; i<MAX_UPDATE_PASSIVE_PANEL_COUNT; ++i) {
    state->passive_selection_panels[i] = state->default_panel;
  }
  state->worldmap_selection_panel = state->default_panel;
  state->debug_info_panel = state->default_panel;

  state->stage = IN_GAME_STAGE_MAP_CHOICE;
  state->hovered_stage = U16_MAX;

  event_fire(EVENT_CODE_UI_START_FADEIN_EFFECT, (event_context){ .data.u16[0] = CLOUDS_ANIMATION_DURATION });
}
void start_game(character_stat* stat) {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_in_game::start_game()::state returned zero");
    return;
  }
  gm_start_game(*get_active_worldmap());

  upgrade_player_stat(stat);
  _set_player_position(BASE_RENDER_SCALE(.5f));
  state->stage = IN_GAME_STAGE_PLAY;
}
void end_scene_in_game(void) {
  gm_reset_game();

  state->stage = IN_GAME_STAGE_MAP_CHOICE;
  state->end_game_result.play_time = 0.f;
  state->hovered_stage = U16_MAX;
  state->has_game_started = false; 
  state->end_game_result = (end_game_results){0};
}

void update_scene_in_game(void) {
  STATE_ASSERT("update_scene_in_game")

  in_game_update_bindings();
  update_user_interface();

  switch (state->stage) {
    case IN_GAME_STAGE_MAP_CHOICE: {
      event_fire(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, (event_context) {
        .data.f32[0] = BASE_RENDER_SCALE(.5f).x,
        .data.f32[1] = BASE_RENDER_SCALE(.5f).y,
      });
      state->in_camera_metrics->frustum = sig_get_camera_view_rect(state->in_camera_metrics->handle);

      if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->hovered_stage <= MAX_WORLDMAP_LOCATIONS) {
        set_worldmap_location(state->hovered_stage);
        state->stage = IN_GAME_STAGE_PASSIVE_CHOICE;
      }
      break;
    }
    case IN_GAME_STAGE_PASSIVE_CHOICE: {
      break;
    }
    case IN_GAME_STAGE_PLAY: {
      if (get_is_game_paused() || !state->has_game_started) { return; }
      else if (get_remaining_enemies() <= 0 || get_remaining_enemies() > MAX_SPAWN_COUNT) {
        event_fire(EVENT_CODE_END_GAME, (event_context) {0});
        state->end_game_result.is_win = true;
      }
      if (get_is_game_end()) {
        state->stage = IN_GAME_STAGE_PLAY_RESULTS;
      }
      if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyReleased(KEY_D)) {
        state->stage = IN_GAME_STAGE_PLAY_DEBUG;
      }

      update_map();
      update_game_manager();
      state->end_game_result.play_time += GetFrameTime();
    
      event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, (event_context){
        .data.f32[0] = _get_player_position(true).x,
        .data.f32[1] = _get_player_position(true).y,
      });
      state->in_camera_metrics->frustum = sig_get_camera_view_rect(state->in_camera_metrics->handle);
      break;
    }
    case IN_GAME_STAGE_PLAY_DEBUG: {
      if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyReleased(KEY_D)) {
        state->stage = IN_GAME_STAGE_PLAY;
      }
      update_map();
      update_game_manager_debug();

      event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, (event_context){
        .data.f32[0] = _get_player_position(true).x,
        .data.f32[1] = _get_player_position(true).y,
      });
      state->in_camera_metrics->frustum = sig_get_camera_view_rect(state->in_camera_metrics->handle);
      break;
    }
    case IN_GAME_STAGE_PLAY_RESULTS: { break; }
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::update_scene_in_game()::Unsupported stage");
      break;
    }
  }
}
void in_game_update_mouse_bindings(void) { 
  switch (state->stage) {
    case IN_GAME_STAGE_MAP_CHOICE: {
      state->hovered_stage = U16_MAX;
      for (int i=0; i<MAX_WORLDMAP_LOCATIONS; ++i) {
        Rectangle scrloc = (Rectangle){
          state->worldmap_locations[i].screen_location.x * BASE_RENDER_RES.x - WORLDMAP_LOC_PIN_SIZE_DIV2, 
          state->worldmap_locations[i].screen_location.y * BASE_RENDER_RES.y - WORLDMAP_LOC_PIN_SIZE_DIV2,
          WORLDMAP_LOC_PIN_SIZE, WORLDMAP_LOC_PIN_SIZE
        };
        if (CheckCollisionPointRec(*ui_get_mouse_pos(), scrloc)) {
          state->hovered_stage = i;
        }
      }
      break;
    }
    case IN_GAME_STAGE_PASSIVE_CHOICE: {
      break;
    }
    case IN_GAME_STAGE_PLAY: {
      break;
    }
    case IN_GAME_STAGE_PLAY_DEBUG: {
      state->hovered_spawn = U16_MAX;
      for (int i=0; i<get_remaining_enemies(); ++i) {
        Character2D* spawn = get_spawn_info(i);
        if (spawn) {
          Vector2 mouse_pos_world = gm_get_mouse_pos_world();
          if (CheckCollisionPointRec(mouse_pos_world, spawn->collision)) {
            state->hovered_spawn = i;
          }
        }
      }
      break;
    }
    case IN_GAME_STAGE_PLAY_RESULTS: { break; }
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::update_scene_in_game()::Unsupported stage");
      break;
    }
  }
}
void in_game_update_keyboard_bindings(void) {

  switch (state->stage) {
    case IN_GAME_STAGE_MAP_CHOICE: {
      if (IsKeyReleased(KEY_ESCAPE)) {
        event_fire(EVENT_CODE_UI_SHOW_PAUSE_MENU, (event_context){0});
      }

      break;
    }
    case IN_GAME_STAGE_PASSIVE_CHOICE: {
      if (IsKeyReleased(KEY_R)) {
        for (int i=0; i<MAX_UPDATE_PASSIVE_PANEL_COUNT; ++i) {
          state->passive_selection_panels[i].buffer[0].data.u16[0] = 0;
        }
      }
      break;
    }
    case IN_GAME_STAGE_PLAY: {  
      if (!state->has_game_started && IsKeyPressed(KEY_SPACE)) {
        state->has_game_started = true;
        set_is_game_paused(false);
      }
      if (IsKeyReleased(KEY_ESCAPE)) {
        if(!get_b_player_have_upgrade_points()) toggle_is_game_paused();
        event_fire(EVENT_CODE_UI_SHOW_PAUSE_MENU, (event_context){0});
      }
      break;
    }
    case IN_GAME_STAGE_PLAY_DEBUG: {  
      if (!state->has_game_started) {
        state->stage = IN_GAME_STAGE_PLAY;
      }
      if (IsKeyReleased(KEY_ESCAPE)) {
        state->stage = IN_GAME_STAGE_PLAY;
      }
      break;
    }
    case IN_GAME_STAGE_PLAY_RESULTS: { break; }
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::in_game_update_keyboard_bindings()::Unsupported stage");
      break;
    }
  }
}
void in_game_update_bindings(void) {
  in_game_update_mouse_bindings();
  in_game_update_keyboard_bindings();
}

void render_scene_in_game(void) {
  STATE_ASSERT("render_scene_in_game")

  switch (state->stage) {
    case IN_GAME_STAGE_MAP_CHOICE: {
 
      gui_draw_texture_id(TEX_ID_WORLDMAP_WO_CLOUDS, (Rectangle) {0, 0, BASE_RENDER_RES.x, BASE_RENDER_RES.y});
      for (int i=0; i<MAX_WORLDMAP_LOCATIONS; ++i) {
        if (state->worldmap_locations[i].is_active) {
          Vector2 scrloc = (Vector2) {
            state->worldmap_locations[i].screen_location.x * BASE_RENDER_RES.x, 
            state->worldmap_locations[i].screen_location.y * BASE_RENDER_RES.y
          };
          gui_draw_map_stage_pin(state->hovered_stage == i, scrloc);
        }
      }
      break;
    }
    case IN_GAME_STAGE_PASSIVE_CHOICE: {
      render_map();
      break;
    }
    case IN_GAME_STAGE_PLAY: {
      render_map();
      render_game();
      break;
    }
    case IN_GAME_STAGE_PLAY_DEBUG: {
      render_map();
      render_game();
      break;
    }
    case IN_GAME_STAGE_PLAY_RESULTS: {
      render_map();
      break; 
    }
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::render_scene_in_game()::Unsupported stage");
      break;
    }
  }
}
void render_interface_in_game(void) {
  STATE_ASSERT("render_interface_in_game")
  
  switch (state->stage) {
    case IN_GAME_STAGE_MAP_CHOICE: {
      for (int i=0; i<MAX_WORLDMAP_LOCATIONS; ++i) {
        if (state->hovered_stage == i) {
          panel* pnl = &state->worldmap_selection_panel;
          Rectangle scrloc = (Rectangle){
            state->worldmap_locations[i].screen_location.x * BASE_RENDER_RES.x - WORLDMAP_LOC_PIN_SIZE_DIV2, 
            state->worldmap_locations[i].screen_location.y * BASE_RENDER_RES.y - WORLDMAP_LOC_PIN_SIZE_DIV2,
            WORLDMAP_LOC_PIN_SIZE, WORLDMAP_LOC_PIN_SIZE
          };
          pnl->dest = (Rectangle) {scrloc.x + WORLDMAP_LOC_PIN_SIZE, scrloc.y + WORLDMAP_LOC_PIN_SIZE_DIV2, BASE_RENDER_SCALE(.25f).x, BASE_RENDER_SCALE(.25f).y};
          DrawCircleGradient(scrloc.x + WORLDMAP_LOC_PIN_SIZE_DIV2, scrloc.y + WORLDMAP_LOC_PIN_SIZE_DIV2, 100, (Color){236,240,241,50}, (Color){0});
          gui_panel_scissored((*pnl), false, {
            gui_label(state->worldmap_locations[i].displayname, FONT_TYPE_MOOD, 10, (Vector2) {
              pnl->dest.x + pnl->dest.width *.5f, pnl->dest.y + pnl->dest.height*.5f
            }, WHITE, true, true);
          });
        }
      }
      break;
    }
    case IN_GAME_STAGE_PASSIVE_CHOICE: {
      Rectangle dest = (Rectangle) {
        BASE_RENDER_SCALE(.25f).x, BASE_RENDER_SCALE(.5f).y, 
        BASE_RENDER_SCALE(.25f).x, BASE_RENDER_SCALE(.8f).y 
      };
      f32 dest_x_buffer = dest.x;
      for (int i=0; i<MAX_UPDATE_PASSIVE_PANEL_COUNT; ++i) {
        panel* pnl = &state->passive_selection_panels[i];
        if (pnl->frame_tex_id <= ATLAS_TEX_ID_UNSPECIFIED || pnl->frame_tex_id >= ATLAS_TEX_ID_MAX || 
          pnl->bg_tex_id    <= ATLAS_TEX_ID_UNSPECIFIED || pnl->bg_tex_id    >= ATLAS_TEX_ID_MAX ) 
        {
          *pnl = state->default_panel;
        }
        if(pnl->buffer[0].data.u16[0] <= 0 || pnl->buffer[0].data.u16[0] >= CHARACTER_STATS_MAX) {
          pnl->buffer[0].data.u16[0] = get_random(1,CHARACTER_STATS_MAX-1);
        }
        dest.x = dest_x_buffer + ((dest.width + SCREEN_OFFSET.x) * i);
        character_stat* stat = get_player_in_game_stat(pnl->buffer[0].data.u16[0]);
        if(gui_panel_active(pnl, dest, true)) {
          set_is_game_paused(false);
          start_game(stat);
          for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
            state->passive_selection_panels[i].buffer[0].data.u16[0] = 0;
          }
          break;
        }
        draw_passive_selection_panel(stat, dest);
      }
      break;
    }
    case IN_GAME_STAGE_PLAY: {  
      DrawFPS(BASE_RENDER_SCALE(.75f).x, SCREEN_OFFSET.y * 10);

      if (!state->has_game_started) {
        gui_label("Press Space to Start!", FONT_TYPE_MOOD_OUTLINE, 10, (Vector2) {BASE_RENDER_SCALE(.5f).x, BASE_RENDER_SCALE(.5f).y}, WHITE, true, true);
        return;
      }
      else if (get_b_player_have_upgrade_points()) {
        set_is_game_paused(true);
        Rectangle dest = (Rectangle) {
          BASE_RENDER_SCALE(.25f).x, BASE_RENDER_SCALE(.5f).y, 
          BASE_RENDER_SCALE(.25f).x, BASE_RENDER_SCALE(.5f).y 
        };
        f32 dest_x_buffer = dest.x;
        for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
          panel* pnl = &state->ability_upg_panels[i];
          if(pnl->buffer[0].data.u16[0] <= 0 || pnl->buffer[0].data.u16[0] >= ABILITY_TYPE_MAX) {
            pnl->buffer[0].data.u16[0] = get_random(1,ABILITY_TYPE_MAX-1);
          }
          dest.x = dest_x_buffer + ((dest.width + SCREEN_OFFSET.x) * i);
          ability* abl = get_player_ability(pnl->buffer[0].data.u16[0]);
          ability new = {0};
          if (abl->type <= ABILITY_TYPE_UNDEFINED || abl->type >= ABILITY_TYPE_MAX) {
            new = _get_ability(pnl->buffer[0].data.u16[0]);
            *abl = (ability) {0};
          }
          else {
            new = _get_next_level(*abl);
          }
          if (new.type <= ABILITY_TYPE_UNDEFINED || new.type >= ABILITY_TYPE_MAX) {
            TraceLog(LOG_WARNING, "scene_in_game::render_interface_in_game()::Upgraded ability is out of bounds"); 
            return;
          }
          if(gui_panel_active(pnl, dest, true)) {
            set_is_game_paused(false);
            set_player_have_ability_upgrade_points(false);
            for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
              state->ability_upg_panels[i].buffer[0].data.u16[0] = 0;
            }
            if (new.level >= MAX_ABILITY_LEVEL || new.level <= 1) { 
              _add_ability(new.type); 
            }
            else { _upgrade_ability(abl); }
          }
          draw_in_game_upgrade_panel(abl, new, dest);
        }
      }
      else {
        gui_progress_bar(PRG_BAR_ID_PLAYER_EXPERIANCE, (Vector2){.x = BASE_RENDER_SCALE(.5f).x, .y = SCREEN_OFFSET.x}, true);
        gui_progress_bar(PRG_BAR_ID_PLAYER_HEALTH, SCREEN_OFFSET, false);
        gui_label_format(FONT_TYPE_MOOD, 10, BASE_RENDER_SCALE(.75f).x, SCREEN_OFFSET.y, WHITE, false, false, "Remaining: %d", get_remaining_enemies());
        gui_label_format(FONT_TYPE_MOOD, 10, BASE_RENDER_SCALE(.75f).x, SCREEN_OFFSET.y * 5.f, WHITE, false, false, "Souls: %d", get_currency_souls());

        gui_label_format(FONT_TYPE_MOOD, 10, 0, BASE_RENDER_SCALE(.35f).y, WHITE, false, false, "Health: %d", _get_player_in_game_state()->health_max);
        gui_label_format(FONT_TYPE_MOOD, 10, 0, BASE_RENDER_SCALE(.40f).y, WHITE, false, false, "Current Health: %d", _get_player_in_game_state()->health_current);
        gui_label_format(FONT_TYPE_MOOD, 10, 0, BASE_RENDER_SCALE(.45f).y, WHITE, false, false, "Damage: %d", _get_player_in_game_state()->damage);
      }
      break;
    }
    case IN_GAME_STAGE_PLAY_DEBUG: {  
      DrawFPS(BASE_RENDER_SCALE(.75f).x, SCREEN_OFFSET.y * 10);

      if (!state->has_game_started) { return; }
      else {
        gui_label_format(
          FONT_TYPE_MOOD, 15, ui_get_mouse_pos()->x, ui_get_mouse_pos()->y, 
          WHITE, false, false, "world_pos{%.1f, %.1f}", gm_get_mouse_pos_world().x, gm_get_mouse_pos_world().y
        );
        for (int i=0; i<get_remaining_enemies(); ++i) {
          if(state->hovered_spawn == i && i != 0){
            Character2D* spawn = get_spawn_info(i);
            panel* pnl = &state->debug_info_panel;
            pnl->dest = (Rectangle) {
              ui_get_mouse_pos()->x, ui_get_mouse_pos()->y, 
              BASE_RENDER_SCALE(.4f).x, BASE_RENDER_SCALE(.3f).y
            };
            i32 font_size = 18;
            i32 line_height = BASE_RENDER_SCALE(.05f).y;
            Vector2 debug_info_position_buffer = VECTOR2(pnl->dest.x, pnl->dest.y);
            gui_panel_scissored((*pnl), false, {
              gui_label_format(
                FONT_TYPE_MOOD, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
                WHITE, false, false, "Id: %d", spawn->character_id
              );
              debug_info_position_buffer.y += line_height;
              gui_label_format(
                FONT_TYPE_MOOD, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
                WHITE, false, false, "Collision: {%.1f, %.1f, %.1f, %.1f}", spawn->collision.x, spawn->collision.y, spawn->collision.width, spawn->collision.height
              );
              debug_info_position_buffer.y += line_height;
              gui_label_format(
                FONT_TYPE_MOOD, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
                WHITE, false, false, "Position: {%.1f, %.1f}", spawn->position.x, spawn->position.y
              );
              debug_info_position_buffer.y += line_height;
              gui_label_format(
                FONT_TYPE_MOOD, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
                WHITE, false, false, "Health: %d", spawn->health
              );
              debug_info_position_buffer.y += line_height;
              gui_label_format(
                FONT_TYPE_MOOD, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
                WHITE, false, false, "Scale: %.1f", spawn->scale
              );
              debug_info_position_buffer.y += line_height;
              gui_label_format(
                FONT_TYPE_MOOD, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
                WHITE, false, false, "Speed: %.1f", spawn->speed
              );
            });
          }
        }

        gui_label_format(FONT_TYPE_MOOD, 10, BASE_RENDER_SCALE(.75f).x, SCREEN_OFFSET.y, WHITE, false, false, "Remaining: %d", get_remaining_enemies());
        gui_label_format(FONT_TYPE_MOOD, 10, BASE_RENDER_SCALE(.75f).x, SCREEN_OFFSET.y * 5.f, WHITE, false, false, "Souls: %d", get_currency_souls());

        gui_label_format(FONT_TYPE_MOOD, 10, 0, BASE_RENDER_SCALE(.35f).y, WHITE, false, false, "Health: %d", _get_player_in_game_state()->health_max);
        gui_label_format(FONT_TYPE_MOOD, 10, 0, BASE_RENDER_SCALE(.40f).y, WHITE, false, false, "Current Health: %d", _get_player_in_game_state()->health_current);
        gui_label_format(FONT_TYPE_MOOD, 10, 0, BASE_RENDER_SCALE(.45f).y, WHITE, false, false, "Damage: %d", _get_player_in_game_state()->damage);
      }
      break;
    }
    case IN_GAME_STAGE_PLAY_RESULTS: { 
      draw_end_game_panel();
      if(gui_menu_button("Accept", BTN_ID_IN_GAME_BUTTON_RETURN_MENU, (Vector2) {0, 5}, 2.7f, true)) {
        currency_souls_add(state->end_game_result.collected_souls);
        gm_save_game();
        end_scene_in_game();
        event_fire(EVENT_CODE_SCENE_MAIN_MENU, (event_context) {0});
      }
      break; 
    }
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::render_interface_in_game()::Unsupported stage");
      break;
    }
  }

  render_user_interface();
}

void draw_in_game_upgrade_panel(ability* abl, ability upg, Rectangle panel_dest) {
  if (upg.type <= ABILITY_TYPE_UNDEFINED || upg.type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_WARNING, "scene_in_game::draw_upgrade_panel()::Upgraded ability is out of bounds"); 
    return;
  }
  if (upg.level <= 0 || upg.level >= MAX_ABILITY_LEVEL) {
    TraceLog(LOG_WARNING, "scene_in_game::draw_upgrade_panel()::Upgraded ability level is out of bounds"); 
    return;
  }
  const u16 elm_space_gap = 30;
  const u16 btw_space_gap = 20;
  const u16 start_panel_height = panel_dest.y - panel_dest.height*.25f;
  const Rectangle icon_rect = (Rectangle) {
    panel_dest.x - ABILITY_UPG_PANEL_ICON_SIZE/2.f,
    start_panel_height - ABILITY_UPG_PANEL_ICON_SIZE*.5f,
    ABILITY_UPG_PANEL_ICON_SIZE,ABILITY_UPG_PANEL_ICON_SIZE
  };
  const Vector2 ability_name_pos  = (Vector2) {panel_dest.x, start_panel_height + ABILITY_UPG_PANEL_ICON_SIZE*.5f + elm_space_gap};
  const Vector2 ability_level_ind = (Vector2) {panel_dest.x, ability_name_pos.y + btw_space_gap};

  const u16 title_font_size = 10; 
  const u16 level_ind_font_size = 9; 
  const u16 upgr_font_size = 6; 

  gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_ICON_ATLAS, upg.icon_src, icon_rect, true, false);
  gui_label(upg.display_name, FONT_TYPE_MOOD, title_font_size, ability_name_pos, WHITE, true, true);

  if (upg.level == 1) {
    gui_label("NEW!", FONT_TYPE_MOOD_OUTLINE, level_ind_font_size, ability_level_ind, WHITE, true, true);
  } else if(upg.level>1 && upg.level <= MAX_ABILITY_LEVEL) {
    gui_label_format_v(FONT_TYPE_MOOD_OUTLINE, level_ind_font_size, ability_level_ind, WHITE, true, true, "%d -> %d", abl->level, upg.level);
  }
  const u16 start_upgradables_x_exis = panel_dest.x - panel_dest.width*.5f + elm_space_gap;
  u16 upgradables_height_buffer = ability_level_ind.y + elm_space_gap;
  for (int i=0; i<ABILITY_UPG_MAX; ++i) {
    ability_upgradables abl_upg = upg.upgradables[i];
    if (abl_upg <= ABILITY_UPG_UNDEFINED  || abl_upg >= ABILITY_UPG_MAX) {
      break;
    }
    switch (abl_upg) {
      case ABILITY_UPG_DAMAGE: DRAW_ABL_UPG_STAT_PNL(abl, upg, "Damage:", base_damage);break;
      case ABILITY_UPG_AMOUNT: DRAW_ABL_UPG_STAT_PNL(abl, upg, "Amouth:", proj_count); break;
      case ABILITY_UPG_HITBOX: DRAW_ABL_UPG_STAT_PNL(abl, upg, "Hitbox:", proj_dim.x); break;
      case ABILITY_UPG_SPEED:  DRAW_ABL_UPG_STAT_PNL(abl, upg, "Speed:",  proj_speed); break;
      
      default: {
        TraceLog(LOG_WARNING, "scene_in_game::draw_upgrade_panel()::Unsupported ability upgrade type");
        break;
      }
    }
  }
}
void draw_passive_selection_panel(character_stat* stat, Rectangle panel_dest) {
  if (stat && (stat->id <= CHARACTER_STATS_UNDEFINED || stat->id >= CHARACTER_STATS_MAX)) {
    TraceLog(LOG_WARNING, "scene_in_game::draw_passive_selection_panel()::Character stat id is out of bound"); 
    return;
  }
  const u16 elm_space_gap = 30;
  const u16 start_panel_height = panel_dest.y - panel_dest.height*.25f;
  const Rectangle icon_rect = (Rectangle) {
    panel_dest.x - PASSIVE_SELECTION_PANEL_ICON_SIZE/2.f,
    start_panel_height - PASSIVE_SELECTION_PANEL_ICON_SIZE*.5f,
    PASSIVE_SELECTION_PANEL_ICON_SIZE,PASSIVE_SELECTION_PANEL_ICON_SIZE
  };
  const Vector2 passive_name_pos  = (Vector2) {panel_dest.x, start_panel_height + PASSIVE_SELECTION_PANEL_ICON_SIZE*.5f + elm_space_gap};

  const u16 title_font_size = 10; 
  
  gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_ICON_ATLAS, stat->passive_icon_src, icon_rect, true, false);
  gui_label(stat->passive_display_name, FONT_TYPE_MOOD, title_font_size, passive_name_pos, WHITE, true, true);
  
  const u16 desc_font_size = 6;
  const f32 desc_box_height = panel_dest.y + (panel_dest.height * .5f) - passive_name_pos.y - elm_space_gap;
  const Vector2 padding = (Vector2){ SCREEN_OFFSET.x, elm_space_gap };
  const Rectangle desc_box = (Rectangle) {
    .x      = panel_dest.x - (panel_dest.width * .5f) + padding.x,
    .y      = passive_name_pos.y + padding.y,
    .width  = panel_dest.width - padding.x,
    .height = desc_box_height - padding.y,
  };
  gui_label_wrap(stat->passive_desc, FONT_TYPE_MINI_MOOD, desc_font_size, desc_box, WHITE, false);

}
void draw_end_game_panel() {
  STATE_ASSERT("draw_end_game_panel")
  gui_panel(state->default_panel, (Rectangle){ BASE_RENDER_SCALE(.5f).x, BASE_RENDER_SCALE(.5f).y, BASE_RENDER_SCALE(.75f).x, BASE_RENDER_SCALE(.75f).y}, true);

  if (state->end_game_result.is_win) {
    gui_label("Stage Cleared", FONT_TYPE_MOOD, 20, BASE_RENDER_SCALE(.5f), WHITE, true, true);
  }
  else {
    gui_label("Dead", FONT_TYPE_MOOD, 20, BASE_RENDER_SCALE(.5f), RED, true, true);
  }
  u32 min  = (i32)state->end_game_result.play_time/60;
  u32 secs = (i32)state->end_game_result.play_time%60;
  gui_label_format_v(FONT_TYPE_MOOD, 15, VECTOR2(BASE_RENDER_SCALE(.5f).x, BASE_RENDER_SCALE(.55f).y), WHITE, true, true, "%d:%d", min, secs);

  gui_label_format_v(FONT_TYPE_MOOD, 15, VECTOR2(BASE_RENDER_SCALE(.5f).x, BASE_RENDER_SCALE(.75f).y), WHITE, true, true, 
    "Collected Souls:%d", state->end_game_result.collected_souls
  );
}
Rectangle sig_get_camera_view_rect(Camera2D camera) {

  f32 view_width = BASE_RENDER_RES.x / camera.zoom;
  f32 view_height = BASE_RENDER_RES.y / camera.zoom;

  f32 x = camera.target.x;
  f32 y = camera.target.y;
  
  x -= camera.offset.x/camera.zoom;
  y -= camera.offset.y/camera.zoom;
  
  return (Rectangle){ x, y, view_width, view_height };
}

bool scene_in_game_on_event(u16 code, event_context context) {
  switch (code) {
    case EVENT_CODE_ADD_CURRENCY_SOULS: {
      state->end_game_result.collected_souls += context.data.u32[0];
      return true;
    }
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::scene_in_game_on_event()::Unsuppported code.");
      return false;
    }
  }

  TraceLog(LOG_WARNING, "scene_in_game::scene_in_game_on_event()::Fire event ended unexpectedly");
  return false;
}

#undef STATE_ASSERT
#undef ABILITY_UPG_PANEL_ICON_SIZE
#undef CLOUDS_ANIMATION_DURATION
#undef FADE_ANIMATION_DURATION
#undef DRAW_ABL_UPG_STAT_PNL

