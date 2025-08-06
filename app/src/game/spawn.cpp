#include "spawn.h"

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"

#include "spritesheet.h"

typedef enum spawn_movement_animations {
  SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT,
  SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT,
  SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,
  SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT,
} spawn_movement_animations; 

typedef struct spawn_system_state {
  std::vector<Character2D> spawns; // NOTE: See also clean-up function
  const camera_metrics* in_camera_metrics;
  i32 next_spawn_id;
} spawn_system_state;

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL + 1];
static spawn_system_state * state = nullptr;

#define CHECK_pSPAWN_COLLISION(REC1, SPAWN, NEW_POS) \
CheckCollisionRecs(REC1, \
  (Rectangle) { \
    NEW_POS.x, NEW_POS.y, \
    SPAWN->collision.width, SPAWN->collision.height\
})
#define SPAWN_RND_SCALE_MIN 1.5f
#define SPAWN_RND_SCALE_MAX 2.5f
#define SPAWN_SCALE_INCREASE_BY_LEVEL(LEVEL) LEVEL * .1

#define SPAWN_BASE_HEALTH 300
#define SPAWN_HEALTH_CURVE(LEVEL, SCALE, TYPE) (SPAWN_BASE_HEALTH + (SPAWN_BASE_HEALTH * ((SCALE * 0.4f) + (LEVEL * 0.6f) + (TYPE * 0.3f)) * level_curve[LEVEL] * 0.002f))

#define SPAWN_BASE_DAMAGE 170
#define SPAWN_DAMAGE_CURVE(LEVEL, SCALE, TYPE) (SPAWN_BASE_DAMAGE + (SPAWN_BASE_DAMAGE * ((SCALE * 0.45f) + (LEVEL * 0.6f) + (TYPE * 0.3f)) * level_curve[LEVEL] * 0.002f))

#define SPAWN_BASE_SPEED 50
#define SPAWN_SPEED_CURVE(LEVEL, SCALE, TYPE) (SPAWN_BASE_SPEED + ((SPAWN_BASE_SPEED * (LEVEL + (LEVEL * 0.2f)) + (TYPE + (TYPE * 0.25f))) / SCALE))

void spawn_play_anim(Character2D* spawn, spawn_movement_animations sheet);
void remove_spawn(i32 index);
void register_spawn_animation(Character2D* spawn, spawn_movement_animations movement);

bool spawn_system_initialize(const camera_metrics* _camera_metrics) {
  if (state) {
    clean_up_spawn_state();
    return true;
  }
  state = (spawn_system_state *)allocate_memory_linear(sizeof(spawn_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "spawn::spawn_system_initialize()::Spawn system init failed");
    return false;
  }
  state->in_camera_metrics = _camera_metrics;
  return true;
}

i32 damage_spawn(i32 _id, i32 damage) {
  Character2D* character = nullptr;
  std::vector<Character2D>& spawns = state->spawns;

  for (size_t itr_000 = 0; itr_000 < spawns.size() ; ++itr_000) {
    if(spawns.at(itr_000).character_id == _id) {
      character = __builtin_addressof(spawns.at(itr_000));
      break;
    }
  }
  if (!character) return INVALID_IDI32;
  if (!character->is_damagable) { return character->health; }

  if(character->health - damage > 0 && character->health - damage < MAX_SPAWN_HEALTH) {
    character->is_damagable = false;
    character->damage_break_time = character->take_damage_left_animation.fps / static_cast<f32>(TARGET_FPS);
    character->health -= damage;
    return character->health;
  }
  character->health = 0;
  character->is_dead = true;
  character->is_damagable = false;
  character->damage_break_time = character->take_damage_left_animation.fps / static_cast<f32>(TARGET_FPS);
  
  event_fire(EVENT_CODE_ADD_CURRENCY_SOULS, event_context((i32)character->scale));
  event_fire(EVENT_CODE_PLAYER_ADD_EXP, event_context(static_cast<i32>(32))); // TODO: Make exp gain dynamic 
  return 0;
}

bool spawn_character(Character2D _character) {
  if (_character.buffer.i32[0] <= SPAWN_TYPE_UNDEFINED || _character.buffer.i32[0] >= SPAWN_TYPE_MAX) {
    return false;
  }
  spawn_type spw_type = static_cast<spawn_type>(_character.buffer.i32[0]);
  _character.scale =  SPAWN_RND_SCALE_MIN + (SPAWN_RND_SCALE_MAX * _character.buffer.i32[2] / 100.f);
  _character.scale += SPAWN_SCALE_INCREASE_BY_LEVEL(_character.buffer.i32[1]);

  _character.health = SPAWN_HEALTH_CURVE(_character.buffer.i32[1], _character.scale, static_cast<f32>(spw_type));
  _character.damage = SPAWN_DAMAGE_CURVE(_character.buffer.i32[1], _character.scale, static_cast<f32>(spw_type));
  _character.speed =  SPAWN_SPEED_CURVE(_character.buffer.i32[1], _character.scale, static_cast<f32>(spw_type));
  
  register_spawn_animation(__builtin_addressof(_character), SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT);

  // INFO: Setting collision equal to the any animation dimentions, as we consider each are equal.
  _character.collision.width = _character.move_left_animation.current_frame_rect.width * _character.scale;
  _character.collision.height = _character.move_left_animation.current_frame_rect.height * _character.scale;
  _character.collision.x = _character.position.x;
  _character.collision.y = _character.position.y;
  if(spw_type != SPAWN_TYPE_BOSS && CheckCollisionRecs(state->in_camera_metrics->frustum, _character.collision)) { return false; }

  register_spawn_animation(__builtin_addressof(_character), SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT);
  register_spawn_animation(__builtin_addressof(_character), SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT);
  register_spawn_animation(__builtin_addressof(_character), SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT);

  _character.w_direction = WORLD_DIRECTION_RIGHT;
  _character.character_id = state->next_spawn_id++;
  _character.initialized = true;

  for (size_t itr_000 = 0; itr_000 < state->spawns.size(); ++itr_000) {
    if(CheckCollisionRecs(state->spawns.at(itr_000).collision, _character.collision)) { return false; }
  }
  if(_character.damage_break_time > _character.take_damage_left_animation.fps * static_cast<f32>(TARGET_FPS)) {
    _character.damage_break_time = _character.take_damage_left_animation.fps  * static_cast<f32>(TARGET_FPS);
  }

  state->spawns.push_back(_character);
  return true;
}

bool update_spawns(Vector2 player_position) {
  if (!state) { 
    TraceLog(LOG_ERROR, "spawn::update_spawns()::State is not valid");
    return false; 
  }

  for (size_t itr_000 = 0; itr_000 < state->spawns.size(); ++itr_000) {
    Character2D *character = __builtin_addressof(state->spawns.at(itr_000));
    if (!character->initialized) { 
      remove_spawn(itr_000);
      continue; 
    }
    if (!character->is_damagable) {
      if (character->damage_break_time >= 0) {
        character->damage_break_time -= GetFrameTime();
      }
      else character->is_damagable = true;
    }
    if ((character->health <= 0 && character->is_damagable) || character->health > MAX_SPAWN_HEALTH) {
      remove_spawn(itr_000);
      continue;
    }

    if (character->is_dead) { continue; }

    Vector2 new_position = move_towards(character->position, player_position, character->speed * GetFrameTime());
    bool x0_collide = false;
    bool y0_collide = false;
    for (size_t itr_111 = 0; itr_111 < state->spawns.size(); ++itr_111) {
      if (state->spawns.at(itr_111).character_id == character->character_id) { continue; }

      const Rectangle spw_col = character->collision;
      const Rectangle x0 = Rectangle {spw_col.x, new_position.y, spw_col.width, spw_col.height};
      const Rectangle y0 = Rectangle {new_position.x, spw_col.y, spw_col.width, spw_col.height};

      if(CheckCollisionRecs(state->spawns.at(itr_111).collision, x0)) {
        x0_collide = true;
      }
      if(CheckCollisionRecs(state->spawns.at(itr_111).collision, y0)) {
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
    event_fire(EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE, event_context(
      static_cast<i16>(character->collision.x), static_cast<i16>(character->collision.y), static_cast<i16>(character->collision.width), static_cast<i16>(character->collision.height),
      static_cast<i16>(character->damage),
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));

    if (character->last_played_animation) {
      update_sprite(character->last_played_animation);
    }
  }
  return true;
}
bool render_spawns(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "spawn::render_spawns()::State is not valid");
    return false;
  }
  // Enemies
  for (size_t itr_000 = 0; itr_000 < state->spawns.size(); ++itr_000) {
    if (state->spawns.at(itr_000).initialized) {
      if(!state->spawns.at(itr_000).is_dead) 
      {
        if(state->spawns.at(itr_000).is_damagable)
        {
          switch (state->spawns.at(itr_000).w_direction) 
          {
            case WORLD_DIRECTION_LEFT: spawn_play_anim(&state->spawns.at(itr_000), SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT);
            break;
            case WORLD_DIRECTION_RIGHT:spawn_play_anim(&state->spawns.at(itr_000), SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT);
            break;
            default: {
              TraceLog(LOG_WARNING, "spawn::render_spawns()::Spawn has no directions");
              break;
            }
          }
        }	
        else 
        {
          (state->spawns.at(itr_000).w_direction == WORLD_DIRECTION_LEFT) 
            ? spawn_play_anim(&state->spawns.at(itr_000), SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT)
            : spawn_play_anim(&state->spawns.at(itr_000), SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT);
        }
      } 
      else 
      {
        (state->spawns.at(itr_000).w_direction == WORLD_DIRECTION_LEFT) 
          ? spawn_play_anim(&state->spawns.at(itr_000), SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT)
          : spawn_play_anim(&state->spawns.at(itr_000), SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT);
      }
    }
  }
  return true;
}

std::vector<Character2D>* get_spawns(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "spawn::get_spawns()::State was null");
    return 0;
  }

  return __builtin_addressof(state->spawns);
}

void clean_up_spawn_state(void) {
  state->spawns.clear();

  state->next_spawn_id = 0;
}

void spawn_play_anim(Character2D* spawn, spawn_movement_animations movement) {
  Rectangle dest = spawn->collision;

  switch (movement) {
    case SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT: {
      play_sprite_on_site(&spawn->move_left_animation, WHITE, dest);
      spawn->last_played_animation = &spawn->move_left_animation;
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT: {
      play_sprite_on_site(&spawn->move_right_animation, WHITE, dest);
      spawn->last_played_animation = &spawn->move_right_animation;
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT:  {
      play_sprite_on_site(&spawn->take_damage_left_animation, WHITE, dest);
      spawn->last_played_animation = &spawn->take_damage_left_animation;
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT:  {
      play_sprite_on_site(&spawn->take_damage_right_animation, WHITE, dest);
      spawn->last_played_animation = &spawn->take_damage_right_animation;
      break;
    }
    default: {
      TraceLog(LOG_ERROR, "spawn::spawn_play_anim()::Unsupported sheet id");
      break;
    }
  }
}
 
void remove_spawn(i32 index) {
  if (!state || state == nullptr) {
    TraceLog(LOG_WARNING, "spawn::remove_spawn()::State is not valid"); 
    return; 
  }
  if (static_cast<size_t>(index) >= state->spawns.size()) {
    TraceLog(LOG_WARNING, "spawn::remove_spawn()::Index is out of bounds"); 
    return; 
  }

  state->spawns.at(index) = state->spawns.back();
  state->spawns.pop_back();
}

void register_spawn_animation(Character2D* spawn, spawn_movement_animations movement) {
  if (!spawn) {
    TraceLog(LOG_WARNING, "spawn::register_spawn_animation()::Spawn is not valid");
    return;
  }

  switch (movement) {
    case SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT: {
      switch (spawn->buffer.i32[0]) {
        case SPAWN_TYPE_BROWN: {
          spawn->move_left_animation.sheet_id = SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(&spawn->move_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_ORANGE: {
          spawn->move_left_animation.sheet_id = SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(&spawn->move_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_YELLOW: {
          spawn->move_left_animation.sheet_id = SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(&spawn->move_left_animation, true, false);
          return;
        } 
        case SPAWN_TYPE_RED: {
          spawn->move_left_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(&spawn->move_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_BOSS: {
          spawn->move_left_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(&spawn->move_left_animation, true, false);
          return;
        }
        default: {
          TraceLog(LOG_WARNING, "spawn::register_spawn_animation()::Unsupported spawn type");
          return;
        }
      }
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT: {
      switch (spawn->buffer.i32[0]) {
        case SPAWN_TYPE_BROWN: {
          spawn->move_right_animation.sheet_id = SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_MOVE_RIGHT;
          set_sprite(&spawn->move_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_ORANGE: {
          spawn->move_right_animation.sheet_id = SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_MOVE_RIGHT;
          set_sprite(&spawn->move_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_YELLOW: {
          spawn->move_right_animation.sheet_id = SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_MOVE_RIGHT;
          set_sprite(&spawn->move_right_animation, true, false);
          return;
        } 
        case SPAWN_TYPE_RED: {
          spawn->move_right_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_RIGHT;
          set_sprite(&spawn->move_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_BOSS: {
          spawn->move_right_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_RIGHT;
          set_sprite(&spawn->move_right_animation, true, false);
          return;
        }
        default: {
          TraceLog(LOG_WARNING, "spawn::register_spawn_animation()::Unsupported spawn type");
          return;
        }
      }
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT:  {
      switch (spawn->buffer.i32[0]) {
        case SPAWN_TYPE_BROWN: {
          spawn->take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(&spawn->take_damage_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_ORANGE: {
          spawn->take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(&spawn->take_damage_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_YELLOW: {
          spawn->take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(&spawn->take_damage_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_RED: {
          spawn->take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(&spawn->take_damage_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_BOSS: {
          spawn->take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(&spawn->take_damage_left_animation, true, false);
          return;
        }
        default: {
          TraceLog(LOG_WARNING, "spawn::register_spawn_animation()::Unsupported spawn type");
          return;
        }
      }
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT:  {
      switch (spawn->buffer.i32[0]) {
        case SPAWN_TYPE_BROWN: {
          spawn->take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(&spawn->take_damage_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_ORANGE: {
          spawn->take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(&spawn->take_damage_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_YELLOW: {
          spawn->take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(&spawn->take_damage_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_RED: {
          spawn->take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(&spawn->take_damage_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_BOSS: {
          spawn->take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(&spawn->take_damage_right_animation, true, false);
          return;
        }
        default: {
          TraceLog(LOG_WARNING, "spawn::register_spawn_animation()::Unsupported spawn type");
          return;
        }
      }
      break;
    }
    default: {
      TraceLog(LOG_ERROR, "spawn::register_spawn_animation()::Unsupported sheet id");
      return;
    }
  }

  TraceLog(LOG_WARNING, "spawn::register_spawn_animation()::Function has ended unexpectedly");
}
