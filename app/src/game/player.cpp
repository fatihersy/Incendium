#include "player.h"

#include "core/event.h"
#include "core/fmemory.h"
#include "loc_types.h"

#include "game/spritesheet.h"

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL+1];
#define PLAYER_SCALE 3

typedef struct player_system_state {
  const camera_metrics * in_camera_metrics;
  const app_settings * in_app_settings;
  const ingame_info * in_ingame_info;
} player_system_state;

static player_state* player = nullptr;
static player_system_state* state = nullptr;

bool player_system_on_event(i32 code, event_context context);
void player_system_reinit(void);
void play_anim(spritesheet_id player_anim_sheet);
void player_update_sprite(void);

bool player_system_initialize(const camera_metrics* in_camera_metrics,const app_settings* in_app_settings,const ingame_info* in_ingame_info) {
  if (player){ 
    player_system_reinit();
    return true;
  }
  player = (player_state*)allocate_memory_linear(sizeof(player_state), true);
  if (!player) {
    TraceLog(LOG_ERROR, "player::player_system_initialize()::Failed to allocate player state");
    return false;
  }
  *player = player_state();
  state = (player_system_state*)allocate_memory_linear(sizeof(player_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "player::player_system_initialize()::Failed to allocate player system");
    return false;
  }
  *state = player_system_state();
  state->in_camera_metrics = in_camera_metrics;
  state->in_app_settings = in_app_settings;
  state->in_ingame_info = in_ingame_info;

  player->move_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_MOVE_RIGHT;
  player->idle_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_IDLE_RIGHT;
  player->take_damage_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT;
  player->wreck_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_WRECK_RIGHT;
  player->attack_1_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ATTACK_1;
  player->attack_2_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ATTACK_2;
  player->attack_3_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ATTACK_3;
  player->attack_down_sprite.sheet_id =  SHEET_ID_PLAYER_ANIMATION_ATTACK_DOWN;
  player->attack_up_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ATTACK_UP;
  player->roll_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_ROLL;
  player->dash_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_DASH;
  set_sprite(&player->move_right_sprite,        true, false);
  set_sprite(&player->idle_right_sprite,        true, false);
  set_sprite(&player->take_damage_right_sprite, true, false);
  set_sprite(&player->wreck_right_sprite,       true, false);
  set_sprite(&player->attack_1_sprite,          true, false);
  set_sprite(&player->attack_2_sprite,          true, false);
  set_sprite(&player->attack_3_sprite,          true, false);
  set_sprite(&player->attack_down_sprite,       true, false);
  set_sprite(&player->attack_up_sprite,         true, false);
  set_sprite(&player->roll_sprite,              true, false);
  set_sprite(&player->dash_sprite,              true, false);

  {
    player->stats.at(CHARACTER_STATS_HEALTH) = character_stat(CHARACTER_STATS_HEALTH, 
      LOC_TEXT_PLAYER_STAT_LIFE_ESSENCE, LOC_TEXT_PLAYER_STAT_DESC_LIFE_ESSENCE, Rectangle{2016, 640, 32, 32}, (i32) level_curve[1]
    );
    player->stats.at(CHARACTER_STATS_HP_REGEN) = character_stat(CHARACTER_STATS_HP_REGEN, 
      LOC_TEXT_PLAYER_STAT_BREAD, LOC_TEXT_PLAYER_STAT_DESC_BREAD, Rectangle{1856, 896, 32, 32}, (i32) level_curve[1]
    );
    player->stats.at(CHARACTER_STATS_MOVE_SPEED) = character_stat(CHARACTER_STATS_MOVE_SPEED, 
      LOC_TEXT_PLAYER_STAT_CARDINAL_BOOTS, LOC_TEXT_PLAYER_STAT_DESC_CARDINAL_BOOTS, Rectangle{1632, 960, 32, 32}, (i32) level_curve[1]
    );
    player->stats.at(CHARACTER_STATS_AOE) =  character_stat(CHARACTER_STATS_AOE, 
      LOC_TEXT_PLAYER_STAT_BLAST_SCROLL, LOC_TEXT_PLAYER_STAT_DESC_BLAST_SCROLL, Rectangle{1888, 640, 32, 32}, (i32)  level_curve[1]
    );
    player->stats.at(CHARACTER_STATS_DAMAGE) =  character_stat(CHARACTER_STATS_DAMAGE, 
      LOC_TEXT_PLAYER_STAT_HEAVY_CROSS, LOC_TEXT_PLAYER_STAT_DESC_HEAVY_CROSS, Rectangle{1856, 640, 32, 32}, (i32) level_curve[1]
    );
    player->stats.at(CHARACTER_STATS_ABILITY_CD) = character_stat(CHARACTER_STATS_ABILITY_CD, 
      LOC_TEXT_PLAYER_STAT_HOURGLASS, LOC_TEXT_PLAYER_STAT_DESC_HOURGLASS, Rectangle{1696, 672, 32, 32}, (i32)level_curve[1]
    );
    player->stats.at(CHARACTER_STATS_PROJECTILE_AMOUTH) = character_stat(CHARACTER_STATS_PROJECTILE_AMOUTH, 
      LOC_TEXT_PLAYER_STAT_SECOND_HAND, LOC_TEXT_PLAYER_STAT_DESC_SECOND_HAND, Rectangle{1632, 1056, 32, 32}, (i32) level_curve[1]
    );
    player->stats.at(CHARACTER_STATS_EXP_GAIN) =  character_stat(CHARACTER_STATS_EXP_GAIN, 
      LOC_TEXT_PLAYER_STAT_SEEING_EYES, LOC_TEXT_PLAYER_STAT_DESC_SEEING_EYES, Rectangle{1760, 640, 32, 32}, (i32) level_curve[1]
    );
    player->stats.at(CHARACTER_STATS_TOTAL_TRAIT_POINTS) =  character_stat(CHARACTER_STATS_TOTAL_TRAIT_POINTS, 
      LOC_TEXT_PLAYER_STAT_TOTAL_TRAIT_POINTS, LOC_TEXT_PLAYER_STAT_DESC_TOTAL_TRAIT_POINTS, Rectangle{2272, 640, 32, 32}, (i32) level_curve[1],
      data128(static_cast<i32>(7))
    );
  }

  event_register(EVENT_CODE_PLAYER_ADD_EXP, player_system_on_event);
  event_register(EVENT_CODE_PLAYER_SET_POSITION, player_system_on_event);
  event_register(EVENT_CODE_PLAYER_TAKE_DAMAGE, player_system_on_event);

  player_system_reinit();

  return true;
}
void player_system_reinit(void) {
  
  player->position = ZEROVEC2;
  player->collision.width = (player->idle_right_sprite.coord.width * .9f) * PLAYER_SCALE; // INFO: player collision scales with idle spritesheet
  player->collision.height = (player->idle_right_sprite.coord.height * .9f) * PLAYER_SCALE;
  player->is_dead = false;
  player->is_moving = false;
  player->w_direction = WORLD_DIRECTION_LEFT;
  player->is_damagable = true;
  player->damage_break_time = .2f; //ms
  player->damage_break_current = player->damage_break_time;
  player->level = 1;
  player->exp_to_next_level = level_curve[player->level];
  player->exp_current = 0;
  player->stats.at(CHARACTER_STATS_HEALTH).buffer.i32[0] = 0; // INFO: Will be Modified by player stats
  player->health_current = 0;
  player->health_perc = 0.f;
  
  player->starter_ability = ABILITY_TYPE_FIREBALL;
  player->is_initialized = true;
}

player_update_results update_player(void) {
  if (!player) return player_update_results();
  if (player->is_dead) { 
    return player_update_results(); 
  }
  
  if(!player->is_damagable) {
    if(player->damage_break_current <= 0) { player->is_damagable = true; }
    else { player->damage_break_current -= GetFrameTime(); }
  }
  Vector2 new_position = ZEROVEC2;
  if (IsKeyDown(KEY_W)) {
    new_position.y -= player->stats.at(CHARACTER_STATS_MOVE_SPEED).buffer.f32[3] * GetFrameTime();
  }
  if (IsKeyDown(KEY_A)) {
    new_position.x -= player->stats.at(CHARACTER_STATS_MOVE_SPEED).buffer.f32[3] * GetFrameTime();
  }
  if (IsKeyDown(KEY_S)) {
    new_position.y += player->stats.at(CHARACTER_STATS_MOVE_SPEED).buffer.f32[3] * GetFrameTime();
  }
  if (IsKeyDown(KEY_D)) {
    new_position.x += player->stats.at(CHARACTER_STATS_MOVE_SPEED).buffer.f32[3] * GetFrameTime();
  }
  player_update_sprite();
  
  return player_update_results(new_position, true);
}
bool render_player(void) {
  if (!player) { return false; }
  spritesheet& _sheet = player->current_anim_to_play;
  const Rectangle& frame_rect = player->current_anim_to_play.current_frame_rect;
  
  play_sprite_on_site_ex(
    __builtin_addressof(_sheet), 
    Rectangle {frame_rect.x + _sheet.offset.x, frame_rect.y + _sheet.offset.y, frame_rect.width, frame_rect.height}, 
    player->current_anim_to_play.coord, 
    player->current_anim_to_play.origin, 
    player->current_anim_to_play.rotation, 
    WHITE
  );
  DrawRectangleLines(static_cast<i32>(player->collision.x),static_cast<i32>(player->collision.y),static_cast<i32>(player->collision.width),static_cast<i32>(player->collision.height), WHITE);

  #if DEBUG_COLLISIONS
    DrawRectangleLines(static_cast<i32>(player->collision.x),static_cast<i32>(player->collision.y),static_cast<i32>(player->collision.width),static_cast<i32>(player->collision.height), WHITE);
  #endif
  return true;
}

void player_move_player(Vector2 new_pos) {
  if (!player) {
    TraceLog(LOG_ERROR, "player::player_move_player()::Player state is null");
    return;
  }
  if (new_pos.x < 0.f) {
    player->w_direction = WORLD_DIRECTION_LEFT;
  }
  else if (new_pos.x > 0.f) {
    player->w_direction = WORLD_DIRECTION_RIGHT;
  }
  else if(new_pos.x == 0.f && new_pos.y == 0.f) {
    player->is_moving = false;
    return;
  }
  
  player->is_moving = true;
  player->position.x += new_pos.x;
  player->position.y += new_pos.y;

  player->collision = Rectangle {
    player->position.x - (player->collision.width * .5f),
    player->position.y - (player->collision.height * .5f),
    player->collision.width,
    player->collision.height
  };

  player->map_level_collision = Rectangle {
    player->collision.x + (player->collision.width * .25f),
    player->collision.y + (player->collision.height * .8f),
    player->collision.width * .5f,
    player->collision.height * .2f
  };
}
void player_add_exp_to_player(i32 exp) {
  i32 curr = player->exp_current;
  i32 to_next = player->exp_to_next_level;
  if ( curr + exp >= to_next) 
  {
    player->exp_current = (curr + exp) - to_next;
    player->level++;
    player->exp_to_next_level = level_curve[player->level];
    player->is_player_have_ability_upgrade_points = true;
  }
  else {
    player->exp_current += exp;
  }
  player->exp_perc = static_cast<f32>(player->exp_current) / static_cast<f32>(player->exp_to_next_level);
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_EXPERIANCE, (f32)player->exp_perc));
}
void player_take_damage(i32 damage) {
  if(!player->is_damagable || player->is_dead) return;
  if((player->health_current - damage) > 0 && (player->health_current - damage) <= player->stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3]) {
    player->health_current -= damage;
    player->is_damagable = false;
    player->damage_break_current = player->damage_break_time;
  }
  else {
    player->health_current = 0;
    player->is_dead = true;
    event_fire(EVENT_CODE_END_GAME, event_context());
  }
  player->health_perc = static_cast<f32>(player->health_current) / static_cast<f32>(player->stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3]);
  
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_HEALTH, (f32)player->health_perc));
}
void player_heal_player(i32 amouth){
  if(player->is_dead) return;
  if(player->health_current + amouth <= player->stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3]) {
    player->health_current += amouth;
  }
  else {
    player->health_current = player->stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3];
  }
  player->health_perc = static_cast<f32>(player->health_current) / static_cast<f32>(player->stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3]);
}
void player_update_sprite(void) {
  if (!player || player == nullptr) { return; }

  if( not player->is_dead ) {
    if(player->is_damagable) {
      if(player->is_moving) {
        if (player->current_anim_to_play.sheet_id != player->move_right_sprite.sheet_id) {
          player->current_anim_to_play = player->move_right_sprite;
        }
			}
      else {
        if (player->current_anim_to_play.sheet_id != player->idle_right_sprite.sheet_id) {
          player->current_anim_to_play = player->idle_right_sprite;
        }
			}
    }	
    else {
      if (player->current_anim_to_play.sheet_id != player->take_damage_right_sprite.sheet_id) {
        player->current_anim_to_play = player->take_damage_right_sprite;
      }
    }
  } 
  else {
    if (player->current_anim_to_play.sheet_id != player->wreck_right_sprite.sheet_id) {
      player->current_anim_to_play = player->wreck_right_sprite;
    }
  }
  spritesheet& _sheet = player->current_anim_to_play;
  if (player->w_direction == WORLD_DIRECTION_LEFT && _sheet.current_frame_rect.width > 0) {
    _sheet.current_frame_rect.width *= -1;
  }
  if (player->w_direction == WORLD_DIRECTION_RIGHT && _sheet.current_frame_rect.width < 0) {
    _sheet.current_frame_rect.width *= -1;
  }
  _sheet.coord = Rectangle { 
    player->position.x, player->position.y,
    std::abs(_sheet.current_frame_rect.width) * PLAYER_SCALE, std::abs(_sheet.current_frame_rect.height) * PLAYER_SCALE
  };
  _sheet.origin = Vector2 { _sheet.coord.width * .5f, _sheet.coord.height * .5f};
  
  update_sprite(__builtin_addressof(_sheet));
}

player_state* get_player_state(void) {
  if (!player) {
    return nullptr;
  }
  return player;
}

bool player_system_on_event(i32 code, event_context context) {
    switch (code) {
        case EVENT_CODE_PLAYER_ADD_EXP: {
          player_add_exp_to_player(context.data.i32[0]);
          return true;
        }
        case EVENT_CODE_PLAYER_SET_POSITION: {
          player->position.x = context.data.f32[0]; 
          player->position.y = context.data.f32[1]; 
          player->collision.x = player->position.x; 
          player->collision.y = player->position.y; 
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
