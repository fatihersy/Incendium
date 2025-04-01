#include "spawn.h"

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"

#include "spritesheet.h"

typedef struct spawn_system_state {
  Character2D spawns[MAX_SPAWN_COUNT];
  u16 current_spawn_count;
} spawn_system_state;

static spawn_system_state *state;

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL + 1];

#define CHECK_pSPAWN_COLLISION(REC1, SPAWN, NEW_POS) \
CheckCollisionRecs(REC1, \
  (Rectangle) { \
    NEW_POS.x, NEW_POS.y, \
    SPAWN->collision.width, SPAWN->collision.height\
})

void spawn_play_anim(Character2D* spawn, spritesheet_id sheet);

bool spawn_system_initialize(void) {
  if (state) {
    state->current_spawn_count = 0;
    return true;
  }
  state = (spawn_system_state *)allocate_memory_linear(sizeof(spawn_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "spawn::spawn_system_initialize()::Spawn system init failed");
    return false;
  }

  state->current_spawn_count = 0;
  return true;
}

u16 damage_spawn(u16 _id, u16 damage) {
  Character2D *character = (Character2D*){0};
  Character2D *spawns = state->spawns;
  const u16 spawn_count = state->current_spawn_count;

  for (int i = 0; i < spawn_count ; ++i) {
    if(spawns[i].character_id == _id) {
      character = &spawns[i];
      break;
    }
  }
  if (!character) return INVALID_ID16;

  if(character->health - damage > 0 && character->health - damage < 100) {
    character->is_damagable = false;
    character->damage_break_time = (f32)TARGET_FPS / character->take_damage_left_animation.fps;
    character->health -= damage;
    return character->health;
  }
  character->health = 0;
  character->is_dead = true;
  
  event_fire(EVENT_CODE_ADD_ITEM_PLAYER_CURRENCY_SOULS, (event_context){.data.u32[0] = (u32)character->scale});
  event_fire(EVENT_CODE_PLAYER_ADD_EXP, (event_context){.data.u32[0] = 32}); // TODO: Make exp gain dynamic 
  return 0;
}

bool spawn_character(Character2D _character) {
  if (state->current_spawn_count >= MAX_SPAWN_COUNT) {
    TraceLog(LOG_WARNING, "spawn_character()::Spawn count exceeded");
    return false;
  }
  
  _character.move_left_animation.sheet_id = SHEET_ID_SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT;
  set_sprite(&_character.move_left_animation, true, false, false);
  _character.move_right_animation.sheet_id = SHEET_ID_SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT;
  set_sprite(&_character.move_right_animation, true, false, false);
  _character.take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
  set_sprite(&_character.take_damage_left_animation, true, false, false);
  _character.take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
  set_sprite(&_character.take_damage_right_animation, true, false, false);

  // INFO: Setting collision equal to the any animation dimentions, as we consider each are equal.
  _character.collision.width = _character.move_left_animation.current_frame_rect.width * _character.scale;
  _character.collision.height = _character.move_left_animation.current_frame_rect.height * _character.scale;
  _character.w_direction = WORLD_DIRECTION_RIGHT;

  for (i32 i=0; i<state->current_spawn_count; ++i) {
    if(CheckCollisionRecs(state->spawns[i].collision, (Rectangle) {
      _character.position.x, _character.position.y,
      _character.collision.width, _character.collision.height})) { return false; }
  }

  _character.collision.x = _character.position.x;
  _character.collision.y = _character.position.y;
  _character.character_id = state->current_spawn_count;
  _character.initialized = true;
  
  state->spawns[state->current_spawn_count] = _character;
  state->current_spawn_count++;
  return true;
}

bool update_spawns(Vector2 player_position) {

  for (i32 i = 0; i < state->current_spawn_count; ++i) {
    Character2D *character = &state->spawns[i];
    if (character->health <= 0 || character->health > 100) {
      *character = state->spawns[state->current_spawn_count-1];
      state->spawns[state->current_spawn_count-1] = (Character2D){0};
      if (state->current_spawn_count > 0) {
        state->current_spawn_count--;
      }
      continue;
    }
    update_sprite(character->last_played_animation);
    Vector2 new_position = move_towards(character->position, player_position,character->speed);
    bool x0_collide = false;
    bool y0_collide = false;
    for (i32 j=0; j<state->current_spawn_count; ++j) {
      if (state->spawns[j].character_id == character->character_id) { continue; }

      const Rectangle spw_col = character->collision;
      const Rectangle x0 = (Rectangle) {spw_col.x, new_position.y, spw_col.width, spw_col.height};
      const Rectangle y0 = (Rectangle) {new_position.x, spw_col.y, spw_col.width, spw_col.height};

      if(CheckCollisionRecs(state->spawns[j].collision, x0)) {
        x0_collide = true;
      }
      if(CheckCollisionRecs(state->spawns[j].collision, y0)) {
        y0_collide = true;
      }
    }
    if (!x0_collide) {
      character->position.y = new_position.y;
      character->collision.y = character->position.y;
    }
    if (!y0_collide) {
      if (character->position.x - new_position.x > 0) {
        character->w_direction = WORLD_DIRECTION_LEFT;
      } else {
        character->w_direction = WORLD_DIRECTION_RIGHT;
      }
      character->position.x = new_position.x;
      character->collision.x = character->position.x;
    }
    event_fire(EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE, (event_context) {
      .data.u16[0] = character->collision.x, .data.u16[1] = character->collision.y,
      .data.u16[2] = character->collision.width, .data.u16[3] = character->collision.height,
      .data.u16[4] = character->damage,
    });
    if (!character->is_damagable) {
      if (character->damage_break_time >= 0) {
        character->damage_break_time -= GetFrameTime();
      }
      else character->is_damagable = true;
    }
  }
  return true;
}

bool render_spawns(void) {

  if (state->current_spawn_count > MAX_SPAWN_COUNT) {
    TraceLog(LOG_WARNING, "spawn::render_spawns()::Spawn count is out of bounds");
    return false;
  }
  // Enemies
  for (i32 i = 0; i < state->current_spawn_count; ++i) {
    if (state->spawns[i].initialized) {
      if(!state->spawns[i].is_dead) 
      {
        if(state->spawns[i].is_damagable)
        {
          switch (state->spawns[i].w_direction) 
          {
            case WORLD_DIRECTION_LEFT: spawn_play_anim(&state->spawns[i], SHEET_ID_SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT);
            break;
            case WORLD_DIRECTION_RIGHT:spawn_play_anim(&state->spawns[i], SHEET_ID_SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT);
            break;
            default: {
              TraceLog(LOG_WARNING, "spawn::render_spawns()::Spawn has no directions");
              break;
            }
          }
        }	
        else 
        {
          (state->spawns[i].w_direction == WORLD_DIRECTION_LEFT) 
            ? spawn_play_anim(&state->spawns[i], SHEET_ID_SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT)
            : spawn_play_anim(&state->spawns[i], SHEET_ID_SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT);
        }
      } 
      else 
      {
        (state->spawns[i].w_direction == WORLD_DIRECTION_LEFT) 
          ? spawn_play_anim(&state->spawns[i], SHEET_ID_SPAWN_ZOMBIE_ANIMATION_WRECK_LEFT)
          : spawn_play_anim(&state->spawns[i], SHEET_ID_SPAWN_ZOMBIE_ANIMATION_WRECK_RIGHT);
      }
    }
  }
  return true;
}

Character2D *get_spawns() {
  if (!state) {
    TraceLog(LOG_ERROR, "spawn::get_spawns()::State was null");
    return 0;
  }

  return state->spawns;
}
u16 *get_spawn_count() {
  if (!state) {
    TraceLog(LOG_ERROR, "spawn::get_spawn_count()::State was null");
    return 0;
  }

  return &state->current_spawn_count;
}

void clean_up_spawn_state(void) {
  for (u16 i = 0; i < state->current_spawn_count; i++) {
    state->spawns[i] = (Character2D){0};
  }

  state->current_spawn_count = 0;

  // state = (spawn_system_state*){0};
}

void spawn_play_anim(Character2D* spawn, spritesheet_id sheet) {
  Rectangle dest = spawn->collision;

  switch (sheet) {
    case SHEET_ID_SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT: {
      play_sprite_on_site(&spawn->move_left_animation, WHITE, dest);
      spawn->last_played_animation = &spawn->move_left_animation;
      break;
    }
    case SHEET_ID_SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT: {
      play_sprite_on_site(&spawn->move_right_animation, WHITE, dest);
      spawn->last_played_animation = &spawn->move_right_animation;
      break;
    }
    case SHEET_ID_SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT:  {
      play_sprite_on_site(&spawn->take_damage_left_animation, WHITE, dest);
      spawn->last_played_animation = &spawn->take_damage_left_animation;
      break;
    }
    case SHEET_ID_SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT:  {
      play_sprite_on_site(&spawn->take_damage_right_animation, WHITE, dest);
      spawn->last_played_animation = &spawn->take_damage_right_animation;
      break;
    }

    default: TraceLog(LOG_ERROR, "spawn::spawn_play_anim()::Unsupported sheet id");
    break;
  }
}
 
