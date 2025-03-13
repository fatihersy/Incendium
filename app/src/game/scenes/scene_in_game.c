#include "scene_in_game.h"
#include <reasings.h>
#include <settings.h>

#include <core/ftime.h>
#include <core/event.h>
#include <core/fmath.h>
#include <core/fmemory.h>

#include "game/world.h"
#include "game/game_manager.h"
#include "game/user_interface.h"

typedef enum in_game_stages {
  IN_GAME_STAGE_UNDEFINED,
  IN_GAME_STAGE_MAP_CHOICE,
  IN_GAME_STAGE_PASSIVE_CHOICE,
  IN_GAME_STAGE_PLAY,
  IN_GAME_STAGE_MAX,
} in_game_stages;

typedef struct scene_in_game_state {
  panel ability_upg_panels[MAX_UPDATE_ABILITY_PANEL_COUNT];
  panel passive_selection_panels[MAX_UPDATE_PASSIVE_PANEL_COUNT];
  worldmap_stage worldmap_locations[MAX_WORLDMAP_LOCATIONS];
  panel worldmap_selection_panel;
  in_game_stages stage;

  u16 clouds_animation_timer;
  u16 hovered_stage;

  bool has_game_started;
  bool clouds_animation_playing;
} scene_in_game_state;

static scene_in_game_state *restrict state;

#define STATE_ASSERT(FUNCTION) if (!state) {                                              \
  TraceLog(LOG_ERROR, "scene_in_game::" FUNCTION "::In game state was not initialized");  \
  event_fire(EVENT_CODE_SCENE_MAIN_MENU, (event_context){0});                             \
  return;                                                                                 \
}
#define DRAW_ABL_UPG_STAT_PNL(ABL, UPG, TEXT, STAT){ \
if (UPG.level == 1) {\
  gui_label_format(FONT_TYPE_MINI_MOOD, upgr_font_size, start_upgradables_x_exis, upgradables_height_buffer, WHITE, false, TEXT"%d", UPG.base_damage);\
  upgradables_height_buffer += btw_space_gap;\
} else if(UPG.level>1 && UPG.level <= MAX_ABILITY_LEVEL) {\
  gui_label_format(FONT_TYPE_MINI_MOOD, upgr_font_size, start_upgradables_x_exis, upgradables_height_buffer, WHITE, false, TEXT"%d -> %d", ABL->base_damage, UPG.base_damage);\
  upgradables_height_buffer += btw_space_gap;\
}}

#define ABILITY_UPG_PANEL_ICON_SIZE get_resolution_div4()->x*.5f
#define PASSIVE_SELECTION_PANEL_ICON_SIZE ABILITY_UPG_PANEL_ICON_SIZE
#define CLOUDS_ANIMATION_DURATION TARGET_FPS * 1.5f // second

void in_game_update_bindings(void);
void in_game_update_mouse_bindings(void);
void in_game_update_keyboard_bindings(void);
void initialize_worldmap_locations(void);
void start_game(void);
void draw_upgrade_panel(ability* abl, ability upg, Rectangle panel_dest);
void draw_passive_selection_panel(character_stat* stat, Rectangle panel_dest);

/**
 * @brief Requires world system, world init moved to app as well as its loading time
 */
bool initialize_scene_in_game(camera_metrics* _camera_metrics) {
  if (state) {
    TraceLog(LOG_ERROR, "scene_in_game::initialize_scene_in_game()::Initialize called twice");
    return false;
  }

  state = (scene_in_game_state *)allocate_memory_linear(sizeof(scene_in_game_state), true);

  copy_memory(&state->worldmap_locations, get_worldmap_locations(), sizeof(state->worldmap_locations));

  if (!game_manager_initialize(_camera_metrics)) { // Inits player & spawns
    TraceLog(LOG_ERROR, "scene_in_game::initialize_scene_in_game()::game_manager_initialize() failed");
    return false;
  }
  _set_player_position(*get_resolution_div2());
  
  panel default_panel = (panel) {
    .signal_state  = BTN_STATE_RELEASED,
    .bg_tex_id     = TEX_ID_CRIMSON_FANTASY_PANEL_BG,
    .frame_tex_id  = TEX_ID_CRIMSON_FANTASY_PANEL,
    .bg_tint       = (Color) { 30, 39, 46, 245},
    .bg_hover_tint = (Color) { 52, 64, 76, 245},
    .offsets = (Vector4) {6, 6, 6, 6},
  };
  for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
    state->ability_upg_panels[i] = default_panel;
  }
  for (int i=0; i<MAX_UPDATE_PASSIVE_PANEL_COUNT; ++i) {
    state->passive_selection_panels[i] = default_panel;
  }
  state->worldmap_selection_panel = default_panel;

  set_is_game_paused(true);
  state->stage = IN_GAME_STAGE_PASSIVE_CHOICE;
  state->clouds_animation_playing = false;
  state->hovered_stage = U16_MAX;
  event_fire(EVENT_CODE_UI_START_FADEIN_EFFECT, (event_context){ .data.u16[0] = CLOUDS_ANIMATION_DURATION });
  return true;
}

void update_scene_in_game(void) {
  STATE_ASSERT("update_scene_in_game")

  in_game_update_bindings();
  update_user_interface();

  switch (state->stage) {
    case IN_GAME_STAGE_UNDEFINED: {

      break;
    }
    case IN_GAME_STAGE_MAP_CHOICE: {
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
      if (get_is_game_paused() || !state->has_game_started) {
        return;
      }
      update_map();
      update_game_manager();
    
      event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, (event_context){
        .data.f32[0] = _get_player_position(false).x,
        .data.f32[1] = _get_player_position(false).y,
      });
      break;
    }
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::update_scene_in_game()::Unsupported stage");
      break;
    }
  }
}

void render_scene_in_game(void) {
  STATE_ASSERT("render_scene_in_game")

  gui_draw_texture_id(TEX_ID_GAME_BG_SPACE, (Rectangle) {0, 0, GetScreenWidth(), GetScreenHeight()});

  switch (state->stage) {
    case IN_GAME_STAGE_UNDEFINED: {

      break;
    }
    case IN_GAME_STAGE_MAP_CHOICE: {
      if (IsKeyReleased(KEY_R)) {
        state->clouds_animation_playing = true;
        state->clouds_animation_timer = 0;
        event_fire(EVENT_CODE_UI_START_FADEOUT_EFFECT, (event_context){ .data.u16[0] = CLOUDS_ANIMATION_DURATION});
      }
      if (state->clouds_animation_playing && (state->clouds_animation_timer >= 0 && state->clouds_animation_timer <= CLOUDS_ANIMATION_DURATION)) {
        f32 clouds_mul = EaseQuadIn(state->clouds_animation_timer, 2, -1, CLOUDS_ANIMATION_DURATION);
        f32 worldmap_mul = EaseQuadIn(state->clouds_animation_timer, 0.75f, 0.25f, CLOUDS_ANIMATION_DURATION);
        gui_draw_texture_id_center(TEX_ID_WORLDMAP_WO_CLOUDS, 
          *get_resolution_div2(), VECTOR2(GetScreenWidth() * worldmap_mul, GetScreenHeight() * worldmap_mul), true);
        gui_draw_texture_id_center(TEX_ID_WORLDMAP_CLOUDS, 
          *get_resolution_div2(), VECTOR2(GetScreenWidth() * clouds_mul, GetScreenHeight() * clouds_mul), true);
        state->clouds_animation_timer++;
        if (state->clouds_animation_timer > CLOUDS_ANIMATION_DURATION) {
          state->clouds_animation_playing = false;
          state->clouds_animation_timer = 0;
        }
      }
      else{
        gui_draw_texture_id(TEX_ID_WORLDMAP_WO_CLOUDS, (Rectangle) {0, 0, GetScreenWidth(), GetScreenHeight()});
        for (int i=0; i<MAX_WORLDMAP_LOCATIONS; ++i) {
          if (state->worldmap_locations[i].is_active) {
            Vector2 scrloc = (Vector2) {
              state->worldmap_locations[i].screen_location.x * GetScreenWidth(), 
              state->worldmap_locations[i].screen_location.y * GetScreenHeight()
            };
            gui_draw_map_stage_pin(state->hovered_stage == i, scrloc);
          }
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
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::render_scene_in_game()::Unsupported stage");
      break;
    }
  }

}

void render_interface_in_game(void) {
  STATE_ASSERT("render_interface_in_game")
  
  switch (state->stage) {
    case IN_GAME_STAGE_UNDEFINED: {
      break;
    }
    case IN_GAME_STAGE_MAP_CHOICE: {
      for (int i=0; i<MAX_WORLDMAP_LOCATIONS; ++i) {
        if (state->hovered_stage == i) {
          panel* pnl = &state->worldmap_selection_panel;
          Rectangle scrloc = (Rectangle){
            state->worldmap_locations[i].screen_location.x * GetScreenWidth() - WORLDMAP_LOC_PIN_SIZE_DIV2, 
            state->worldmap_locations[i].screen_location.y * GetScreenHeight() - WORLDMAP_LOC_PIN_SIZE_DIV2,
            WORLDMAP_LOC_PIN_SIZE, WORLDMAP_LOC_PIN_SIZE
          };
          pnl->dest = (Rectangle) {scrloc.x + WORLDMAP_LOC_PIN_SIZE, scrloc.y + WORLDMAP_LOC_PIN_SIZE_DIV2, get_resolution_div4()->x, get_resolution_div4()->y};
          DrawCircleGradient(scrloc.x + WORLDMAP_LOC_PIN_SIZE_DIV2, scrloc.y + WORLDMAP_LOC_PIN_SIZE_DIV2, 100, (Color){236,240,241,50}, (Color){0});
          gui_panel_scissored((*pnl), false, {
            gui_label(state->worldmap_locations[i].displayname, FONT_TYPE_MOOD, 10, (Vector2) {
              pnl->dest.x + pnl->dest.width *.5f, pnl->dest.y + pnl->dest.height*.5f
            }, WHITE, true);
          });
        }
      }
      break;
    }
    case IN_GAME_STAGE_PASSIVE_CHOICE: {
      Rectangle dest = (Rectangle) {
        get_resolution_div4()->x, get_resolution_div2()->y, 
        get_resolution_div4()->x, get_resolution_3div2()->y 
      };
      f32 dest_x_buffer = dest.x;
      for (int i=0; i<MAX_UPDATE_PASSIVE_PANEL_COUNT; ++i) {
        panel* pnl = &state->passive_selection_panels[i];
        if(pnl->buffer[0].data.u16[0] <= 0 || pnl->buffer[0].data.u16[0] >= CHARACTER_STATS_MAX) {
          pnl->buffer[0].data.u16[0] = get_random(1,CHARACTER_STATS_MAX-1);
        }
        dest.x = dest_x_buffer + ((dest.width + get_screen_offset().x) * i);
        character_stat* stat = get_player_stat(pnl->buffer[0].data.u16[0]);
        if(gui_panel_active(pnl, dest, true)) {
          set_is_game_paused(false);
          for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
            state->passive_selection_panels[i] = (panel){0};
          }
        }
        draw_passive_selection_panel(stat, dest);
      }
      break;
    }
    case IN_GAME_STAGE_PLAY: {  
      DrawFPS(get_screen_offset().x, get_resolution_div2()->y);

      if (!state->has_game_started) {
        gui_label("Press Space to Start!", FONT_TYPE_MOOD_OUTLINE, 10, (Vector2) {get_resolution_div2()->x, get_resolution_3div2()->y}, WHITE, true);
        return;
      }
      else if (get_b_player_have_upgrade_points()) {
        set_is_game_paused(true);
        Rectangle dest = (Rectangle) {
          get_resolution_div4()->x, get_resolution_div2()->y, 
          get_resolution_div4()->x, get_resolution_3div2()->y 
        };
        f32 dest_x_buffer = dest.x;
        for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
          panel* pnl = &state->ability_upg_panels[i];
          if(pnl->buffer[0].data.u16[0] <= 0 || pnl->buffer[0].data.u16[0] >= ABILITY_TYPE_MAX) {
            pnl->buffer[0].data.u16[0] = get_random(1,ABILITY_TYPE_MAX-1);
          }
          dest.x = dest_x_buffer + ((dest.width + get_screen_offset().x) * i);
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
            // TODO: Upgrade ability
          }
          draw_upgrade_panel(abl, new, dest);
        }
      }
      else {
        gui_progress_bar(PRG_BAR_ID_PLAYER_EXPERIANCE, (Vector2){.x = get_resolution_div2()->x, .y = get_screen_offset().x}, true);
        gui_progress_bar(PRG_BAR_ID_PLAYER_HEALTH, get_screen_offset(), false);
        gui_label_format(FONT_TYPE_MOOD, 10, get_screen_offset().x, get_resolution_div4()->y, WHITE, false, "Remaining: %d", get_remaining_enemies());
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
    case IN_GAME_STAGE_UNDEFINED: {
      break;
    }
    case IN_GAME_STAGE_MAP_CHOICE: {
      state->hovered_stage = U16_MAX;
      for (int i=0; i<MAX_WORLDMAP_LOCATIONS; ++i) {
        Rectangle scrloc = (Rectangle){
          state->worldmap_locations[i].screen_location.x * GetScreenWidth() - WORLDMAP_LOC_PIN_SIZE_DIV2, 
          state->worldmap_locations[i].screen_location.y * GetScreenHeight() - WORLDMAP_LOC_PIN_SIZE_DIV2,
          WORLDMAP_LOC_PIN_SIZE, WORLDMAP_LOC_PIN_SIZE
        };
        if (CheckCollisionPointRec(GetMousePosition(), scrloc)) {
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
    default: {
      TraceLog(LOG_WARNING, "scene_in_game::update_scene_in_game()::Unsupported stage");
      break;
    }
  }
}

void in_game_update_keyboard_bindings(void) {

  switch (state->stage) {
    case IN_GAME_STAGE_UNDEFINED: {
      break;
    }
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
        start_game();
        state->has_game_started = true;
        set_is_game_paused(false);
      }
      if (IsKeyReleased(KEY_ESCAPE)) {
        if(!get_b_player_have_upgrade_points()) toggle_is_game_paused();
        event_fire(EVENT_CODE_UI_SHOW_PAUSE_MENU, (event_context){0});
      }
      break;
    }
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

void start_game(void) {}

void draw_upgrade_panel(ability* abl, ability upg, Rectangle panel_dest) {
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

  gui_draw_texture_id_pro(TEX_ID_ABILITY_ICON_ATLAS, upg.icon_src, icon_rect);
  gui_label(upg.display_name, FONT_TYPE_MOOD, title_font_size, ability_name_pos, WHITE, true);

  if (upg.level == 1) {
    gui_label("NEW!", FONT_TYPE_MOOD_OUTLINE, level_ind_font_size, ability_level_ind, WHITE, true);
  } else if(upg.level>1 && upg.level <= MAX_ABILITY_LEVEL) {
    gui_label_format_v(FONT_TYPE_MOOD_OUTLINE, level_ind_font_size, ability_level_ind, WHITE, true, "%d -> %d", abl->level, upg.level);
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
  
  gui_draw_texture_id_pro(TEX_ID_ICON_ATLAS, stat->passive_icon_src, icon_rect);
  gui_label(stat->passive_display_name, FONT_TYPE_MOOD, title_font_size, passive_name_pos, WHITE, true);
  
  const u16 desc_font_size = 6;
  const f32 desc_box_height = panel_dest.y + (panel_dest.height * .5f) - passive_name_pos.y - elm_space_gap;
  const Vector2 padding = (Vector2){ get_screen_offset().x, elm_space_gap };
  const Rectangle desc_box = (Rectangle) {
    .x      = panel_dest.x - (panel_dest.width * .5f) + padding.x,
    .y      = passive_name_pos.y + padding.y,
    .width  = panel_dest.width - padding.x,
    .height = desc_box_height - padding.y,
  };
  gui_label_wrap(stat->passive_desc, FONT_TYPE_MINI_MOOD, desc_font_size, desc_box, WHITE, false);

}

#undef STATE_ASSERT
#undef ABILITY_UPG_PANEL_ICON_SIZE
#undef CLOUDS_ANIMATION_DURATION
#undef FADE_ANIMATION_DURATION
#undef DRAW_ABL_UPG_STAT_PNL
