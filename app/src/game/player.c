#include "player.h"
#include <defines.h>

#include "core/event.h"
#include "core/fmemory.h"

#include "game/spritesheet.h"

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL+1];

static player_state* player;

#define PLAYER_SCALE 3.f

void play_anim(spritesheet_id player_anim_sheet);
void add_exp_to_player(u32 exp);
void take_damage(u16 damage);

bool player_system_on_event(u16 code, event_context context);

bool player_system_initialize(void) {
    if (player) return false;

    player = (player_state*)allocate_memory_linear(sizeof(player_state), true);

    if (!player) {
        TraceLog(LOG_FATAL, "PLAYER_SYSTEM ALLOCATION FAILED");
        return false;
    }

    event_register(EVENT_CODE_PLAYER_ADD_EXP, player_system_on_event);
    event_register(EVENT_CODE_PLAYER_SET_POSITION, player_system_on_event);
    event_register(EVENT_CODE_PLAYER_TAKE_DAMAGE, player_system_on_event);

    player->position.x = 0;
    player->position.y = 0;
    player->dimentions = (Vector2) {22, 32}; // INFO: Hardcoded player dimentions
    player->dimentions.x *= PLAYER_SCALE;
    player->dimentions.y *= PLAYER_SCALE;
    player->dimentions_div2 = (Vector2) {player->dimentions.x/2, player->dimentions.y/2};

    //player->ability_system = ability_manager_initialize(PLAYER, player->dimentions);

    player->collision = (Rectangle)
    {
      .x = player->position.x - player->dimentions.x / 2.f,
      .y = player->position.y - player->dimentions.y / 2.f,
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
    player->health_max = 255;
    player->health_current = player->health_max;
    player->health_perc = (float) player->health_current / player->health_max;

    player->move_left_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_MOVE_LEFT;
    player->move_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_MOVE_RIGHT;
    player->idle_left_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_IDLE_LEFT;
    player->idle_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_IDLE_RIGHT;
    player->take_damage_left_sprite.sheet_id =  SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_LEFT;
    player->take_damage_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT;
    player->wreck_left_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_WRECK_LEFT;
    player->wreck_right_sprite.sheet_id = SHEET_ID_PLAYER_ANIMATION_WRECK_RIGHT;

    set_sprite(&player->move_left_sprite, true, false, true);
    set_sprite(&player->move_right_sprite, true, false, true);
    set_sprite(&player->idle_left_sprite, true, false, true);
    set_sprite(&player->idle_right_sprite, true, false, true);
    set_sprite(&player->take_damage_left_sprite, true, false, true);
    set_sprite(&player->take_damage_right_sprite, true, false, true);
    set_sprite(&player->wreck_left_sprite, true, false, true);
    set_sprite(&player->wreck_right_sprite, true, false, true);

    player->last_played_animation = &player->idle_left_sprite; // The position player starts. To avoid from the error when move firstly called
    
    {
      player->stats[CHARACTER_STATS_UNDEFINED] = (character_stat){0};

      player->stats[CHARACTER_STATS_HEALTH] = (character_stat){
				.id = CHARACTER_STATS_HEALTH,
				.level = 1,
				.buffer.i32 = {0},
				.passive_display_name = "Life Essance",
				.passive_desc = "Essential",
				.passive_icon_src = (Rectangle){512, 320, 32, 32}
			};
      player->stats[CHARACTER_STATS_HP_REGEN] = (character_stat){
				.id = CHARACTER_STATS_HP_REGEN,
				.level = 1,
				.buffer.i32 = {0},
				.passive_display_name = "Bread",
				.passive_desc = "Keeps you fed",
				.passive_icon_src = (Rectangle){288, 288, 32, 32}
			};
      player->stats[CHARACTER_STATS_MOVE_SPEED] = (character_stat){
				.id = CHARACTER_STATS_MOVE_SPEED,
				.level = 1,
				.buffer.i32 = {0},
				.passive_display_name = "Cardinal Boots",
				.passive_desc = "Increases move speed",
				.passive_icon_src = (Rectangle){64, 352, 32, 32}
			};
      player->stats[CHARACTER_STATS_AOE] = (character_stat){
				.id = CHARACTER_STATS_AOE,
				.level = 1,
				.buffer.i32 = {0},
				.passive_display_name = "Blast Scroll",
				.passive_desc = "Increase the area",
				.passive_icon_src = (Rectangle){320, 32, 32, 32}
			};
      player->stats[CHARACTER_STATS_DAMAGE] = (character_stat){
				.id = CHARACTER_STATS_DAMAGE,
				.level = 1,
				.buffer.i32 = {0},
				.passive_display_name = "Heavy Cross",
				.passive_desc = "Increases Damage",
				.passive_icon_src = (Rectangle){288, 32, 32, 32}
			};
      player->stats[CHARACTER_STATS_ABILITY_CD] = (character_stat){
				.id = CHARACTER_STATS_ABILITY_CD,
				.level = 1,
				.buffer.i32 = {0},
				.passive_display_name = "Hourglass",
				.passive_desc = "Reduce CD",
				.passive_icon_src = (Rectangle){128, 64, 32, 32}
			};
      player->stats[CHARACTER_STATS_PROJECTILE_AMOUTH] = (character_stat){
				.id = CHARACTER_STATS_PROJECTILE_AMOUTH,
				.level = 1,
				.buffer.i32 = {0},
				.passive_display_name = "Second Hand",
				.passive_desc = "Increases projectile amouth",
				.passive_icon_src = (Rectangle){64, 448, 32, 32}
			};
      player->stats[CHARACTER_STATS_EXP_GAIN] = (character_stat){
				.id = CHARACTER_STATS_EXP_GAIN,
				.level = 1,
				.buffer.i32 = {0},
				.passive_display_name = "Seeing Eyes",
				.passive_desc = "Increases experiance gain",
				.passive_icon_src = (Rectangle){192, 32, 32, 32}
			};
    }

    player->starter_ability = ABILITY_TYPE_FIREBALL;
    player->is_initialized = true;
    return true;
}

player_state* get_player_state(void) {
  if (!player) {
    return (player_state*)0;
  }
  //TraceLog(LOG_INFO, "player->position:{%f, %f}", player->position.x, player->position.y);
  return player;
}

Vector2 get_player_position(bool centered) {
  Vector2 pos = {0};
  if(centered) {
    pos = (Vector2) {
      .x = player->position.x - player->dimentions_div2.x,
      .y = player->position.y - player->dimentions_div2.y
    };
  }
  else { 
    pos = player->position;
  }
  //TraceLog(LOG_INFO, "(Vector2){%f, %f}", pos.x, pos.y);
  return pos;
}

void add_exp_to_player(u32 exp) {
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
void take_damage(u16 damage) {
  if(!player->is_damagable) return;
  if(player->health_current - damage > 0) {
    player->health_current -= damage;
    player->is_damagable = false;
  }
  else {
    player->health_current = 0;
    player->is_dead = true;
  }
  player->health_perc = (float) player->health_current / player->health_max;
}

bool update_player(void) {
  if (!player) return false;
  if (player->is_dead) {
    event_fire(EVENT_CODE_PAUSE_GAME, (event_context){0});
  }
  if(!player->is_damagable) {
    if(player->damage_break_current - GetFrameTime() > 0) player->damage_break_current -= GetFrameTime();
    else
    {
      player->damage_break_current = player->damage_break_time;
      player->is_damagable = true;
    }
  }
  if (IsKeyDown(KEY_W)) {
    player->position.y -= 2;
    player->is_moving = true;
  }
  if (IsKeyDown(KEY_A)) {
    player->position.x -= 2;
    player->w_direction = WORLD_DIRECTION_LEFT;
    player->is_moving = true;
  }
  if (IsKeyDown(KEY_S)) {
    player->position.y += 2;
    player->is_moving = true;
  }
  if (IsKeyDown(KEY_D)) {
    player->position.x += 2;
    player->w_direction = WORLD_DIRECTION_RIGHT;
    player->is_moving = true;
  }
  if (IsKeyUp(KEY_W) && IsKeyUp(KEY_A) && IsKeyUp(KEY_S) && IsKeyUp(KEY_D)) {    
    player->is_moving = false;
  }
  player->collision.x = player->position.x - player->dimentions_div2.x;
  player->collision.y = player->position.y - player->dimentions_div2.y;
  player->collision.width = player->dimentions.x;
  player->collision.height = player->dimentions.y;

  update_sprite(player->last_played_animation);
  return true;
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
			} else 
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
    }	else 
			{
        (player->w_direction == WORLD_DIRECTION_LEFT) 
            ? play_anim(SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_LEFT)
            : play_anim(SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT);
    	}
  } else 
		{
      (player->w_direction == WORLD_DIRECTION_LEFT) 
          ? play_anim(SHEET_ID_PLAYER_ANIMATION_WRECK_LEFT)
          : play_anim(SHEET_ID_PLAYER_ANIMATION_WRECK_RIGHT);
  	}
  
  //render_abilities(&player->ability_system);
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
    Rectangle dest = (Rectangle) {
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
        
        default: TraceLog(LOG_ERROR, "ERROR::player::move()::move function called with unspecified value.");
        break;
    }
}

bool player_system_on_event(u16 code, event_context context) {
    switch (code) {
        case EVENT_CODE_PLAYER_ADD_EXP: {
            add_exp_to_player(context.data.u32[0]);
            event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, (event_context){
                .data.f32[0] = PRG_BAR_ID_PLAYER_EXPERIANCE,
                .data.f32[1] = player->exp_perc,
            });
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
            take_damage(context.data.u8[0]);
            event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, (event_context){
                .data.f32[0] = PRG_BAR_ID_PLAYER_HEALTH,
                .data.f32[1] = player->health_perc,
            });
            return true;
        }
        default: return false; // TODO: Warn
    }

    return false;
}

