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
  IN_GAME_STAGE_IDLE,
  IN_GAME_STAGE_PLAY,
  IN_GAME_STAGE_PAUSE,
  IN_GAME_STAGE_PLAY_DEBUG,
  IN_GAME_STAGE_PLAY_RESULTS,
  IN_GAME_STAGE_MAX,
} in_game_stages;

typedef struct scene_in_game_state {
  std::array<worldmap_stage, MAX_WORLDMAP_LOCATIONS>  worldmap_locations;
  std::array<panel, MAX_UPDATE_ABILITY_PANEL_COUNT> ability_upg_panels;
  std::array<ability, MAX_UPDATE_ABILITY_PANEL_COUNT> ability_upgrade_choices;
  panel default_panel;
  panel debug_info_panel;
  bool is_upgrade_choices_ready;
  
  const camera_metrics* in_camera_metrics;
  const app_settings* in_app_settings;
  const ingame_info* in_ingame_info;
  
  in_game_stages stage;
  i32 hovered_spawn;
  ability_type hovered_ability;
  i32 hovered_projectile;
  ui_fade_control_system sig_fade;

  scene_in_game_state(void) {
    this->worldmap_locations.fill(worldmap_stage());
    this->ability_upg_panels.fill(panel());
    this->ability_upgrade_choices.fill(ability());
    this->default_panel = panel();
    this->debug_info_panel = panel();
    this->is_upgrade_choices_ready = false;
    this->in_camera_metrics = nullptr;
    this->in_app_settings = nullptr;
    this->in_ingame_info = nullptr;
    this->stage = IN_GAME_STAGE_UNDEFINED;
    this->hovered_spawn = I32_MAX;
    this->hovered_ability = ABILITY_TYPE_UNDEFINED;
    this->hovered_projectile = I32_MAX;
    this->sig_fade = ui_fade_control_system();
  }
} scene_in_game_state;

static scene_in_game_state * state = nullptr;

#define STATE_ASSERT(FUNCTION) if (!state) {                                              \
  TraceLog(LOG_ERROR, "scene_in_game::" FUNCTION "::In game state was not initialized");  \
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
#define GET_PLAYER_DYNAMIC_STAT(STAT_TYPE) __builtin_addressof(state->in_ingame_info->player_state_dynamic->stats[(static_cast<character_stats>(STAT_TYPE))])
#define GET_PLAYER_DYNAMIC_ABILITY(ABILITY_TYPE) __builtin_addressof(state->in_ingame_info->player_state_dynamic->ability_system.abilities.at(ABILITY_TYPE))
#define INGAME_FADE_DURATION 1 * TARGET_FPS
#define DRAW_ABL_UPG_STAT_PNL(UPG, TEXT, ...){ \
if (UPG->level == 1) {\
  gui_label_format(FONT_TYPE_ABRACADABRA, upgr_font_size, (f32)start_upgradables_x_exis, (f32)upgradables_height_buffer, WHITE, false, false, TEXT, __VA_ARGS__);\
  upgradables_height_buffer += btw_space_gap;\
} else {\
  gui_label_format(FONT_TYPE_ABRACADABRA, upgr_font_size, (f32)start_upgradables_x_exis, (f32)upgradables_height_buffer, WHITE, false, false, TEXT, __VA_ARGS__);\
  upgradables_height_buffer += btw_space_gap;\
}}
#define HEALTH_BAR_WIDTH static_cast<f32>(state->in_app_settings->render_width) * .15f

[[__nodiscard__]] bool begin_scene_in_game(bool fade_in);

bool scene_in_game_on_event(i32 code, event_context context);
void in_game_update_bindings(void);
void in_game_update_mouse_bindings(void);
void in_game_update_keyboard_bindings(void);
void initialize_worldmap_locations(void);
void draw_in_game_upgrade_panel(u16 which_panel, Rectangle panel_dest);
void draw_passive_selection_panel(const character_stat* stat,const Rectangle panel_dest);
void draw_end_game_panel(void);
void draw_ingame_stage_play_ui(void);
void prepare_ability_upgrade_state(void);
void end_ability_upgrade_state(u16 which_panel_chosen);
void sig_begin_fade(void);

bool start_game(void);
void reset_game(void);

/**
 * @brief Requires world system, world init moved to app, as well as its loading time
 */
[[__nodiscard__]] bool initialize_scene_in_game(const app_settings * _in_app_settings, bool fade_in) {
  if (state) {
    return begin_scene_in_game(fade_in);
  }
  state = (scene_in_game_state *)allocate_memory_linear(sizeof(scene_in_game_state), true);
  if (state == nullptr) {
    TraceLog(LOG_ERROR, "scene_in_game::initialize_scene_in_game()::State allocation failed!");
    return false;
  }
  *state = scene_in_game_state();

  state->in_app_settings = _in_app_settings;
  if (!_in_app_settings || _in_app_settings == nullptr) {
    TraceLog(LOG_ERROR, "scene_in_game::initialize_scene_in_game()::App setting pointer is invalid");
    return false;
  }

  if(!create_camera(state->in_app_settings->render_width_div2, state->in_app_settings->render_height_div2, state->in_app_settings->render_width, state->in_app_settings->render_height)) {
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

  return begin_scene_in_game(fade_in);
}

[[__nodiscard__]] bool begin_scene_in_game(bool fade_in) {
  if (!state || state == nullptr) {
    TraceLog(LOG_ERROR, "scene_in_game::begin_scene_in_game()::State is not valid");
    return false;
  }
  state->default_panel = panel( BTN_STATE_RELEASED, ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG, ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL, 
    Vector4 {6, 6, 6, 6}, Color { 30, 39, 46, 245}, Color { 52, 64, 76, 245}
  );
  copy_memory(state->worldmap_locations.data(), get_worldmap_locations(), MAX_WORLDMAP_LOCATIONS * sizeof(worldmap_stage));
  _set_player_position(Vector2 {0.f, 0.f});
  event_fire(EVENT_CODE_PAUSE_GAME, event_context());
  
  for (size_t itr_000 = 0; itr_000 < MAX_UPDATE_ABILITY_PANEL_COUNT; ++itr_000) {
    state->ability_upg_panels.at(itr_000) = state->default_panel;
  }
  state->debug_info_panel = state->default_panel;

  state->hovered_spawn = U16_MAX;
  state->hovered_ability = ABILITY_TYPE_MAX;
  state->hovered_projectile = U16_MAX;
  state->stage = IN_GAME_STAGE_IDLE;

  if (fade_in) {
    sig_begin_fade();
  }
  return true;
}
bool start_game(void) {
  if (!state || state == nullptr) {
    TraceLog(LOG_ERROR, "scene_in_game::start_game()::State is not valid");
    return false;
  }
  if(!gm_start_game(*get_active_worldmap())) {
    return false;
  }
  
  
  _set_player_position(ZEROVEC2);
  state->stage = IN_GAME_STAGE_PLAY;
  return true;
}
void end_scene_in_game(void) {
  if (!state || state == nullptr ) {
    return;
  }

  gm_end_game(state->in_ingame_info->is_win);
  state->hovered_spawn = U16_MAX;
  state->hovered_ability = ABILITY_TYPE_MAX;
  state->hovered_projectile = U16_MAX;
}

void update_scene_in_game(void) {
  STATE_ASSERT("update_scene_in_game")

  update_camera();
  in_game_update_bindings();
  update_user_interface();
  if(state->sig_fade.fade_animation_playing){
    process_fade_effect(__builtin_addressof(state->sig_fade));
  }

  switch (state->stage) {
    case IN_GAME_STAGE_IDLE: { break; }
    case IN_GAME_STAGE_PAUSE: { break; }
    case IN_GAME_STAGE_PLAY: {
      event_fire(EVENT_CODE_CAMERA_SET_TARGET, event_context(
        state->in_ingame_info->player_state_dynamic->position_centered.x,
        state->in_ingame_info->player_state_dynamic->position_centered.y
      ));
      if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyReleased(KEY_D)) {
        state->stage = IN_GAME_STAGE_PLAY_DEBUG;
      }
      update_map();
      update_game_manager();

      if ( (*state->in_ingame_info->is_game_end) ) {
        state->stage = IN_GAME_STAGE_PLAY_RESULTS;
      }
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
    case IN_GAME_STAGE_IDLE: {  
      render_map(); 
      //render_game();
      break; 
    }
    case IN_GAME_STAGE_PAUSE: { 
      render_map(); 
      render_game();
      break; 
    }
    case IN_GAME_STAGE_PLAY: {
      render_map();
      const ingame_info* const iginf = gm_get_ingame_info();
      i32 bottom_of_the_screen = state->in_camera_metrics->frustum.y + state->in_camera_metrics->frustum.height;
      i32 top_of_the_screen = state->in_camera_metrics->frustum.y;
      i32 player_texture_begin = iginf->player_state_dynamic->position.y + iginf->player_state_dynamic->dimentions.y;
      
      _render_props_y_based(top_of_the_screen, player_texture_begin);
      render_game();
      _render_props_y_based(player_texture_begin, bottom_of_the_screen);
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
    case IN_GAME_STAGE_IDLE: { 
      gui_label("Press Space to Start!", FONT_TYPE_ABRACADABRA, 1, Vector2 {SIG_BASE_RENDER_WIDTH * .5f, SIG_BASE_RENDER_HEIGHT * .75f}, WHITE, true, true);
      render_user_interface();
      return; 
    }
    case IN_GAME_STAGE_PAUSE: {
      gui_draw_pause_screen(true); // true for drawing resume button
      render_user_interface();
      return; 
    }
    case IN_GAME_STAGE_PLAY: {  
      if (get_b_player_have_upgrade_points()) {
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
        draw_ingame_stage_play_ui();
      }
      render_user_interface();
      return;
    }
    case IN_GAME_STAGE_PLAY_DEBUG: {
      const Vector2& mouse_pos_screen = (*state->in_ingame_info->mouse_pos_screen);
      gui_label_format(
        FONT_TYPE_ABRACADABRA, 1, mouse_pos_screen.x, mouse_pos_screen.y, 
        WHITE, false, false, "world_pos {%.1f, %.1f}", state->in_ingame_info->mouse_pos_world->x, state->in_ingame_info->mouse_pos_world->y
      );
      if(static_cast<size_t>(state->hovered_spawn) < state->in_ingame_info->in_spawns->size()){
          const Character2D* const spawn = __builtin_addressof(state->in_ingame_info->in_spawns->at(state->hovered_spawn));
          panel* const pnl = __builtin_addressof(state->debug_info_panel);
          pnl->dest = Rectangle {
            mouse_pos_screen.x, mouse_pos_screen.y, 
            SIG_BASE_RENDER_WIDTH * .4f, SIG_BASE_RENDER_HEIGHT * .3f
          };
          i32 font_size = 1;
          i32 line_height = SIG_BASE_RENDER_HEIGHT * .05f;
          Vector2 debug_info_position_buffer = VECTOR2(pnl->dest.x, pnl->dest.y);

          gui_panel((*pnl), pnl->dest, false);
          BeginScissorMode(pnl->dest.x, pnl->dest.y, pnl->dest.width, pnl->dest.height);
          {
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
          }
          EndScissorMode();
      }
      if(state->hovered_ability > 0 && state->hovered_ability < ABILITY_TYPE_MAX) {
          const ability* const abl = GET_PLAYER_DYNAMIC_ABILITY(state->hovered_ability);
          panel* const pnl = __builtin_addressof(state->debug_info_panel);
          pnl->dest = Rectangle {
            mouse_pos_screen.x, mouse_pos_screen.y, 
            SIG_BASE_RENDER_WIDTH * .4f, SIG_BASE_RENDER_HEIGHT * .3f
          };
          i32 font_size = 1;
          i32 line_height = SIG_BASE_RENDER_HEIGHT * .05f;
          Vector2 debug_info_position_buffer = VECTOR2(pnl->dest.x, pnl->dest.y);
          if (state->hovered_projectile >= 0 && static_cast<size_t>(state->hovered_projectile) < abl->projectiles.size()) {
            const projectile* const prj = __builtin_addressof(abl->projectiles.at(state->hovered_projectile));

            gui_panel((*pnl), pnl->dest, false);
            BeginScissorMode(pnl->dest.x, pnl->dest.y, pnl->dest.width, pnl->dest.height);
            {
              gui_label_format(
                FONT_TYPE_ABRACADABRA, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
                WHITE, false, false, "Collision: {%.1f, %.1f, %.1f, %.1f}", prj->collision.x, prj->collision.y, prj->collision.width, prj->collision.height
              );
              debug_info_position_buffer.y += line_height;
              gui_label_format(
                FONT_TYPE_ABRACADABRA, font_size, debug_info_position_buffer.x, debug_info_position_buffer.y, 
                WHITE, false, false, "Rotation: %.1f", prj->animations.at(prj->active_sprite).rotation
              );
            }
            EndScissorMode();
          }
      }
      gui_label_format(FONT_TYPE_ABRACADABRA, 1, SIG_BASE_RENDER_WIDTH * .75f, SCREEN_OFFSET.y, WHITE, false, false, 
        "Remaining: %d", state->in_ingame_info->in_spawns->size()
      );
      gui_label_format(FONT_TYPE_ABRACADABRA, 1, SIG_BASE_RENDER_WIDTH * .75f, SCREEN_OFFSET.y * 5.f, WHITE, false, false, 
        "Souls: %d", get_currency_souls()
      );
      gui_label_format(FONT_TYPE_ABRACADABRA, 1, 0, SIG_BASE_RENDER_HEIGHT * .35f, WHITE, false, false, 
        "Health: %d", state->in_ingame_info->player_state_dynamic->stats_base.at(CHARACTER_STATS_HEALTH).buffer.i32[0]
      );
      gui_label_format(FONT_TYPE_ABRACADABRA, 1, 0, SIG_BASE_RENDER_HEIGHT * .40f, WHITE, false, false, 
        "Current Health: %d", state->in_ingame_info->player_state_dynamic->health_current
      );
      gui_label_format(FONT_TYPE_ABRACADABRA, 1, 0, SIG_BASE_RENDER_HEIGHT * .45f, WHITE, false, false, 
        "Damage: %d", state->in_ingame_info->player_state_dynamic->stats_base.at(CHARACTER_STATS_DAMAGE).buffer.i32[0]
      );
      
      render_user_interface();
      return;
    }
    case IN_GAME_STAGE_PLAY_RESULTS: { 
      draw_end_game_panel();
      if(gui_menu_button("Accept", BTN_ID_IN_GAME_BUTTON_RETURN_MENU, Vector2 {0, 5}, SIG_BASE_RENDER_DIV2, true)) {
        gm_save_game();
        end_scene_in_game();
        event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context(static_cast<i32>(false)));
      }
      render_user_interface();
      return; 
    }
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::render_interface_in_game()::Unsupported stage");
      render_user_interface();
      return;
    }
  }

  TraceLog(LOG_WARNING, "scene_in_game::render_interface_in_game()::Function ended unexpectedly");
}
void in_game_update_mouse_bindings(void) { 
  switch (state->stage) {
    case IN_GAME_STAGE_IDLE: { break; }
    case IN_GAME_STAGE_PAUSE: { break; }
    case IN_GAME_STAGE_PLAY: {
      break;
    }
    case IN_GAME_STAGE_PLAY_DEBUG: {
      const Vector2* const mouse_pos_world = state->in_ingame_info->mouse_pos_world;
      state->hovered_spawn = U16_MAX;
      size_t _remaining_enemies = state->in_ingame_info->in_spawns->size();
      for (size_t itr_000 = 0; itr_000 < _remaining_enemies; ++itr_000) {
        const Character2D* const spawn = __builtin_addressof(state->in_ingame_info->in_spawns->at(itr_000));
        if (spawn) {
          if (CheckCollisionPointRec(*mouse_pos_world, spawn->collision)) {
            state->hovered_spawn = itr_000;
          }
        }
      }
      const player_state* const player = state->in_ingame_info->player_state_dynamic;
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
    case IN_GAME_STAGE_IDLE: { 
      if (IsKeyReleased(KEY_ESCAPE)) {
        event_fire(EVENT_CODE_TOGGLE_GAME_PAUSE, event_context());
      }
      else if (IsKeyPressed(KEY_SPACE) && state->in_ingame_info->is_game_paused) {
        event_fire(EVENT_CODE_RESUME_GAME, event_context());
        state->stage = IN_GAME_STAGE_PLAY;
        start_game();
      }
      break; 
    }
    case IN_GAME_STAGE_PAUSE: { 
      if (IsKeyReleased(KEY_ESCAPE)) {
        event_fire(EVENT_CODE_RESUME_GAME, event_context());
        state->stage = IN_GAME_STAGE_PLAY;
      }
      break; 
    }
    case IN_GAME_STAGE_PLAY: {  
      if (IsKeyReleased(KEY_ESCAPE)) {
        event_fire(EVENT_CODE_PAUSE_GAME, event_context());
        state->stage = IN_GAME_STAGE_PAUSE;
      }
      break;
    }
    case IN_GAME_STAGE_PLAY_DEBUG: {  
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

void draw_in_game_upgrade_panel(u16 which_panel, Rectangle panel_dest) {
  const ability* upg = __builtin_addressof(state->ability_upgrade_choices.at(which_panel));
  if (upg->type < 0 && upg->type >= state->in_ingame_info->player_state_dynamic->ability_system.abilities.size()) {
    return;
  }
  const ability* abl = GET_PLAYER_DYNAMIC_ABILITY(upg->type);
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
void draw_end_game_panel(void) {
  STATE_ASSERT("draw_end_game_panel")
  gui_panel(state->default_panel, Rectangle{ 
      SIG_BASE_RENDER_WIDTH * .5f, SIG_BASE_RENDER_HEIGHT * .5f, 
      SIG_BASE_RENDER_WIDTH * .75f, SIG_BASE_RENDER_HEIGHT * .75f
    }, 
    true
  );

  if (state->in_ingame_info->is_win) {
    gui_label(lc_txt(LOC_TEXT_INGAME_STAGE_RESULT_CLEARED), FONT_TYPE_ABRACADABRA, 1, SIG_BASE_RENDER_DIV2, WHITE, true, true);
  }
  else {
    gui_label(lc_txt(LOC_TEXT_INGAME_STAGE_RESULT_DEAD), FONT_TYPE_ABRACADABRA, 1, SIG_BASE_RENDER_DIV2, RED, true, true);
  }
  u32 min  = (i32)state->in_ingame_info->play_time / 60.f;
  u32 secs = (i32)state->in_ingame_info->play_time % 60;
  gui_label_format_v(FONT_TYPE_ABRACADABRA, 1, VECTOR2(static_cast<f32>(state->in_app_settings->render_width_div2), SIG_BASE_RENDER_HEIGHT * .55f), 
    WHITE, true, true, "%d:%d", min, secs
  );

  gui_label_format_v(FONT_TYPE_ABRACADABRA, 1, VECTOR2(static_cast<f32>(state->in_app_settings->render_width_div2), SIG_BASE_RENDER_HEIGHT * .75f), WHITE, true, true, 
    "%s%d", lc_txt(LOC_TEXT_INGAME_STAGE_RESULT_COLLECTED_SOULS), state->in_ingame_info->collected_souls
  );
}
void draw_ingame_stage_play_ui(void) {
  Rectangle exp_bar_dest = Rectangle {0, 0, static_cast<f32>(state->in_app_settings->render_width), static_cast<f32>(state->in_app_settings->render_height) * .05f};
  gui_progress_bar(PRG_BAR_ID_PLAYER_EXPERIANCE, exp_bar_dest, false);
  const Rectangle * exp_bar_orn = get_atlas_texture_source_rect(ATLAS_TEX_ID_DARK_FANTASY_BOSSBAR_6_MIDDLE);
  f32 exp_bar_orn_height_scale = exp_bar_orn->width / exp_bar_orn->height;
  Rectangle exp_bar_orn_dest = Rectangle { exp_bar_dest.x + exp_bar_dest.width * .5f,  exp_bar_dest.y,  exp_bar_dest.height * exp_bar_orn_height_scale,  exp_bar_dest.height};
  gui_draw_atlas_texture_id(ATLAS_TEX_ID_DARK_FANTASY_BOSSBAR_6_MIDDLE, exp_bar_orn_dest, Vector2{exp_bar_orn_dest.width * .5f, 0.f}, 0.f);

  f32 render_width_scale = static_cast<f32>(state->in_app_settings->render_width) / static_cast<f32>(state->in_app_settings->render_height);
  Rectangle portrait_dest = Rectangle {
    static_cast<f32>(state->in_app_settings->render_width)  * .025f,
    static_cast<f32>(state->in_app_settings->render_height) * .025f * render_width_scale,
    static_cast<f32>(state->in_app_settings->render_height) * .15f,
    static_cast<f32>(state->in_app_settings->render_height) * .15f
  };
  gui_draw_atlas_texture_id(ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG, portrait_dest, ZEROVEC2, 0.f, Color {121, 58, 128, 150});
  gui_draw_atlas_texture_id(ATLAS_TEX_ID_INQUISITOR_PORTRAIT, portrait_dest, ZEROVEC2, 0.f);
  gui_draw_atlas_texture_id(ATLAS_TEX_ID_PORTRAIT_FRAME, portrait_dest, ZEROVEC2, 0.f);
    
  f32 slot_gap = portrait_dest.width * .075f;
  gui_progress_bar(PRG_BAR_ID_PLAYER_HEALTH, Rectangle {
      portrait_dest.x + portrait_dest.width + slot_gap,
      portrait_dest.y + portrait_dest.height * .65f,
      HEALTH_BAR_WIDTH,
      static_cast<f32>(state->in_app_settings->render_height) * .02f,
    }, false
  );

  for (size_t itr_000 = 0; itr_000 < state->in_ingame_info->player_state_dynamic->ability_system.abilities.size(); ++itr_000) {
    const ability* const abl = __builtin_addressof(state->in_ingame_info->player_state_dynamic->ability_system.abilities.at(itr_000));
    if (!abl->is_active) continue;
    
    Rectangle ability_slot_dest = Rectangle {
      portrait_dest.x + portrait_dest.width + slot_gap + (slot_gap * itr_000),
      portrait_dest.y,
      portrait_dest.width * .25f,
      portrait_dest.height * .25f
    };
    gui_draw_atlas_texture_id(ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG, ability_slot_dest, ZEROVEC2, 0.f, Color {121, 58, 128, 150});
    gui_draw_texture_id_pro(TEX_ID_ASSET_ATLAS, abl->icon_src, ability_slot_dest);
    gui_draw_atlas_texture_id(ATLAS_TEX_ID_ABILITY_SLOT_FRAME, ability_slot_dest, ZEROVEC2, 0.f);
  }
}
void prepare_ability_upgrade_state(void) {
  event_fire(EVENT_CODE_PAUSE_GAME, event_context());
  for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
    panel* pnl = __builtin_addressof(state->ability_upg_panels.at(i));
    ability* pnl_slot = __builtin_addressof(state->ability_upgrade_choices.at(i));
    if(pnl->buffer.u16[0] <= 0 || pnl->buffer.u16[0] >= ABILITY_TYPE_MAX) {
      pnl->buffer.u16[0] = get_random(ABILITY_TYPE_UNDEFINED+1,ABILITY_TYPE_MAX-1);
    }
    if (pnl->buffer.u16[0] < 0 || pnl->buffer.u16[0] >= state->in_ingame_info->player_state_dynamic->ability_system.abilities.size()) {
      continue;
    }
    const ability* player_ability = GET_PLAYER_DYNAMIC_ABILITY(static_cast<ability_type>(pnl->buffer.u16[0])); // INFO: do we need upgrade the ability player already have
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
    _upgrade_ability(GET_PLAYER_DYNAMIC_ABILITY(new_ability->type)); 
  }

  event_fire(EVENT_CODE_RESUME_GAME, event_context());
  state->ability_upgrade_choices.fill(ability());
  set_dynamic_player_have_ability_upgrade_points(false);
  state->is_upgrade_choices_ready = false;
  for (size_t itr_000 = 0; itr_000 < MAX_UPDATE_ABILITY_PANEL_COUNT; ++itr_000) {
    state->ability_upg_panels.at(itr_000).buffer.u16[0] = 0;
  }
}
void sig_begin_fade(void) {
  state->sig_fade.fade_animation_duration = INGAME_FADE_DURATION;
  state->sig_fade.fade_type = FADE_TYPE_FADEIN;
  state->sig_fade.fade_animation_timer = 0.f;
  state->sig_fade.fade_animation_playing = true;
  state->sig_fade.is_fade_animation_played = false;
}

bool scene_in_game_on_event(i32 code, [[maybe_unused]] event_context context) {
  switch (code) {
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::scene_in_game_on_event()::Unsuppported code.");
      return false;
    }
  }

  TraceLog(LOG_WARNING, "scene_in_game::scene_in_game_on_event()::event handling ended unexpectedly");
  return false;
}

#undef STATE_ASSERT
#undef SIG_BASE_RENDER_SCALE
#undef SIG_BASE_RENDER_DIV2
#undef SIG_BASE_RENDER_WIDTH
#undef SIG_BASE_RENDER_HEIGHT
#undef SIG_SCREEN_POS
#undef ABILITY_UPG_PANEL_ICON_SIZE
#undef PASSIVE_SELECTION_PANEL_ICON_SIZE
#undef CLOUDS_ANIMATION_DURATION
#undef GET_PLAYER_DYNAMIC_STAT
#undef GET_PLAYER_DYNAMIC_ABILITY
#undef INGAME_FADE_DURATION
#undef DRAW_ABL_UPG_STAT_PNL
#undef HEALTH_BAR_WIDTH
