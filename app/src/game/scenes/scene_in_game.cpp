#include "scene_in_game.h"
#include <reasings.h>
#include <settings.h>
#include <loc_types.h>

#include <core/fmath.h>
#include <core/event.h>
#include <core/fmemory.h>
#include <core/ftime.h>

#include "game/game_manager.h"
#include "game/user_interface.h"
#include "game/world.h"
#include "game/camera.h"

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
  end_game_results(void) {
    this->collected_souls = 0u;
    this->play_time = 0.f;
    this->is_win = false;
  }
}end_game_results;

typedef struct scene_in_game_state {
  std::array<worldmap_stage, MAX_WORLDMAP_LOCATIONS>  worldmap_locations;
  std::array<panel, MAX_UPDATE_ABILITY_PANEL_COUNT> ability_upg_panels;
  std::array<panel, MAX_UPDATE_PASSIVE_PANEL_COUNT> passive_selection_panels;
  panel worldmap_selection_panel;
  panel debug_info_panel;
  panel default_panel;
  std::array<ability, MAX_UPDATE_ABILITY_PANEL_COUNT> ability_upgrade_choices;
  bool is_upgrade_choices_ready;
  
  const camera_metrics* in_camera_metrics;
  const app_settings* in_app_settings;
  const ingame_info* in_ingame_info;
  
  in_game_stages stage;
  end_game_results end_game_result;
  u16 hovered_stage;
  u16 hovered_spawn;
  ability_type hovered_ability;
  u16 hovered_projectile;
  bool has_game_started;
  bool show_pause_menu;
} scene_in_game_state;

static scene_in_game_state * state;

#define STATE_ASSERT(FUNCTION) if (!state) {                                              \
  TraceLog(LOG_ERROR, "scene_in_game::" FUNCTION "::In game state was not initialized");  \
  event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context());                                \
  return;                                                                                 \
}

#define SIG_BASE_RENDER_SCALE(SCALE) VECTOR2(\
  static_cast<f32>(state->in_app_settings->render_width * SCALE),\
  static_cast<f32>(state->in_app_settings->render_height * SCALE))
#define SIG_BASE_RENDER_DIV2 VECTOR2(static_cast<f32>(state->in_app_settings->render_width_div2), static_cast<f32>(state->in_app_settings->render_height_div2))
#define SIG_BASE_RENDER_WIDTH state->in_app_settings->render_width
#define SIG_BASE_RENDER_HEIGHT state->in_app_settings->render_height
#define SIG_SCREEN_POS(X, Y) (Vector2{  \
  SIG_BASE_RENDER_WIDTH  * (X / 100), \
  SIG_BASE_RENDER_HEIGHT * (Y / 100)  \
})

#define ABILITY_UPG_PANEL_ICON_SIZE SIG_BASE_RENDER_WIDTH * .25f * .5f
#define PASSIVE_SELECTION_PANEL_ICON_SIZE ABILITY_UPG_PANEL_ICON_SIZE
#define CLOUDS_ANIMATION_DURATION TARGET_FPS * 1.5f // second

bool scene_in_game_on_event(i32 code, event_context context);
void begin_scene_in_game(void);
void in_game_update_bindings(void);
void in_game_update_mouse_bindings(void);
void in_game_update_keyboard_bindings(void);
void initialize_worldmap_locations(void);
void draw_in_game_upgrade_panel(u16 which_panel, Rectangle panel_dest);
void draw_passive_selection_panel(const character_stat* stat,const Rectangle panel_dest);
void draw_end_game_panel(void);
void prepare_ability_upgrade_state(void);
void end_ability_upgrade_state(u16 which_panel_chosen);

void start_game(character_stats stat);
void reset_game();

/**
 * @brief Requires world system, world init moved to app, as well as its loading time
 */
bool initialize_scene_in_game(const app_settings * _in_app_settings) {
  if (state) {
    begin_scene_in_game();
    return false;
  }
  state = (scene_in_game_state *)allocate_memory_linear(sizeof(scene_in_game_state), true);
  if (state == nullptr) {
    TraceLog(LOG_ERROR, "scene_in_game::initialize_scene_in_game()::State allocation failed!");
    return false;
  }

  state->in_app_settings = _in_app_settings;
  if (!_in_app_settings || _in_app_settings == nullptr) {
    TraceLog(LOG_ERROR, "scene_in_game::initialize_scene_in_game()::App setting pointer is invalid");
    return false;
  }

  if(!create_camera(
    state->in_app_settings->render_width_div2, state->in_app_settings->render_height_div2, 
    state->in_app_settings->render_width, state->in_app_settings->render_height
  )) {
    TraceLog(LOG_ERROR, "scene_in_game::initialize_scene_in_game()::Creating camera failed");
    return false;
  }
  state->in_camera_metrics = get_in_game_camera();
  if (!state->in_camera_metrics || state->in_camera_metrics == nullptr) {
    TraceLog(LOG_ERROR, "scene_in_game::initialize_scene_in_game()::Camera pointer is invalid");
    return false;
  }

  if (!game_manager_initialize(state->in_camera_metrics, _in_app_settings, get_active_map_ptr())) { // Inits player & spawns
    TraceLog(LOG_ERROR, "scene_in_game::initialize_scene_in_game()::game_manager_initialize() failed");
    return false;
  }
  state->in_ingame_info = gm_get_ingame_info();
  if (!state->in_ingame_info || state->in_ingame_info == nullptr) {
    TraceLog(LOG_ERROR, "scene_in_game::initialize_scene_in_game()::Game info pointer is invalid");
    return false;
  }

  event_register(EVENT_CODE_ADD_CURRENCY_SOULS, scene_in_game_on_event);
  event_register(EVENT_CODE_RESUME_GAME, scene_in_game_on_event);

  begin_scene_in_game();
  return true;
}

void begin_scene_in_game(void) {
  
  state->default_panel = panel( BTN_STATE_RELEASED, ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG, ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL, 
    Vector4 {6, 6, 6, 6}, Color { 30, 39, 46, 245}, Color { 52, 64, 76, 245}
  );
  copy_memory(state->worldmap_locations.data(), get_worldmap_locations(), MAX_WORLDMAP_LOCATIONS * sizeof(worldmap_stage));
  _set_player_position(Vector2 {0.f, 0.f});
  set_is_game_paused(true);
  
  for (size_t itr_000 = 0; itr_000 < MAX_UPDATE_ABILITY_PANEL_COUNT; ++itr_000) {
    state->ability_upg_panels.at(itr_000) = state->default_panel;
  }
  for (size_t itr_000 = 0; itr_000 < MAX_UPDATE_PASSIVE_PANEL_COUNT; ++itr_000) {
    state->passive_selection_panels.at(itr_000) = state->default_panel;
  }
  state->worldmap_selection_panel = state->default_panel;
  state->debug_info_panel = state->default_panel;

  state->stage = IN_GAME_STAGE_MAP_CHOICE;
  state->hovered_stage = U16_MAX;
  state->hovered_spawn = U16_MAX;
  state->hovered_ability = ABILITY_TYPE_MAX;
  state->hovered_projectile = U16_MAX;

  event_fire(EVENT_CODE_UI_START_FADEIN_EFFECT, event_context((u16)CLOUDS_ANIMATION_DURATION));
}
void start_game(character_stats stat) {
  if (!state) {
    TraceLog(LOG_ERROR, "scene_in_game::start_game()::state returned zero");
    return;
  }
  gm_start_game(*get_active_worldmap());

  upgrade_dynamic_player_stat(stat);
  _set_player_position(ZEROVEC2);
  state->stage = IN_GAME_STAGE_PLAY;
}
void end_scene_in_game(void) {
  gm_reset_game();

  //state->worldmap_locations       = one time set, no need to change every time;
  //state->default_panel            = no need to set it second time;
  //state->ability_upg_panels       = same as default_panel;
  //state->passive_selection_panels = same as default_panel;
  //state->worldmap_selection_panel = same as default_panel;
  //state->debug_info_panel         = same as default_panel;
  //state->in_camera_metrics        = pointer from camera system, do not touch it here.
  //state->is_game_paused           = Handled by update, each frame fetches from game manager;

  state->stage = IN_GAME_STAGE_MAP_CHOICE;
  state->end_game_result = end_game_results();
  state->hovered_stage = U16_MAX;
  state->hovered_spawn = U16_MAX;
  state->hovered_ability = ABILITY_TYPE_MAX;
  state->hovered_projectile = U16_MAX;
  state->has_game_started = false;
  state->show_pause_menu = false;
}

void update_scene_in_game(void) {
  STATE_ASSERT("update_scene_in_game")

  update_camera();
  in_game_update_bindings();
  update_user_interface();

  switch (state->stage) {
    case IN_GAME_STAGE_MAP_CHOICE: {
      event_fire(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, event_context(SIG_BASE_RENDER_WIDTH * .5f, SIG_BASE_RENDER_HEIGHT * .5f));

      if (state->show_pause_menu) {}
      else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->hovered_stage <= MAX_WORLDMAP_LOCATIONS) {
        set_worldmap_location(state->hovered_stage);
        state->stage = IN_GAME_STAGE_PASSIVE_CHOICE;
      }
      break;
    }
    case IN_GAME_STAGE_PASSIVE_CHOICE: {
      event_fire(EVENT_CODE_CAMERA_SET_CAMERA_POSITION, event_context(
        state->in_ingame_info->player_state_dynamic->position_centered.x,
        state->in_ingame_info->player_state_dynamic->position_centered.y
      ));
      break;
    }
    case IN_GAME_STAGE_PLAY: {
      event_fire(EVENT_CODE_CAMERA_SET_TARGET, event_context(
        state->in_ingame_info->player_state_dynamic->position_centered.x,
        state->in_ingame_info->player_state_dynamic->position_centered.y
    ));

      if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyReleased(KEY_D)) {
        state->stage = IN_GAME_STAGE_PLAY_DEBUG;
      }

      if (!state->has_game_started) { return; }
      if (*state->in_ingame_info->is_game_paused) { return; }
      else if (get_remaining_enemies() <= 0 || get_remaining_enemies() > MAX_SPAWN_COUNT) {
        event_fire(EVENT_CODE_END_GAME, event_context());
        state->end_game_result.is_win = true;
      }
      if (get_is_game_end()) {
        state->stage = IN_GAME_STAGE_PLAY_RESULTS;
      }

      update_map();
      update_game_manager();
      state->end_game_result.play_time += GetFrameTime();
      break;
    }
    case IN_GAME_STAGE_PLAY_DEBUG: {
      if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyReleased(KEY_D)) {
        state->stage = IN_GAME_STAGE_PLAY;
      }
      update_map();
      update_game_manager_debug();

      event_fire(EVENT_CODE_CAMERA_SET_TARGET, event_context(
        state->in_ingame_info->player_state_dynamic->position_centered.x,
        state->in_ingame_info->player_state_dynamic->position_centered.y
      ));
      break;
    }
    case IN_GAME_STAGE_PLAY_RESULTS: { break; }
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::update_scene_in_game()::Unsupported stage");
      break;
    }
  }
}
void render_scene_in_game(void) {
  STATE_ASSERT("render_scene_in_game")
  BeginMode2D(get_in_game_camera()->handle);

  switch (state->stage) {
    case IN_GAME_STAGE_MAP_CHOICE: {
      gui_draw_texture_id(TEX_ID_WORLDMAP_WO_CLOUDS, Rectangle {0, 0, static_cast<f32>(SIG_BASE_RENDER_WIDTH), static_cast<f32>(SIG_BASE_RENDER_HEIGHT)});
      for (int i=0; i<MAX_WORLDMAP_LOCATIONS; ++i) {
        if (state->worldmap_locations.at(i).is_active) {
          Vector2 scrloc = Vector2 {
            state->worldmap_locations.at(i).screen_location.x * SIG_BASE_RENDER_WIDTH, 
            state->worldmap_locations.at(i).screen_location.y * SIG_BASE_RENDER_HEIGHT
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
      if (!(*state->in_ingame_info->is_game_paused) || state->has_game_started) {
        const ingame_info* const iginf = gm_get_ingame_info();

        i32 bottom_of_the_screen = state->in_camera_metrics->frustum.y + state->in_camera_metrics->frustum.height;
        i32 top_of_the_screen = state->in_camera_metrics->frustum.y;
        i32 player_texture_begin = iginf->player_state_dynamic->position.y + iginf->player_state_dynamic->dimentions.y;
        
        _render_props_y_based(top_of_the_screen, player_texture_begin);
        render_game();
        _render_props_y_based(player_texture_begin, bottom_of_the_screen);
        
        DrawRectangleLines(
          static_cast<i32>(iginf->player_state_dynamic->position.x), 
          static_cast<i32>(iginf->player_state_dynamic->position.y), 
          static_cast<i32>(iginf->player_state_dynamic->dimentions.x), 
          static_cast<i32>(iginf->player_state_dynamic->dimentions.y), 
          RED
        );

        const Character2D* chr = iginf->nearest_spawn;
        if (chr) DrawRectangleLines(chr->collision.x, chr->collision.y, chr->collision.width, chr->collision.height, RED); // TODO: Nearest spawn indicator. Remove later
      }
      
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
  
  EndMode2D();
}
void render_interface_in_game(void) {
  STATE_ASSERT("render_interface_in_game")
  
  switch (state->stage) {
    case IN_GAME_STAGE_MAP_CHOICE: {
      if (state->show_pause_menu) {
        gui_draw_pause_screen(false);
      }
      else {
        for (int i=0; i<MAX_WORLDMAP_LOCATIONS; ++i) {
          if (state->hovered_stage == i) {
            panel* pnl = __builtin_addressof(state->worldmap_selection_panel);
            Rectangle scrloc = Rectangle{
              state->worldmap_locations.at(i).screen_location.x * SIG_BASE_RENDER_WIDTH - WORLDMAP_LOC_PIN_SIZE_DIV2, 
              state->worldmap_locations.at(i).screen_location.y * SIG_BASE_RENDER_HEIGHT - WORLDMAP_LOC_PIN_SIZE_DIV2,
              WORLDMAP_LOC_PIN_SIZE, WORLDMAP_LOC_PIN_SIZE
            };
            pnl->dest = Rectangle {scrloc.x + WORLDMAP_LOC_PIN_SIZE, scrloc.y + WORLDMAP_LOC_PIN_SIZE_DIV2, SIG_BASE_RENDER_WIDTH * .25f, SIG_BASE_RENDER_HEIGHT * .25f};
            DrawCircleGradient(scrloc.x + WORLDMAP_LOC_PIN_SIZE_DIV2, scrloc.y + WORLDMAP_LOC_PIN_SIZE_DIV2, 100, Color{236,240,241,50}, Color{255, 255, 255, 0});
            GUI_PANEL_SCISSORED((*pnl), false, {
              gui_label(state->worldmap_locations.at(i).displayname.c_str(), FONT_TYPE_ABRACADABRA, 1, Vector2 {
                pnl->dest.x + pnl->dest.width *.5f, pnl->dest.y + pnl->dest.height*.5f
              }, WHITE, true, true);
            });
          }
        }
      }
      break;
    }
    case IN_GAME_STAGE_PASSIVE_CHOICE: {
      if (state->show_pause_menu) {
        gui_draw_pause_screen(false);
      }
      else {
        Rectangle dest = Rectangle {
          SIG_BASE_RENDER_WIDTH * .25f, SIG_BASE_RENDER_HEIGHT * .5f, 
          SIG_BASE_RENDER_WIDTH * .25f, SIG_BASE_RENDER_HEIGHT * .8f 
        };
        f32 dest_x_buffer = dest.x;
        for (size_t itr_000 = 0; itr_000 < MAX_UPDATE_PASSIVE_PANEL_COUNT; ++itr_000) {
          panel* pnl = __builtin_addressof(state->passive_selection_panels.at(itr_000));
          if (pnl->frame_tex_id <= ATLAS_TEX_ID_UNSPECIFIED || pnl->frame_tex_id >= ATLAS_TEX_ID_MAX || 
            pnl->bg_tex_id    <= ATLAS_TEX_ID_UNSPECIFIED || pnl->bg_tex_id    >= ATLAS_TEX_ID_MAX ) 
          {
            *pnl = state->default_panel;
          }
          if(pnl->buffer.u16[0] <= 0 || pnl->buffer.u16[0] >= CHARACTER_STATS_MAX) {
            pnl->buffer.u16[0] = get_random(1,CHARACTER_STATS_MAX-1);
          }
          dest.x = dest_x_buffer + ((dest.width + SCREEN_OFFSET.x) * itr_000);
          const character_stat* stat = get_dynamic_player_state_stat(static_cast<character_stats>(pnl->buffer.u16[0]));
          if(gui_panel_active(pnl, dest, true)) {
            set_is_game_paused(false);
            start_game(stat->id);
            for (size_t itr_111 = 0; itr_111 < MAX_UPDATE_ABILITY_PANEL_COUNT; ++itr_111) {
              state->passive_selection_panels.at(itr_111).buffer.u16[0] = 0;
            }
            break;
          }
          draw_passive_selection_panel(stat, dest);
        }
      }
      break;
    }
    case IN_GAME_STAGE_PLAY: {  

      if (state->show_pause_menu) {
        gui_draw_pause_screen(state->has_game_started);
      }
      else if (!state->has_game_started) {
        gui_label("Press Space to Start!", FONT_TYPE_ABRACADABRA, 1, Vector2 {0.f, SIG_BASE_RENDER_HEIGHT * .5f}, WHITE, true, true);
      }
      else if (get_b_player_have_upgrade_points()) {
        if (state->is_upgrade_choices_ready) {
          Rectangle dest = Rectangle {
            SIG_BASE_RENDER_WIDTH * .25f, SIG_BASE_RENDER_HEIGHT * .5f, 
            SIG_BASE_RENDER_WIDTH * .25f, SIG_BASE_RENDER_HEIGHT * .5f 
          };
          f32 dest_x_buffer = dest.x;
          for (size_t itr_000 = 0; itr_000 < MAX_UPDATE_ABILITY_PANEL_COUNT; ++itr_000) {
            panel* pnl = __builtin_addressof(state->ability_upg_panels.at(itr_000));
            dest.x = dest_x_buffer + ((dest.width + SCREEN_OFFSET.x) * itr_000);

            if(gui_panel_active(pnl, dest, true)) {
              end_ability_upgrade_state(itr_000);
              break;
            }
            draw_in_game_upgrade_panel(itr_000, dest);
          }
        }
        else { prepare_ability_upgrade_state(); }
      }
      else {
        gui_progress_bar(PRG_BAR_ID_PLAYER_EXPERIANCE, Vector2{SIG_BASE_RENDER_WIDTH * .5f, SCREEN_OFFSET.y}, true);
        gui_progress_bar(PRG_BAR_ID_PLAYER_HEALTH, SCREEN_OFFSET, false);
      }
      break;
    }
    case IN_GAME_STAGE_PLAY_DEBUG: {  

      if (state->show_pause_menu) {
        gui_draw_pause_screen(state->has_game_started);
      }
      else if (!state->has_game_started) { }
      else {
        gui_label_format(
          FONT_TYPE_ABRACADABRA, 1, ui_get_mouse_pos()->x, ui_get_mouse_pos()->y, 
          WHITE, false, false, "world_pos {%.1f, %.1f}", gm_get_mouse_pos_world()->x, gm_get_mouse_pos_world()->y
        );

        if(state->hovered_spawn < get_remaining_enemies()){
          const Character2D* spawn = get_spawn_info(state->hovered_spawn);
          panel* const pnl = __builtin_addressof(state->debug_info_panel);
          pnl->dest = Rectangle {
            ui_get_mouse_pos()->x, ui_get_mouse_pos()->y, 
            SIG_BASE_RENDER_WIDTH * .4f, SIG_BASE_RENDER_HEIGHT * .3f
          };
          i32 font_size = 1;
          i32 line_height = SIG_BASE_RENDER_HEIGHT * .05f;
          Vector2 debug_info_position_buffer = VECTOR2(pnl->dest.x, pnl->dest.y);
          GUI_PANEL_SCISSORED((*pnl), false, {
            gui_label_format(
              FONT_TYPE_ABRACADABRA, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
              WHITE, false, false, "Id: %d", spawn->character_id
            );
            debug_info_position_buffer.y += line_height;
            gui_label_format(
              FONT_TYPE_ABRACADABRA, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
              WHITE, false, false, "Collision: {%.1f, %.1f, %.1f, %.1f}", spawn->collision.x, spawn->collision.y, spawn->collision.width, spawn->collision.height
            );
            debug_info_position_buffer.y += line_height;
            gui_label_format(
              FONT_TYPE_ABRACADABRA, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
              WHITE, false, false, "Position: {%.1f, %.1f}", spawn->position.x, spawn->position.y
            );
            debug_info_position_buffer.y += line_height;
            gui_label_format(
              FONT_TYPE_ABRACADABRA, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
              WHITE, false, false, "Health: %d", spawn->health
            );
            debug_info_position_buffer.y += line_height;
            gui_label_format(
              FONT_TYPE_ABRACADABRA, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
              WHITE, false, false, "Scale: %.1f", spawn->scale
            );
            debug_info_position_buffer.y += line_height;
            gui_label_format(
              FONT_TYPE_ABRACADABRA, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
              WHITE, false, false, "Speed: %.1f", spawn->speed
            );
          });
        }

        const ability* abl = get_dynamic_player_state_ability(state->hovered_ability);
        if(state->hovered_ability > 0 && state->hovered_ability < ABILITY_TYPE_MAX && state->hovered_projectile >= 0 && state->hovered_projectile < abl->projectiles.size()){
          panel* pnl = __builtin_addressof(state->debug_info_panel);
          pnl->dest = Rectangle {
            ui_get_mouse_pos()->x, ui_get_mouse_pos()->y, 
            SIG_BASE_RENDER_WIDTH * .4f, SIG_BASE_RENDER_HEIGHT * .3f
          };
          i32 font_size = 1;
          i32 line_height = SIG_BASE_RENDER_HEIGHT * .05f;
          Vector2 debug_info_position_buffer = VECTOR2(pnl->dest.x, pnl->dest.y);
          const projectile* prj = __builtin_addressof(abl->projectiles.at(state->hovered_projectile));
          GUI_PANEL_SCISSORED((*pnl), false, {
            gui_label_format(
              FONT_TYPE_ABRACADABRA, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
              WHITE, false, false, "Collision: {%.1f, %.1f, %.1f, %.1f}", prj->collision.x, prj->collision.y, prj->collision.width, prj->collision.height
            );
            debug_info_position_buffer.y += line_height;
            gui_label_format(
              FONT_TYPE_ABRACADABRA, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
              WHITE, false, false, "Rotation: %.1f", prj->animations.at(prj->active_sprite).rotation
            );
          });
        }
        
        gui_label_format(FONT_TYPE_ABRACADABRA, 1, SIG_BASE_RENDER_WIDTH * .75f, SCREEN_OFFSET.y, WHITE, false, false, "Remaining: %d", get_remaining_enemies());
        gui_label_format(FONT_TYPE_ABRACADABRA, 1, SIG_BASE_RENDER_WIDTH * .75f, SCREEN_OFFSET.y * 5.f, WHITE, false, false, "Souls: %d", get_currency_souls());

        gui_label_format(FONT_TYPE_ABRACADABRA, 1, 0, SIG_BASE_RENDER_HEIGHT * .35f, WHITE, false, false, "Health: %d", _get_dynamic_player_state()->health_max);
        gui_label_format(FONT_TYPE_ABRACADABRA, 1, 0, SIG_BASE_RENDER_HEIGHT * .40f, WHITE, false, false, "Current Health: %d", _get_dynamic_player_state()->health_current);
        gui_label_format(FONT_TYPE_ABRACADABRA, 1, 0, SIG_BASE_RENDER_HEIGHT * .45f, WHITE, false, false, "Damage: %d", _get_dynamic_player_state()->damage);
      }

      break;
    }
    case IN_GAME_STAGE_PLAY_RESULTS: { 
      draw_end_game_panel();
      if(gui_menu_button("Accept", BTN_ID_IN_GAME_BUTTON_RETURN_MENU, Vector2 {0, 5}, SIG_BASE_RENDER_DIV2, true)) {
        currency_souls_add(state->end_game_result.collected_souls);
        gm_save_game();
        end_scene_in_game();
        event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context());
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
void in_game_update_mouse_bindings(void) { 
  switch (state->stage) {
    case IN_GAME_STAGE_MAP_CHOICE: {
      state->hovered_stage = U16_MAX;
      for (int i=0; i<MAX_WORLDMAP_LOCATIONS; ++i) {
        Rectangle scrloc = Rectangle{
          state->worldmap_locations.at(i).screen_location.x * SIG_BASE_RENDER_WIDTH - WORLDMAP_LOC_PIN_SIZE_DIV2, 
          state->worldmap_locations.at(i).screen_location.y * SIG_BASE_RENDER_HEIGHT - WORLDMAP_LOC_PIN_SIZE_DIV2,
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
      const Vector2* mouse_pos_world = gm_get_mouse_pos_world();
      state->hovered_spawn = U16_MAX;
      for (int i=0; i<get_remaining_enemies(); ++i) {
        const Character2D* spawn = get_spawn_info(i);
        if (spawn) {
          if (CheckCollisionPointRec(*mouse_pos_world, spawn->collision)) {
            state->hovered_spawn = i;
          }
        }
      }
      const player_state* player = _get_dynamic_player_state();
      state->hovered_ability = ABILITY_TYPE_UNDEFINED;
      state->hovered_projectile = U16_MAX;
      for (size_t iter = 0; iter < player->ability_system.abilities.size(); ++iter) {
        const ability* abl = __builtin_addressof(player->ability_system.abilities.at(iter));
        if(!abl || !abl->is_active || !abl->is_initialized) { continue; }
        for (size_t j=0; j < abl->projectiles.size(); j++) {
          const projectile* prj = __builtin_addressof(abl->projectiles.at(j));
          if (!prj || !prj->is_active) { continue; }
          if (CheckCollisionPointRec(*mouse_pos_world, prj->collision)) {
            state->hovered_ability = abl->type;
            state->hovered_projectile = j;
            break;
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
        state->show_pause_menu = !state->show_pause_menu;
      }

      break;
    }
    case IN_GAME_STAGE_PASSIVE_CHOICE: {
      if (IsKeyReleased(KEY_ESCAPE)) {
        state->show_pause_menu = !state->show_pause_menu;
      }

      if (IsKeyReleased(KEY_R)) {
        for (size_t itr_000 = 0; itr_000 < MAX_UPDATE_PASSIVE_PANEL_COUNT; ++itr_000) {
          state->passive_selection_panels.at(itr_000).buffer.u16[0] = 0;
        }
      }
      break;
    }
    case IN_GAME_STAGE_PLAY: {  
      if (!state->has_game_started) {
        if (IsKeyReleased(KEY_ESCAPE)) {
          state->show_pause_menu = !state->show_pause_menu;
        }
        else if (IsKeyPressed(KEY_SPACE)) {
          state->has_game_started = true;
          set_is_game_paused(false);
        }
        return; 
      }
      if (!(*state->in_ingame_info->is_game_paused)) {
        if (IsKeyReleased(KEY_ESCAPE)) {
          state->show_pause_menu = true;
          set_is_game_paused(true);
        }
      }
      else {
        if (IsKeyReleased(KEY_ESCAPE)) {
          state->show_pause_menu = false;
          set_is_game_paused(false);
        }
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

#define DRAW_ABL_UPG_STAT_PNL(UPG, TEXT, ...){ \
if (UPG->level == 1) {\
  gui_label_format(FONT_TYPE_ABRACADABRA, upgr_font_size, (f32)start_upgradables_x_exis, (f32)upgradables_height_buffer, WHITE, false, false, TEXT, __VA_ARGS__);\
  upgradables_height_buffer += btw_space_gap;\
} else {\
  gui_label_format(FONT_TYPE_ABRACADABRA, upgr_font_size, (f32)start_upgradables_x_exis, (f32)upgradables_height_buffer, WHITE, false, false, TEXT, __VA_ARGS__);\
  upgradables_height_buffer += btw_space_gap;\
}}

void draw_in_game_upgrade_panel(u16 which_panel, Rectangle panel_dest) {
  const ability* upg = __builtin_addressof(state->ability_upgrade_choices.at(which_panel));
  const ability* abl = get_dynamic_player_state_ability(upg->type);
  if (upg->type <= ABILITY_TYPE_UNDEFINED || upg->type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_WARNING, "scene_in_game::draw_in_game_upgrade_panel()::Upgraded ability is out of bounds"); 
    return;
  }
  if (upg->level <= 0 || upg->level >= MAX_ABILITY_LEVEL) {
    TraceLog(LOG_WARNING, "scene_in_game::draw_in_game_upgrade_panel()::Upgraded ability level is out of bounds"); 
    return;
  }
  const u16 elm_space_gap = 30;
  const u16 btw_space_gap = 20;
  const u16 start_panel_height = panel_dest.y - panel_dest.height*.25f;
  const Rectangle icon_rect = {
    panel_dest.x - ABILITY_UPG_PANEL_ICON_SIZE/2.f,
    start_panel_height - ABILITY_UPG_PANEL_ICON_SIZE*.5f,
    ABILITY_UPG_PANEL_ICON_SIZE,ABILITY_UPG_PANEL_ICON_SIZE
  };
  const Vector2 ability_name_pos  = {panel_dest.x, start_panel_height + ABILITY_UPG_PANEL_ICON_SIZE*.5f + elm_space_gap};
  const Vector2 ability_level_ind = {panel_dest.x, ability_name_pos.y + btw_space_gap};

  const u16 title_font_size = 1; 
  const u16 level_ind_font_size = 1; 
  const u16 upgr_font_size = 1; 

  gui_draw_texture_id_pro(TEX_ID_ASSET_ATLAS, upg->icon_src, icon_rect);
  gui_label(upg->display_name.c_str(), FONT_TYPE_ABRACADABRA, title_font_size, ability_name_pos, WHITE, true, true);

  if (upg->level == 1) {
    gui_label("NEW!", FONT_TYPE_ABRACADABRA, level_ind_font_size, ability_level_ind, WHITE, true, true);
  } else if(upg->level>1 && upg->level <= MAX_ABILITY_LEVEL) {
    gui_label_format_v(FONT_TYPE_ABRACADABRA, level_ind_font_size, ability_level_ind, WHITE, true, true, "%d -> %d", abl->level, upg->level);
  } else {
    TraceLog(LOG_ERROR, "scene_in_game::draw_in_game_upgrade_panel()::Ability level is out of bound");
    return;
  }
  const u16 start_upgradables_x_exis = panel_dest.x - panel_dest.width*.5f + elm_space_gap;
  u16 upgradables_height_buffer = ability_level_ind.y + elm_space_gap;
  for (int i=0; i<ABILITY_UPG_MAX; ++i) {
    ability_upgradables abl_upg = upg->upgradables.at(i);
    if (abl_upg <= ABILITY_UPG_UNDEFINED  || abl_upg >= ABILITY_UPG_MAX) {
      break;
    }
    switch (abl_upg) {
      case ABILITY_UPG_DAMAGE: DRAW_ABL_UPG_STAT_PNL(upg, "%s%d",    lc_txt(LOC_TEXT_INGAME_UPGRADE_ABILITY_DAMAGE), abl->base_damage);  break;
      case ABILITY_UPG_AMOUNT: DRAW_ABL_UPG_STAT_PNL(upg, "%s%d",    lc_txt(LOC_TEXT_INGAME_UPGRADE_ABILITY_AMOUTH), abl->proj_count);   break;
      case ABILITY_UPG_HITBOX: DRAW_ABL_UPG_STAT_PNL(upg, "%s%.1f",  lc_txt(LOC_TEXT_INGAME_UPGRADE_ABILITY_HITBOX), abl->proj_dim.x); break;
      case ABILITY_UPG_SPEED:  DRAW_ABL_UPG_STAT_PNL(upg, "%s%d",    lc_txt(LOC_TEXT_INGAME_UPGRADE_ABILITY_SPEED),  abl->proj_speed);   break;
      default: {
        TraceLog(LOG_WARNING, "scene_in_game::draw_in_game_upgrade_panel()::Unsupported ability upgrade type");
        break;
      }
    }
  }
}
void draw_passive_selection_panel(const character_stat* stat, const Rectangle panel_dest) {
  if (stat && (stat->id <= CHARACTER_STATS_UNDEFINED || stat->id >= CHARACTER_STATS_MAX)) {
    TraceLog(LOG_WARNING, "scene_in_game::draw_passive_selection_panel()::Character stat id is out of bound"); 
    return;
  }
  const u16 elm_space_gap = 30;
  const u16 start_panel_height = panel_dest.y - panel_dest.height*.25f;
  const Rectangle icon_rect = {
    panel_dest.x - PASSIVE_SELECTION_PANEL_ICON_SIZE/2.f,
    start_panel_height - PASSIVE_SELECTION_PANEL_ICON_SIZE*.5f,
    PASSIVE_SELECTION_PANEL_ICON_SIZE,PASSIVE_SELECTION_PANEL_ICON_SIZE
  };
  const Vector2 passive_name_pos  = {panel_dest.x, start_panel_height + PASSIVE_SELECTION_PANEL_ICON_SIZE*.5f + elm_space_gap};

  const u16 title_font_size = 1; 
  
  gui_draw_texture_id_pro(TEX_ID_ASSET_ATLAS, stat->passive_icon_src, icon_rect);
  gui_label(lc_txt(stat->passive_display_name_symbol), FONT_TYPE_ABRACADABRA, title_font_size, passive_name_pos, WHITE, true, true);
  
  const u16 desc_font_size = 1;
  const f32 desc_box_height = panel_dest.y + (panel_dest.height * .5f) - passive_name_pos.y - elm_space_gap;
  const Vector2 padding = { SCREEN_OFFSET.x, elm_space_gap };
  const Rectangle desc_box = {
    .x      = panel_dest.x - (panel_dest.width * .5f) + padding.x,
    .y      = passive_name_pos.y + padding.y,
    .width  = panel_dest.width - padding.x,
    .height = desc_box_height - padding.y,
  };
  gui_label_wrap(lc_txt(stat->passive_desc_symbol), FONT_TYPE_ABRACADABRA, desc_font_size, desc_box, WHITE, false);

}
void draw_end_game_panel() {
  STATE_ASSERT("draw_end_game_panel")
  gui_panel(state->default_panel, Rectangle{ 
      SIG_BASE_RENDER_WIDTH * .5f, SIG_BASE_RENDER_HEIGHT * .5f, 
      SIG_BASE_RENDER_WIDTH * .75f, SIG_BASE_RENDER_HEIGHT * .75f
    }, 
    true
  );

  if (state->end_game_result.is_win) {
    gui_label(lc_txt(LOC_TEXT_INGAME_STAGE_RESULT_CLEARED), FONT_TYPE_ABRACADABRA, 1, SIG_BASE_RENDER_DIV2, WHITE, true, true);
  }
  else {
    gui_label(lc_txt(LOC_TEXT_INGAME_STAGE_RESULT_DEAD), FONT_TYPE_ABRACADABRA, 1, SIG_BASE_RENDER_DIV2, RED, true, true);
  }
  u32 min  = (i32)state->end_game_result.play_time/60;
  u32 secs = (i32)state->end_game_result.play_time%60;
  gui_label_format_v(FONT_TYPE_ABRACADABRA, 1, VECTOR2(static_cast<f32>(state->in_app_settings->render_width_div2), SIG_BASE_RENDER_HEIGHT * .55f), 
    WHITE, true, true, "%d:%d", min, secs
  );

  gui_label_format_v(FONT_TYPE_ABRACADABRA, 1, VECTOR2(static_cast<f32>(state->in_app_settings->render_width_div2), SIG_BASE_RENDER_HEIGHT * .75f), WHITE, true, true, 
    "%s%d", lc_txt(LOC_TEXT_INGAME_STAGE_RESULT_COLLECTED_SOULS), state->end_game_result.collected_souls
  );
}
void prepare_ability_upgrade_state(void) {
  set_is_game_paused(true);
  for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
    panel* pnl = __builtin_addressof(state->ability_upg_panels.at(i));
    ability* pnl_slot = __builtin_addressof(state->ability_upgrade_choices.at(i));
    if(pnl->buffer.u16[0] <= 0 || pnl->buffer.u16[0] >= ABILITY_TYPE_MAX) {
      pnl->buffer.u16[0] = get_random(ABILITY_TYPE_UNDEFINED+1,ABILITY_TYPE_MAX-1);
    }
    const ability* player_ability = get_dynamic_player_state_ability(static_cast<ability_type>(pnl->buffer.u16[0])); // INFO: do we need upgrade the ability player already have
    if (player_ability->type <= ABILITY_TYPE_UNDEFINED || player_ability->type >= ABILITY_TYPE_MAX) {
      *pnl_slot = _get_ability(static_cast<ability_type>(pnl->buffer.u16[0]));
    }
    else {
      *pnl_slot = _get_next_level(*player_ability);
    }
    if (pnl_slot->type <= ABILITY_TYPE_UNDEFINED || pnl_slot->type >= ABILITY_TYPE_MAX) {
      TraceLog(LOG_WARNING, "scene_in_game::make_passive_selection_panel_ready()::Ability didn't registered yet"); 
      continue;
    }
  }
  state->is_upgrade_choices_ready = true;
}
void end_ability_upgrade_state(u16 which_panel_chosen) {
  ability* new_ability = __builtin_addressof(state->ability_upgrade_choices.at(which_panel_chosen));
  if (new_ability->level >= MAX_ABILITY_LEVEL || new_ability->level <= 1) {
    _add_ability(new_ability->type);
  }
  else {
    _upgrade_ability(get_dynamic_player_state_ability(new_ability->type)); 
  }

  set_is_game_paused(false);
  state->ability_upgrade_choices.fill(ability());
  set_dynamic_player_have_ability_upgrade_points(false);
  state->is_upgrade_choices_ready = false;
  for (size_t itr_000 = 0; itr_000 < MAX_UPDATE_ABILITY_PANEL_COUNT; ++itr_000) {
    state->ability_upg_panels.at(itr_000).buffer.u16[0] = 0;
  }
}

bool scene_in_game_on_event(i32 code, event_context context) {
  switch (code) {
    case EVENT_CODE_ADD_CURRENCY_SOULS: {
      state->end_game_result.collected_souls += context.data.u32[0];
      return true;
    }
    case EVENT_CODE_RESUME_GAME: {
      set_is_game_paused(false);
      state->show_pause_menu = false;
      return true;
    }
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::scene_in_game_on_event()::Unsuppported code.");
      return false;
    }
  }

  TraceLog(LOG_WARNING, "scene_in_game::scene_in_game_on_event()::event handling ended unexpectedly");
  return false;
}

#undef STATE_ASSERT
#undef ABILITY_UPG_PANEL_ICON_SIZE
#undef CLOUDS_ANIMATION_DURATION
#undef FADE_ANIMATION_DURATION
#undef DRAW_ABL_UPG_STAT_PNL
#undef SIG_BASE_RENDER_SCALE
#undef SIG_BASE_RENDER_DIV2
#undef SIG_BASE_RENDER_RES_VEC
#undef SIG_BASE_RENDER_WIDTH
#undef SIG_BASE_RENDER_HEIGHT
#undef SIG_SCREEN_POS
