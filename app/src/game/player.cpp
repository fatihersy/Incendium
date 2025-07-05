#include "player.h"

#include "core/event.h"
#include "core/fmemory.h"
#include "loc_types.h"

#include "game/spritesheet.h"

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL+1];
#define PLAYER_SCALE 3.f

static player_state* player;

bool player_system_on_event(u16 code, event_context context);
void player_system_reinit(void);
void play_anim(spritesheet_id player_anim_sheet);

bool player_system_initialize(void) {
  if (player){ 
    player_system_reinit();
    return true;
  }
  player = (player_state*)allocate_memory_linear(sizeof(player_state), true);
  if (!player) {
    TraceLog(LOG_ERROR, "player::player_system_initialize()::Failed to allocate player state");
    return false;
  }
  player->move_left_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_MOVE_LEFT;
  player->move_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_MOVE_RIGHT;
  player->idle_left_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_IDLE_LEFT;
  player->idle_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_IDLE_RIGHT;
  player->take_damage_left_sprite.sheet_id =  SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_LEFT;
  player->take_damage_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT;
  player->wreck_left_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_WRECK_LEFT;
  player->wreck_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_WRECK_RIGHT;
  set_sprite(&player->move_left_sprite,         true, false);
  set_sprite(&player->move_right_sprite,        true, false);
  set_sprite(&player->idle_left_sprite,         true, false);
  set_sprite(&player->idle_right_sprite,        true, false);
  set_sprite(&player->take_damage_left_sprite,  true, false);
  set_sprite(&player->take_damage_right_sprite, true, false);
  set_sprite(&player->wreck_left_sprite,        true, false);
  set_sprite(&player->wreck_right_sprite,       true, false);

  {
    player->stats[CHARACTER_STATS_HEALTH] = character_stat(CHARACTER_STATS_HEALTH, 
      LOC_TEXT_PLAYER_STAT_LIFE_ESSENCE, LOC_TEXT_PLAYER_STAT_DESC_LIFE_ESSENCE, Rectangle{2016, 640, 32, 32}, (i32) level_curve[1]
    );
    player->stats[CHARACTER_STATS_HP_REGEN] = character_stat(CHARACTER_STATS_HP_REGEN, 
      LOC_TEXT_PLAYER_STAT_BREAD, LOC_TEXT_PLAYER_STAT_DESC_BREAD, Rectangle{1856, 896, 32, 32}, (i32) level_curve[1]
    );
    player->stats[CHARACTER_STATS_MOVE_SPEED] = character_stat(CHARACTER_STATS_MOVE_SPEED, 
      LOC_TEXT_PLAYER_STAT_CARDINAL_BOOTS, LOC_TEXT_PLAYER_STAT_DESC_CARDINAL_BOOTS, Rectangle{1632, 960, 32, 32}, (i32) level_curve[1]
    );
    player->stats[CHARACTER_STATS_AOE] =  character_stat(CHARACTER_STATS_AOE, 
      LOC_TEXT_PLAYER_STAT_BLAST_SCROLL, LOC_TEXT_PLAYER_STAT_DESC_BLAST_SCROLL, Rectangle{1888, 640, 32, 32}, (i32)  level_curve[1]
    );
    player->stats[CHARACTER_STATS_DAMAGE] =  character_stat(CHARACTER_STATS_DAMAGE, 
      LOC_TEXT_PLAYER_STAT_HEAVY_CROSS, LOC_TEXT_PLAYER_STAT_DESC_HEAVY_CROSS, Rectangle{1856, 640, 32, 32}, (i32) level_curve[1]
    );
    player->stats[CHARACTER_STATS_ABILITY_CD] = character_stat(CHARACTER_STATS_ABILITY_CD, 
      LOC_TEXT_PLAYER_STAT_HOURGLASS, LOC_TEXT_PLAYER_STAT_DESC_HOURGLASS, Rectangle{1696, 672, 32, 32}, (i32)level_curve[1]
    );
    player->stats[CHARACTER_STATS_PROJECTILE_AMOUTH] = character_stat(CHARACTER_STATS_PROJECTILE_AMOUTH, 
      LOC_TEXT_PLAYER_STAT_SECOND_HAND, LOC_TEXT_PLAYER_STAT_DESC_SECOND_HAND, Rectangle{1632, 1056, 32, 32}, (i32) level_curve[1]
    );
    player->stats[CHARACTER_STATS_EXP_GAIN] =  character_stat(CHARACTER_STATS_EXP_GAIN, 
      LOC_TEXT_PLAYER_STAT_SEEING_EYES, LOC_TEXT_PLAYER_STAT_DESC_SEEING_EYES, Rectangle{1760, 640, 32, 32}, (i32) level_curve[1]
    );
  }

  event_register(EVENT_CODE_PLAYER_ADD_EXP, player_system_on_event);
  event_register(EVENT_CODE_PLAYER_SET_POSITION, player_system_on_event);
  event_register(EVENT_CODE_PLAYER_TAKE_DAMAGE, player_system_on_event);

  player_system_reinit();

  return true;
}
void player_system_reinit(void) {
  
  player->position.x = 0;
  player->position.y = 0;
  player->dimentions = Vector2{22, 32};
  player->dimentions.x *= PLAYER_SCALE;
  player->dimentions.y *= PLAYER_SCALE;
  player->dimentions_div2 = Vector2{ player->dimentions.x * .5f, player->dimentions.y * .5f };
  player->collision = Rectangle {
    .x = player->position.x - player->dimentions_div2.x,
    .y = player->position.y - player->dimentions_div2.y,
    .width = player->dimentions.x,
    .height = player->dimentions.y
  };
  player->is_dead = false;
  player->is_moving = false;
  player->w_direction = WORLD_DIRECTION_LEFT;
  player->is_damagable = true;
  player->damage_break_time = .2; //ms
  player->damage_break_current = player->damage_break_time;
  player->level = 1;
  player->exp_to_next_level = level_curve[player->level];
  player->exp_current = 0;
  player->health_max = 0; // INFO: Will be Modified by player stats
  player->health_current = player->health_max;
  player->health_perc = (float) player->health_current / player->health_max;

  player->last_played_animation = &player->idle_left_sprite; // The position player starts. To avoid from the error when move firstly called
  
  player->starter_ability = ABILITY_TYPE_FIRETRAIL;
  player->is_initialized = true;
}

player_state* get_player_state(void) {
  if (!player) {
    return nullptr;
  }
  //TraceLog(LOG_INFO, "player->position:{%f, %f}", player->position.x, player->position.y);
  return player;
}

Vector2 get_player_position(bool centered) {
  Vector2 pos = ZEROVEC2;
  if(centered) {
    pos = Vector2 {
      .x = player->position.x + player->dimentions_div2.x,
      .y = player->position.y + player->dimentions_div2.y
    };
  }
  else { 
    pos = player->position;
  }
  //TraceLog(LOG_INFO, "(Vector2){%f, %f}", pos.x, pos.y);
  return pos;
}

void player_add_exp_to_player(u32 exp) {
  u32 curr = player->exp_current;
  u32 to_next = player->exp_to_next_level;
  if ( curr + exp >= to_next) 
  {
    player->exp_current = (curr + exp) - to_next;
    player->level++;
    player->exp_to_next_level = level_curve[player->level];
    player->is_player_have_ability_upgrade_points = true;
    //play_sprite_on_player(player->);
  }
  else {
    player->exp_current += exp;
  }
  player->exp_perc = (float) player->exp_current / player->exp_to_next_level;
}
void player_take_damage(u16 damage) {
  if(!player->is_damagable || player->is_dead) return;
  if((player->health_current - damage) > 0 && (player->health_current - damage) <= player->health_max) {
    player->health_current -= damage;
    player->is_damagable = false;
    player->damage_break_current = player->damage_break_time;
  }
  else {
    player->health_current = 0;
    player->is_dead = true;
    event_fire(EVENT_CODE_END_GAME, event_context());
  }
  player->health_perc = (float) player->health_current / player->health_max;
}
void player_heal_player(u16 amouth){
  if(player->is_dead) return;
  if(player->health_current + amouth <= player->health_max) {
    player->health_current += amouth;
  }
  else {
    player->health_current = player->health_max;
  }
  player->health_perc = (f32) player->health_current / player->health_max;
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
    new_position.y -= 2;
  }
  if (IsKeyDown(KEY_A)) {
    new_position.x -= 2;
  }
  if (IsKeyDown(KEY_S)) {
    new_position.y += 2;
  }
  if (IsKeyDown(KEY_D)) {
    new_position.x += 2;
  }
  update_sprite(player->last_played_animation);
  
  return player_update_results(new_position, true);
}
void player_move_player(Vector2 new_pos) {
  if (!player) {
    TraceLog(LOG_ERROR, "player::move_player()::Player state is null");
  }
  if (new_pos.x < 0) {
    player->w_direction = WORLD_DIRECTION_LEFT;
  }
  else if (new_pos.x > 0) {
    player->w_direction = WORLD_DIRECTION_RIGHT;
  }
  else if(new_pos.x == 0 && new_pos.y == 0) {
    player->is_moving = false;
    return;
  }
  player->is_moving = true;
  player->position.x += new_pos.x;
  player->position.y += new_pos.y;

  player->collision.x = player->position.x;
  player->collision.y = player->position.y;
  player->collision.width = player->dimentions.x;
  player->collision.height = player->dimentions.y;
}

bool render_player(void) {
  if (!player) { return false; }

  if(!player->is_dead) 
	{
    if(player->is_damagable)
		{
      if(player->is_moving) 
			{
				switch (player->w_direction) 
      	{
        	case WORLD_DIRECTION_LEFT: play_anim(SHEET_ID_PLAYER_ANIMATION_MOVE_LEFT);
        	break;
        	case WORLD_DIRECTION_RIGHT: play_anim(SHEET_ID_PLAYER_ANIMATION_MOVE_RIGHT);
        	break;
        	default: {
            TraceLog(LOG_WARNING, "player::render_player()::Player has no directions");
            break;
        	}
      	}
			} 
      else 
			{
				switch (player->w_direction) {
        	case WORLD_DIRECTION_LEFT: play_anim(SHEET_ID_PLAYER_ANIMATION_IDLE_LEFT);
        	break;
        	case WORLD_DIRECTION_RIGHT:play_anim(SHEET_ID_PLAYER_ANIMATION_IDLE_RIGHT);
        	break;
        	default: {
        		TraceLog(LOG_WARNING, "player::render_player()::Player has no directions");
        		break;
        	}
      	}
			}
    }	
    else 
		{
      (player->w_direction == WORLD_DIRECTION_LEFT) 
        ? play_anim(SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_LEFT)
        : play_anim(SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT);
    }
  } 
  else 
	{
    (player->w_direction == WORLD_DIRECTION_LEFT) 
      ? play_anim(SHEET_ID_PLAYER_ANIMATION_WRECK_LEFT)
      : play_anim(SHEET_ID_PLAYER_ANIMATION_WRECK_RIGHT);
  }
  
  #if DEBUG_COLLISIONS
      DrawRectangleLines(
          player->collision.x,
          player->collision.y,
          player->collision.width,
          player->collision.height,
          WHITE);
  #endif
  return true;
}

void play_anim(spritesheet_id player_anim_sheet) {
  Rectangle dest = Rectangle {
    .x = player->position.x,
    .y = player->position.y,
    .width = player->dimentions.x,
    .height = player->dimentions.y
  };
  switch (player_anim_sheet) {
    case SHEET_ID_PLAYER_ANIMATION_MOVE_LEFT: {
      play_sprite_on_site(&player->move_left_sprite, WHITE, dest);
      player->last_played_animation = &player->move_left_sprite;
      break;
    }
    case SHEET_ID_PLAYER_ANIMATION_MOVE_RIGHT: {
      play_sprite_on_site(&player->move_right_sprite, WHITE, dest);
      player->last_played_animation = &player->move_right_sprite;
      break;
    }
    case SHEET_ID_PLAYER_ANIMATION_IDLE_LEFT:  {
      play_sprite_on_site(&player->idle_left_sprite, WHITE, dest);
      player->last_played_animation = &player->idle_left_sprite;
      break;
    }
    case SHEET_ID_PLAYER_ANIMATION_IDLE_RIGHT:  {
      play_sprite_on_site(&player->idle_right_sprite, WHITE, dest);
      player->last_played_animation = &player->idle_right_sprite;
      break;
    }
    case SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_LEFT:  {
      play_sprite_on_site(&player->take_damage_left_sprite, WHITE, dest);
      player->last_played_animation = &player->take_damage_left_sprite;
      break;
    }
    case SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT:  {
      play_sprite_on_site(&player->take_damage_right_sprite, WHITE, dest);
      player->last_played_animation = &player->take_damage_right_sprite;
      break;
    }
    case SHEET_ID_PLAYER_ANIMATION_WRECK_LEFT:  {
      play_sprite_on_site(&player->wreck_left_sprite, WHITE, dest);
      player->last_played_animation = &player->wreck_left_sprite;
      break;
    }
    case SHEET_ID_PLAYER_ANIMATION_WRECK_RIGHT:  {
      play_sprite_on_site(&player->wreck_right_sprite, WHITE, dest);
      player->last_played_animation = &player->wreck_right_sprite;
      break;
    }
    
    default: TraceLog(LOG_ERROR, "player::move()::move function called with unspecified value.");
    break;
  }
}

bool player_system_on_event(u16 code, event_context context) {
    switch (code) {
        case EVENT_CODE_PLAYER_ADD_EXP: {
          player_add_exp_to_player(context.data.u32[0]);
          event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_EXPERIANCE, (f32)player->exp_perc));
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
          event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_HEALTH, (f32)player->health_perc));
          return true;
        }
        default: return false; // TODO: Warn
    }

    return false;
}

