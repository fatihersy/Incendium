#include "scene_in_game.h"
#include <settings.h>

#include <core/ftime.h>
#include <core/event.h>
#include <core/fmath.h>
#include <core/fmemory.h>

#include "game/resource.h"
#include "game/game_manager.h"
#include "game/user_interface.h"

typedef struct scene_in_game_state {
  player_state* player;
  panel skill_up_panels[MAX_UPDATE_ABILITY_PANEL_COUNT];
  
  bool has_game_started;
} scene_in_game_state;

static scene_in_game_state *state;


#define STATE_ASSERT(FUNCTION) if (!state) {                                                          \
    TraceLog(LOG_ERROR, "scene_in_game::" FUNCTION "::In game state was not initialized");            \
    event_fire(EVENT_CODE_SCENE_MAIN_MENU, (event_context){0});                                        \
    return;                                                                                           \
}
#define SKILL_UP_PANEL_ICON_SIZE get_resolution_div4()->x*.5f

void in_game_update_bindings(void);
void in_game_update_mouse_bindings(void);
void in_game_update_keyboard_bindings(void);
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

  for (u32 i = 0; i < 360; i += 20) {
    Vector2 position = get_a_point_of_a_circle(_get_player_position(false), 500, i);
    Texture2D *tex = get_texture_by_enum(TEX_ID_ENEMY_TEXTURE);
    rectangle_collision rect_col = (rectangle_collision) {
      .rect = (Rectangle) {
        .x = position.x,
        .y = position.y,
        .width = tex->width,
        .height = tex->height
      },
      .owner_type = ENEMY
    };
    rect_col.owner_id = _spawn_character((Character2D){
        .character_id = 0,
        .tex = tex,
        .initialized = false,
        .collision = rect_col.rect,
        .position = position,
        .w_direction = WORLD_DIRECTION_LEFT,
        .type = ENEMY,
        .rotation = 0,
        .health = 100,
        .damage = 10,
        .speed = 1,
    });
    add_collision(rect_col);
  }

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

  set_is_game_paused(true);

  return true;
}

void update_scene_in_game(void) {
  STATE_ASSERT("update_scene_in_game")

  in_game_update_bindings();
  update_user_interface();

  if (get_is_game_paused() || !state->has_game_started) {
    return;
  }
  update_game_manager();
  update_time();

  event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, (event_context){
    .data.f32[0] = _get_player_position(false).x,
    .data.f32[1] = _get_player_position(false).y,
  });
}

void render_scene_in_game(void) {
  STATE_ASSERT("render_scene_in_game")

  render_game();
}

void render_interface_in_game(void) {
  STATE_ASSERT("render_interface_in_game")
  DrawFPS(get_screen_offset().x, get_resolution_div2()->y);

  if (!state->has_game_started) {
    gui_label("Press Space to Start!", FONT_TYPE_MOOD_OUTLINE, 10, (Vector2) {get_resolution_div2()->x, get_resolution_3div2()->y}, WHITE, true);
    return;
  }

  if (state->player->is_player_have_skill_points || true) {
    set_is_game_paused(true);
    Rectangle dest = (Rectangle) { // TODO: Make it responsive
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
      }
      draw_upgrade_panel(abl, new, dest);
    }
  }
  else {
    gui_progress_bar(PRG_BAR_ID_PLAYER_EXPERIANCE, (Vector2){.x = get_resolution_div2()->x, .y = get_screen_offset().x}, true);
    gui_progress_bar(PRG_BAR_ID_PLAYER_HEALTH, get_screen_offset(), false);
  }

  render_user_interface();
}

void start_game(void) {

}

inline void draw_upgrade_panel(ability* abl, ability upg, Rectangle panel_dest) {
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


void in_game_update_bindings(void) {
  in_game_update_mouse_bindings();
  in_game_update_keyboard_bindings();
}

void in_game_update_mouse_bindings(void) { 

}
void in_game_update_keyboard_bindings(void) {

  if (!state->has_game_started && IsKeyPressed(KEY_SPACE)) {
    start_game();
    state->has_game_started = true;
    set_is_game_paused(false);
    return;
  }

  if (IsKeyReleased(KEY_ESCAPE)) {
    if(!state->player->is_player_have_skill_points) toggle_is_game_paused();
    event_fire(EVENT_CODE_UI_SHOW_PAUSE_MENU, (event_context){0});
  }
}





