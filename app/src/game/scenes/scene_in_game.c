#include "scene_in_game.h"
#include <reasings.h>
#include <settings.h>

#include <core/ftime.h>
#include <core/event.h>
#include <core/fmath.h>
#include <core/fmemory.h>

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
  panel skill_up_panels[MAX_UPDATE_ABILITY_PANEL_COUNT];
  player_state* player;
  in_game_stages stage;
  worldmap_stage worldmap_locations[MAX_WORLDMAP_LOCATIONS];
  panel worldmap_selection_panel;

  u16 clouds_animation_timer;

  bool has_game_started;
  bool clouds_animation_playing;
} scene_in_game_state;

static scene_in_game_state *restrict state;

#define STATE_ASSERT(FUNCTION) if (!state) {                                              \
  TraceLog(LOG_ERROR, "scene_in_game::" FUNCTION "::In game state was not initialized");  \
  event_fire(EVENT_CODE_SCENE_MAIN_MENU, (event_context){0});                             \
  return;                                                                                 \
}
#define SKILL_UP_PANEL_ICON_SIZE get_resolution_div4()->x*.5f
#define CLOUDS_ANIMATION_DURATION TARGET_FPS * 1.5f // second

void in_game_update_bindings(void);
void in_game_update_mouse_bindings(void);
void in_game_update_keyboard_bindings(void);
void initialize_worldmap_locations(void);
void start_game(void);
void draw_upgrade_panel(ability* abl, ability upg, Rectangle panel_dest);

bool initialize_scene_in_game(camera_metrics* _camera_metrics) {

  state = (scene_in_game_state *)allocate_memory_linear(sizeof(scene_in_game_state), true);

  // Game
  if (!game_manager_initialize(_camera_metrics)) { // Inits player & spawns
    TraceLog(LOG_ERROR, "game_manager_initialize() failed");
    return false;
  }

  state->player = get_player_state_if_available();

  _set_player_position(*get_resolution_div2());

  user_interface_system_initialize();

  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, (event_context){
    .data.f32[0] = PRG_BAR_ID_PLAYER_EXPERIANCE,
    .data.f32[1] = get_player_state_if_available()->exp_perc,
  });
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, (event_context){
    .data.f32[0] = PRG_BAR_ID_PLAYER_HEALTH,
    .data.f32[1] = get_player_state_if_available()->health_perc,
  });  
  event_fire(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, (event_context){
    .data.f32[0] = get_player_state_if_available()->position.x,
    .data.f32[1] = get_player_state_if_available()->position.y,
  });
  
  panel default_panel = (panel) {
    .signal_state  = BTN_STATE_RELEASED,
    .bg_tex_id     = TEX_ID_CRIMSON_FANTASY_PANEL_BG,
    .frame_tex_id  = TEX_ID_CRIMSON_FANTASY_PANEL,
    .bg_tint       = (Color) { 30, 39, 46, 245},
    .bg_hover_tint = (Color) { 52, 64, 76, 245},
    .offsets = (Vector4) {6, 6, 6, 6},
  };
  for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
    state->skill_up_panels[i] = default_panel;
  }
  initialize_worldmap_locations();
  state->worldmap_selection_panel = default_panel;

  set_is_game_paused(true);
  state->stage = IN_GAME_STAGE_MAP_CHOICE;
  state->clouds_animation_playing = true;
  event_fire(EVENT_CODE_UI_START_FADE_EFFECT, (event_context){ .data.u16[0] = CLOUDS_ANIMATION_DURATION });
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
      

      break;
    }
    case IN_GAME_STAGE_PASSIVE_CHOICE: {
      break;
    }
    case IN_GAME_STAGE_PLAY: {
      if (get_is_game_paused() || !state->has_game_started) {
        return;
      }
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
        event_fire(EVENT_CODE_UI_START_FADE_EFFECT, (event_context){ .data.u16[0] = CLOUDS_ANIMATION_DURATION});
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
        gui_draw_texture_id(TEX_ID_WORLDMAP_W_CLOUDS, (Rectangle) {0, 0, GetScreenWidth(), GetScreenHeight()});
        for (int i=0; i<MAX_WORLDMAP_LOCATIONS; ++i) {
          DrawRectangle(state->worldmap_locations[i].screen_location.x, state->worldmap_locations[i].screen_location.y, 
            state->worldmap_locations[i].screen_location.width, state->worldmap_locations[i].screen_location.height, (Color) {255, 0, 0, 100});
        }
      }
      break;
    }
    case IN_GAME_STAGE_PASSIVE_CHOICE: {
      break;
    }
    case IN_GAME_STAGE_PLAY: {
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
        if (CheckCollisionPointRec(GetMousePosition(), state->worldmap_locations[i].screen_location)) {
          state->worldmap_selection_panel.dest = (Rectangle) {
            state->worldmap_locations[i].screen_location.x, state->worldmap_locations[i].screen_location.y,
            get_resolution_div4()->x, get_resolution_div4()->y
          };
          gui_panel_scissored(state->worldmap_selection_panel, false, {
            
          });
        }
      }

      break;
    }
    case IN_GAME_STAGE_PASSIVE_CHOICE: {
      break;
    }
    case IN_GAME_STAGE_PLAY: {  
      DrawFPS(get_screen_offset().x, get_resolution_div2()->y);

      if (!state->has_game_started) {
        gui_label("Press Space to Start!", FONT_TYPE_MOOD_OUTLINE, 10, (Vector2) {get_resolution_div2()->x, get_resolution_3div2()->y}, WHITE, true);
        return;
      }
      else if (state->player->is_player_have_skill_points) {
        set_is_game_paused(true);
        Rectangle dest = (Rectangle) {
          get_resolution_div4()->x, get_resolution_div2()->y, 
          get_resolution_div4()->x, get_resolution_3div2()->y 
        };
        f32 dest_x_buffer = dest.x;
        for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
          panel* pnl = &state->skill_up_panels[i];
          if(pnl->buffer[0].data.u16[0] <= 0 || pnl->buffer[0].data.u16[0] >= ABILITY_TYPE_MAX) {
            pnl->buffer[0].data.u16[0] = get_random(1,ABILITY_TYPE_MAX-1);
          }
          dest.x = dest_x_buffer + ((dest.width + get_screen_offset().x) * i);
          ability* abl = &state->player->ability_system.abilities[pnl->buffer[0].data.u16[0]];
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
            state->player->is_player_have_skill_points = false;
            for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
              state->skill_up_panels[i].buffer[0].data.u16[0] = 0;
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
      break;
    }
    case IN_GAME_STAGE_PLAY: {  
      if (!state->has_game_started && IsKeyPressed(KEY_SPACE)) {
        start_game();
        state->has_game_started = true;
        set_is_game_paused(false);
      }
      if (IsKeyReleased(KEY_ESCAPE)) {
        if(!state->player->is_player_have_skill_points) toggle_is_game_paused();
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

void initialize_worldmap_locations(void) {
  state->worldmap_locations[0] = (worldmap_stage) {
    .name = "Stage 1",
    .screen_location = SCREEN_RECT(16, 105, 2, 3),
  };
  state->worldmap_locations[1] = (worldmap_stage) {
    .name = "Stage 2",
    .screen_location = SCREEN_RECT(31, 96, 2, 2),
  };
  state->worldmap_locations[2] = (worldmap_stage) {
    .name = "Stage 3",
    .screen_location = SCREEN_RECT(45, 96, 2, 3),
  };
  state->worldmap_locations[3] = (worldmap_stage) {
    .name = "Stage 4",
    .screen_location = SCREEN_RECT(52, 95, 1, 2),
  };
  state->worldmap_locations[4] = (worldmap_stage) {
    .name = "Stage 5",
    .screen_location = SCREEN_RECT(67, 89, 2, 2),
  };
  state->worldmap_locations[5] = (worldmap_stage) {
    .name = "Stage 6",
    .screen_location = SCREEN_RECT(69, 68, 2, 2),
  };
  state->worldmap_locations[6] = (worldmap_stage) {
    .name = "Stage 7",
    .screen_location = SCREEN_RECT(56, 68, 2, 2),
  };
  state->worldmap_locations[7] = (worldmap_stage) {
    .name = "Stage 8",
    .screen_location = SCREEN_RECT(82, 66, 2, 2),
  };
  state->worldmap_locations[8] = (worldmap_stage) {
    .name = "Stage 9",
    .screen_location = SCREEN_RECT(93, 83, 2, 2),
  };
  state->worldmap_locations[9] = (worldmap_stage) {
    .name = "Stage 10",
    .screen_location = SCREEN_RECT(86, 109, 3, 3),
  };
  state->worldmap_locations[10] = (worldmap_stage) {
    .name = "Stage 11",
    .screen_location = SCREEN_RECT(99, 78, 3, 3),
  };
  state->worldmap_locations[11] = (worldmap_stage) {
    .name = "Stage 12",
    .screen_location = SCREEN_RECT(107, 98, 1, 2),
  };
  state->worldmap_locations[12] = (worldmap_stage) {
    .name = "Stage 13",
    .screen_location = SCREEN_RECT(95, 63, 2, 3),
  };
  state->worldmap_locations[13] = (worldmap_stage) {
    .name = "Stage 14",
    .screen_location = SCREEN_RECT(110, 70, 3, 3),
  };
  state->worldmap_locations[14] = (worldmap_stage) {
    .name = "Stage 15",
    .screen_location = SCREEN_RECT(107, 42, 2, 2),
  };
  state->worldmap_locations[15] = (worldmap_stage) {
    .name = "Stage 16",
    .screen_location = SCREEN_RECT(82, 51, 2, 2),
  };
  state->worldmap_locations[16] = (worldmap_stage) {
    .name = "Stage 17",
    .screen_location = SCREEN_RECT(110, 53, 2, 2),
  };
  state->worldmap_locations[17] = (worldmap_stage) {
    .name = "Stage 18",
    .screen_location = SCREEN_RECT(74, 32, 2, 2),
  };
  state->worldmap_locations[18] = (worldmap_stage) {
    .name = "Stage 19",
    .screen_location = SCREEN_RECT(102, 19, 2, 3),
  };
  state->worldmap_locations[19] = (worldmap_stage) {
    .name = "Stage 20",
    .screen_location = SCREEN_RECT(107, 12, 1, 2),
  };
  state->worldmap_locations[20] = (worldmap_stage) {
    .name = "Stage 21",
    .screen_location = SCREEN_RECT(61, 21, 1, 2),
  };
  state->worldmap_locations[21] = (worldmap_stage) {
    .name = "Stage 22",
    .screen_location = SCREEN_RECT(51, 21, 2, 3),
  };
}

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
    panel_dest.x - SKILL_UP_PANEL_ICON_SIZE/2.f,
    start_panel_height - SKILL_UP_PANEL_ICON_SIZE*.5f,
    SKILL_UP_PANEL_ICON_SIZE,SKILL_UP_PANEL_ICON_SIZE
  };
  const Vector2 ability_name_pos  = (Vector2) {panel_dest.x, start_panel_height + SKILL_UP_PANEL_ICON_SIZE*.5f + elm_space_gap};
  const Vector2 ability_level_ind = (Vector2) {panel_dest.x, ability_name_pos.y + btw_space_gap};

  const u16 title_font_size = 10; 
  const u16 level_ind_font_size = 9; 
  const u16 upgr_font_size = 6; 

  gui_draw_texture_id_pro(TEX_ID_SKILL_ICON_ATLAS, upg.icon_src, icon_rect);
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
      case ABILITY_UPG_DAMAGE: {
        if (upg.level == 1) {
          gui_label_format(FONT_TYPE_MINI_MOOD, upgr_font_size, start_upgradables_x_exis, upgradables_height_buffer, WHITE, false, "Damage:%d", upg.base_damage);
          upgradables_height_buffer += btw_space_gap;
        } else if(upg.level>1 && upg.level <= MAX_ABILITY_LEVEL) {
          gui_label_format(FONT_TYPE_MINI_MOOD, upgr_font_size, start_upgradables_x_exis, upgradables_height_buffer, WHITE, false, "Damage:%d -> %d", abl->base_damage, upg.base_damage);
          upgradables_height_buffer += btw_space_gap;
        }
        break;
      }
      case ABILITY_UPG_AMOUNT: {
        if (upg.level == 1) {
          gui_label_format(FONT_TYPE_MINI_MOOD, upgr_font_size, start_upgradables_x_exis, upgradables_height_buffer, WHITE, false, "Amouth:%d", upg.proj_count);
          upgradables_height_buffer += btw_space_gap;
        } else if(upg.level>1 && upg.level <= MAX_ABILITY_LEVEL) {
          gui_label_format(FONT_TYPE_MINI_MOOD, upgr_font_size, start_upgradables_x_exis, upgradables_height_buffer, WHITE, false, "Amouth:%d -> %d", abl->proj_count, upg.proj_count);
          upgradables_height_buffer += btw_space_gap;
        }  
        break;
      }
      case ABILITY_UPG_HITBOX: {
        if (upg.level == 1) {
          gui_label_format(FONT_TYPE_MINI_MOOD, upgr_font_size, start_upgradables_x_exis, upgradables_height_buffer, WHITE, false, "Hitbox:%.0f", upg.proj_dim.x);
          upgradables_height_buffer += btw_space_gap;
        } else if(upg.level>1 && upg.level <= MAX_ABILITY_LEVEL) {
          gui_label_format(FONT_TYPE_MINI_MOOD, upgr_font_size, start_upgradables_x_exis, upgradables_height_buffer, WHITE, false, "Hitbox:%.0f - > %.0f", abl->proj_dim.x, upg.proj_dim.x);
          upgradables_height_buffer += btw_space_gap;
        } 
        break;
      }
      case ABILITY_UPG_SPEED: {
        if (upg.level == 1) {
          gui_label_format(FONT_TYPE_MINI_MOOD, upgr_font_size, start_upgradables_x_exis, upgradables_height_buffer, WHITE, false, "Speed:%d", upg.proj_speed);
          upgradables_height_buffer += btw_space_gap;
        } else if(upg.level>1 && upg.level <= MAX_ABILITY_LEVEL) {
          gui_label_format(FONT_TYPE_MINI_MOOD, upgr_font_size, start_upgradables_x_exis, upgradables_height_buffer, WHITE, false, "Speed:%d - > %d", upg.proj_speed, abl->proj_speed);
          upgradables_height_buffer += btw_space_gap;
        }
        break;
      }
      default: {
        TraceLog(LOG_WARNING, "scene_in_game::draw_upgrade_panel()::Unsupported ability upgrade type");
        break;
      }
    }
  }
}

#undef STATE_ASSERT
#undef SKILL_UP_PANEL_ICON_SIZE
#undef CLOUDS_ANIMATION_DURATION
#undef FADE_ANIMATION_DURATION
