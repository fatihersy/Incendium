#include "player.h"

#include "core/event.h"
#include "core/fmemory.h"
#include "core/logger.h"

#include "loc_types.h"

#include "game/spritesheet.h"

// To avoid dublicate symbol errors. Implementation in defines.h
extern const i32 level_curve[MAX_PLAYER_LEVEL+1];
#define PLAYER_SCALE 3
#define PLAYER_DAMAGE_BREAK_TIME .5f

typedef struct player_system_state {
  const camera_metrics * in_camera_metrics;
  const app_settings * in_app_settings;
  const ingame_info * in_ingame_info;

  player_state dynamic_player;
  player_state defualt_player;

  player_system_state(void) {
  this->in_camera_metrics = nullptr;
  this->in_app_settings = nullptr;
  this->in_ingame_info = nullptr;

  this->dynamic_player = player_state();
  this->defualt_player = player_state();
  }
} player_system_state;

static player_system_state* state = nullptr;

bool player_system_on_event(i32 code, event_context context);
void player_system_reinit(void);
void play_anim(spritesheet_id player_anim_sheet);
void player_update_sprite(void);

bool player_system_initialize(const camera_metrics* in_camera_metrics,const app_settings* in_app_settings,const ingame_info* in_ingame_info) {
  if (state and state != nullptr){ 
    player_system_reinit();
    return true;
  }
  state = (player_system_state*)allocate_memory_linear(sizeof(player_system_state), true);
  if (not state or state == nullptr) {
    IERROR("player::player_system_initialize()::Failed to allocate player system");
    return false;
  }
  *state = player_system_state();

  if (not in_camera_metrics or in_camera_metrics == nullptr) {
    IERROR("player::player_system_initialize()::Camera metric pointer is invalid");
    return false;
  }
  if (not in_app_settings or in_app_settings == nullptr) {
    IERROR("player::player_system_initialize()::Settings pointer is invalid");
    return false;
  }
  if (not in_ingame_info or in_ingame_info == nullptr) {
    IERROR("player::player_system_initialize()::Game info pointer is invalid");
    return false;
  }

  state->in_camera_metrics = in_camera_metrics;
  state->in_app_settings = in_app_settings;
  state->in_ingame_info = in_ingame_info;

  state->defualt_player.move_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_MOVE_RIGHT;
  state->defualt_player.idle_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_IDLE_RIGHT;
  state->defualt_player.take_damage_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT;
  state->defualt_player.wreck_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_WRECK_RIGHT;
  state->defualt_player.attack_1_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ATTACK_1;
  state->defualt_player.attack_2_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ATTACK_2;
  state->defualt_player.attack_3_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ATTACK_3;
  state->defualt_player.attack_down_sprite.sheet_id =  SHEET_ID_PLAYER_ANIMATION_ATTACK_DOWN;
  state->defualt_player.attack_up_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ATTACK_UP;
  state->defualt_player.roll_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ROLL;
  state->defualt_player.dash_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_DASH;
  set_sprite(&state->defualt_player.move_right_sprite,        true, false);
  set_sprite(&state->defualt_player.idle_right_sprite,        true, false);
  set_sprite(&state->defualt_player.take_damage_right_sprite, true, false);
  set_sprite(&state->defualt_player.wreck_right_sprite,       true, false);
  set_sprite(&state->defualt_player.attack_1_sprite,          true, false);
  set_sprite(&state->defualt_player.attack_2_sprite,          true, false);
  set_sprite(&state->defualt_player.attack_3_sprite,          true, false);
  set_sprite(&state->defualt_player.attack_down_sprite,       true, false);
  set_sprite(&state->defualt_player.attack_up_sprite,         true, false);
  set_sprite(&state->defualt_player.roll_sprite,              true, false);
  set_sprite(&state->defualt_player.dash_sprite,              true, false);

  {
    state->defualt_player.stats.at(CHARACTER_STATS_HEALTH) = character_stat(CHARACTER_STATS_HEALTH, LOC_TEXT_PLAYER_STAT_LIFE_ESSENCE, LOC_TEXT_PLAYER_STAT_DESC_LIFE_ESSENCE, 
      Rectangle{2016, 640, 32, 32}, (i32) 1, level_curve[1], data128(static_cast<i32>(0))
    );
    state->defualt_player.stats.at(CHARACTER_STATS_HP_REGEN) = character_stat(CHARACTER_STATS_HP_REGEN, LOC_TEXT_PLAYER_STAT_BREAD, LOC_TEXT_PLAYER_STAT_DESC_BREAD, 
      Rectangle{1856, 896, 32, 32}, (i32) 0, level_curve[1], data128(static_cast<i32>(0))
    );
    state->defualt_player.stats.at(CHARACTER_STATS_MOVE_SPEED) = character_stat(CHARACTER_STATS_MOVE_SPEED, LOC_TEXT_PLAYER_STAT_CARDINAL_BOOTS, LOC_TEXT_PLAYER_STAT_DESC_CARDINAL_BOOTS, 
      Rectangle{1632, 960, 32, 32}, 1, (i32) level_curve[1], data128(static_cast<f32>(64.f))
    );
    state->defualt_player.stats.at(CHARACTER_STATS_AOE) =  character_stat(CHARACTER_STATS_AOE, LOC_TEXT_PLAYER_STAT_BLAST_SCROLL, LOC_TEXT_PLAYER_STAT_DESC_BLAST_SCROLL, 
      Rectangle{1888, 640, 32, 32}, 0, (i32)  level_curve[1], data128(static_cast<f32>(0.f))
    );
    state->defualt_player.stats.at(CHARACTER_STATS_DAMAGE) =  character_stat(CHARACTER_STATS_DAMAGE, LOC_TEXT_PLAYER_STAT_HEAVY_CROSS, LOC_TEXT_PLAYER_STAT_DESC_HEAVY_CROSS, 
      Rectangle{1856, 640, 32, 32}, 1, (i32) level_curve[1], data128(static_cast<i32>(0))
    );
    state->defualt_player.stats.at(CHARACTER_STATS_ABILITY_CD) = character_stat(CHARACTER_STATS_ABILITY_CD, LOC_TEXT_PLAYER_STAT_HOURGLASS, LOC_TEXT_PLAYER_STAT_DESC_HOURGLASS, 
      Rectangle{1696, 672, 32, 32}, 0, (i32)level_curve[1], data128(static_cast<f32>(0.f))
    );
    state->defualt_player.stats.at(CHARACTER_STATS_PROJECTILE_AMOUTH) = character_stat(CHARACTER_STATS_PROJECTILE_AMOUTH, LOC_TEXT_PLAYER_STAT_SECOND_HAND, LOC_TEXT_PLAYER_STAT_DESC_SECOND_HAND, 
      Rectangle{1632, 1056, 32, 32}, 1, (i32) level_curve[1], data128(static_cast<i32>(0))
    );
    state->defualt_player.stats.at(CHARACTER_STATS_EXP_GAIN) =  character_stat(CHARACTER_STATS_EXP_GAIN, LOC_TEXT_PLAYER_STAT_SEEING_EYES, LOC_TEXT_PLAYER_STAT_DESC_SEEING_EYES, 
      Rectangle{1760, 640, 32, 32}, 0, (i32) level_curve[1], data128(static_cast<f32>(0.f))
    );
    state->defualt_player.stats.at(CHARACTER_STATS_TOTAL_TRAIT_POINTS) =  character_stat(CHARACTER_STATS_TOTAL_TRAIT_POINTS, LOC_TEXT_PLAYER_STAT_TOTAL_TRAIT_POINTS, LOC_TEXT_PLAYER_STAT_DESC_TOTAL_TRAIT_POINTS, 
      Rectangle{2272, 640, 32, 32}, 1, (i32) level_curve[1], data128(static_cast<i32>(7))
    );
  }

  state->defualt_player.position = ZEROVEC2;
  state->defualt_player.collision.width = (state->defualt_player.idle_right_sprite.coord.width * .9f) * PLAYER_SCALE; // INFO: player collision scales with idle spritesheet
  state->defualt_player.collision.height = (state->defualt_player.idle_right_sprite.coord.height * .9f) * PLAYER_SCALE;
  state->defualt_player.is_dead = false;
  state->defualt_player.is_moving = false;
  state->defualt_player.w_direction = WORLD_DIRECTION_LEFT;
  state->defualt_player.is_damagable = true;
  state->defualt_player.damage_break_time = PLAYER_DAMAGE_BREAK_TIME;
  state->defualt_player.damage_break_current = state->defualt_player.damage_break_time;
  state->defualt_player.level = 1;
  state->defualt_player.exp_to_next_level = level_curve[state->defualt_player.level];
  state->defualt_player.exp_current = 0;
  state->defualt_player.stats.at(CHARACTER_STATS_HEALTH).buffer.i32[0] = 0; // INFO: Will be Modified by player stats
  state->defualt_player.health_current = 0;
  state->defualt_player.health_perc = 0.f;
  state->defualt_player.is_initialized = true;

  event_register(EVENT_CODE_PLAYER_ADD_EXP, player_system_on_event);
  event_register(EVENT_CODE_PLAYER_SET_POSITION, player_system_on_event);
  event_register(EVENT_CODE_PLAYER_TAKE_DAMAGE, player_system_on_event);

  player_system_reinit();

  return true;
}
void player_system_reinit(void) {
  if (not state or state == nullptr) {
    IERROR("player::player_system_reinit()::State is invalid");
    return;
  }
  state->dynamic_player = state->defualt_player;
}

player_update_results update_player(void) {
  if (not state or state == nullptr) return player_update_results();
  if (state->dynamic_player.is_dead) { 
    return player_update_results(); 
  }

  if(!state->dynamic_player.is_damagable) {
    if(state->dynamic_player.damage_break_current <= 0) { state->dynamic_player.is_damagable = true; }
    else { state->dynamic_player.damage_break_current -= GetFrameTime(); }
  }
  Vector2 new_position = ZEROVEC2;
  if (IsKeyDown(KEY_W)) {
    new_position.y -= state->dynamic_player.stats.at(CHARACTER_STATS_MOVE_SPEED).buffer.f32[3] * GetFrameTime();
  }
  if (IsKeyDown(KEY_A)) {
    new_position.x -= state->dynamic_player.stats.at(CHARACTER_STATS_MOVE_SPEED).buffer.f32[3] * GetFrameTime();
  }
  if (IsKeyDown(KEY_S)) {
    new_position.y += state->dynamic_player.stats.at(CHARACTER_STATS_MOVE_SPEED).buffer.f32[3] * GetFrameTime();
  }
  if (IsKeyDown(KEY_D)) {
    new_position.x += state->dynamic_player.stats.at(CHARACTER_STATS_MOVE_SPEED).buffer.f32[3] * GetFrameTime();
  }
  player_update_sprite();
  
  const i32 elapsed_time = static_cast<i32>(GetTime());
  i32& last_time_hp_regen_fired = state->in_ingame_info->player_state_dynamic->stats.at(CHARACTER_STATS_HP_REGEN).mm_ex.i32[0];
  const i32& hp_max = state->in_ingame_info->player_state_dynamic->stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3];
  i32& hp_current = state->in_ingame_info->player_state_dynamic->health_current;

  if (last_time_hp_regen_fired < elapsed_time and hp_current < hp_max) {
    last_time_hp_regen_fired = elapsed_time;
    const i32& hp_regen_amouth = state->in_ingame_info->player_state_dynamic->stats.at(CHARACTER_STATS_HP_REGEN).buffer.i32[3];
    hp_current += hp_regen_amouth;
    hp_current = FCLAMP(hp_current, 0, hp_max);
  }

  state->dynamic_player.health_perc = static_cast<f32>(state->dynamic_player.health_current) / static_cast<f32>(state->dynamic_player.stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3]);
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_HEALTH, (f32)state->dynamic_player.health_perc));

  return player_update_results(new_position, true);
}
bool render_player(void) {
  if (not state or state == nullptr) { return false; }
  
  spritesheet& _sheet = state->dynamic_player.current_anim_to_play;
  const Rectangle& frame_rect = state->dynamic_player.current_anim_to_play.current_frame_rect;
  
  play_sprite_on_site_ex(
    __builtin_addressof(_sheet), 
    Rectangle {frame_rect.x + _sheet.offset.x, frame_rect.y + _sheet.offset.y, frame_rect.width, frame_rect.height}, 
    state->dynamic_player.current_anim_to_play.coord, 
    state->dynamic_player.current_anim_to_play.origin, 
    state->dynamic_player.current_anim_to_play.rotation, 
    WHITE
  );

  // TODO: Remove Later
  //DrawRectangleLines(
  //  static_cast<i32>(state->dynamic_player.collision.x), static_cast<i32>(state->dynamic_player.collision.y),
  //  static_cast<i32>(state->dynamic_player.collision.width), static_cast<i32>(state->dynamic_player.collision.height), 
  //  WHITE
  //);

  #if DEBUG_COLLISIONS
    DrawRectangleLines(
      static_cast<i32>(state->dynamic_player.collision.x), static_cast<i32>(state->dynamic_player.collision.y),
      static_cast<i32>(state->dynamic_player.collision.width), static_cast<i32>(state->dynamic_player.collision.height), 
      WHITE
    );
  #endif
  return true;
}

void player_move_player(Vector2 new_pos) {
  if (not state or state == nullptr) {
    IERROR("player::player_move_player()::State is invalid");
    return;
  }
  if (new_pos.x < 0.f) {
    state->dynamic_player.w_direction = WORLD_DIRECTION_LEFT;
  }
  else if (new_pos.x > 0.f) {
    state->dynamic_player.w_direction = WORLD_DIRECTION_RIGHT;
  }
  else if(new_pos.x == 0.f && new_pos.y == 0.f) {
    state->dynamic_player.is_moving = false;
    return;
  }
  
  state->dynamic_player.is_moving = true;
  state->dynamic_player.position.x += new_pos.x;
  state->dynamic_player.position.y += new_pos.y;

  state->dynamic_player.collision = Rectangle {
    state->dynamic_player.position.x - (state->dynamic_player.collision.width * .5f),
    state->dynamic_player.position.y - (state->dynamic_player.collision.height * .5f),
    state->dynamic_player.collision.width,
    state->dynamic_player.collision.height
  };

  state->dynamic_player.map_level_collision = Rectangle {
    state->dynamic_player.collision.x + (state->dynamic_player.collision.width * .25f),
    state->dynamic_player.collision.y + (state->dynamic_player.collision.height * .8f),
    state->dynamic_player.collision.width * .5f,
    state->dynamic_player.collision.height * .2f
  };
}
void player_add_exp_to_player(i32 raw_exp) {
  if (not state or state == nullptr) { return; }

  i32 curr = state->dynamic_player.exp_current;
  i32 to_next = state->dynamic_player.exp_to_next_level;
  i32 exp = raw_exp + (raw_exp * state->dynamic_player.stats.at(CHARACTER_STATS_EXP_GAIN).buffer.f32[3]);

  if ( curr + exp >= to_next) 
  {
    state->dynamic_player.exp_current = (curr + exp) - to_next;
    state->dynamic_player.level++;
    state->dynamic_player.exp_to_next_level = level_curve[state->dynamic_player.level];
    state->dynamic_player.is_player_have_ability_upgrade_points = true;
  }
  else {
    state->dynamic_player.exp_current += exp;
  }
  state->dynamic_player.exp_perc = static_cast<f32>(state->dynamic_player.exp_current) / static_cast<f32>(state->dynamic_player.exp_to_next_level);
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_EXPERIANCE, (f32)state->dynamic_player.exp_perc));
}
void player_take_damage(i32 damage) {
  if (not state or state == nullptr) { return; }

  if(!state->dynamic_player.is_damagable || state->dynamic_player.is_dead) return;
  if((state->dynamic_player.health_current - damage) > 0 && (state->dynamic_player.health_current - damage) <= state->dynamic_player.stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3]) {
    state->dynamic_player.health_current -= damage;
    state->dynamic_player.is_damagable = false;
    state->dynamic_player.damage_break_current = state->dynamic_player.damage_break_time;
  }
  else {
    state->dynamic_player.health_current = 0;
    state->dynamic_player.is_dead = true;
    event_fire(EVENT_CODE_END_GAME, event_context());
  }
}
void player_heal_player(i32 amouth){
  if (not state or state == nullptr) { return; }

  if(state->dynamic_player.is_dead) return;
  if(state->dynamic_player.health_current + amouth <= state->dynamic_player.stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3]) {
    state->dynamic_player.health_current += amouth;
  }
  else {
    state->dynamic_player.health_current = state->dynamic_player.stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3];
  }
  state->dynamic_player.health_perc = static_cast<f32>(state->dynamic_player.health_current) / static_cast<f32>(state->dynamic_player.stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3]);
}
void player_update_sprite(void) {
  if (not state or state == nullptr) { return; }

  if( not state->dynamic_player.is_dead ) {
    if(state->dynamic_player.is_damagable) {
      if(state->dynamic_player.is_moving) {
        if (state->dynamic_player.current_anim_to_play.sheet_id != state->dynamic_player.move_right_sprite.sheet_id) {
          state->dynamic_player.current_anim_to_play = state->dynamic_player.move_right_sprite;
        }
			}
      else {
        if (state->dynamic_player.current_anim_to_play.sheet_id != state->dynamic_player.idle_right_sprite.sheet_id) {
          state->dynamic_player.current_anim_to_play = state->dynamic_player.idle_right_sprite;
        }
			}
    }	
    else {
      if (state->dynamic_player.current_anim_to_play.sheet_id != state->dynamic_player.take_damage_right_sprite.sheet_id) {
        state->dynamic_player.current_anim_to_play = state->dynamic_player.take_damage_right_sprite;
      }
    }
  } 
  else {
    if (state->dynamic_player.current_anim_to_play.sheet_id != state->dynamic_player.wreck_right_sprite.sheet_id) {
      state->dynamic_player.current_anim_to_play = state->dynamic_player.wreck_right_sprite;
    }
  }
  spritesheet& _sheet = state->dynamic_player.current_anim_to_play;
  if (state->dynamic_player.w_direction == WORLD_DIRECTION_LEFT && _sheet.current_frame_rect.width > 0) {
    _sheet.current_frame_rect.width *= -1;
  }
  if (state->dynamic_player.w_direction == WORLD_DIRECTION_RIGHT && _sheet.current_frame_rect.width < 0) {
    _sheet.current_frame_rect.width *= -1;
  }
  _sheet.coord = Rectangle { 
    state->dynamic_player.position.x, state->dynamic_player.position.y,
    std::abs(_sheet.current_frame_rect.width) * PLAYER_SCALE, std::abs(_sheet.current_frame_rect.height) * PLAYER_SCALE
  };
  _sheet.origin = Vector2 { _sheet.coord.width * .5f, _sheet.coord.height * .5f};
  
  update_sprite(__builtin_addressof(_sheet));
}

player_state* get_player_state(void) {
  if (not state or state == nullptr) {
    return nullptr;
  }
  return __builtin_addressof(state->dynamic_player);
}
const player_state* get_default_player(void) {
  if (not state or state == nullptr) {
    return nullptr;
  }
  return __builtin_addressof(state->defualt_player);
}

bool player_system_on_event(i32 code, event_context context) {
    switch (code) {
        case EVENT_CODE_PLAYER_ADD_EXP: {
          player_add_exp_to_player(context.data.i32[0]);
          return true;
        }
        case EVENT_CODE_PLAYER_SET_POSITION: {
          state->dynamic_player.position.x = context.data.f32[0]; 
          state->dynamic_player.position.y = context.data.f32[1]; 
          state->dynamic_player.collision.x = state->dynamic_player.position.x; 
          state->dynamic_player.collision.y = state->dynamic_player.position.y; 
          return true;
        }
        case EVENT_CODE_PLAYER_TAKE_DAMAGE: {
          player_take_damage(context.data.i32[0]);
          return true;
        }
        default: return false; // TODO: Warn
    }

    return false;
}
