#include "spawn.h"

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"
#include "core/logger.h"
#include "core/ftime.h"

#include "spritesheet.h"

#include "fshader.h"

typedef struct spawn_system_state {
  std::vector<Character2D> spawns; // NOTE: See also clean-up function
  const camera_metrics* in_camera_metrics;
  const ingame_info* in_ingame_info;
  i32 next_spawn_id;

  spawn_system_state(void) {
    this->spawns = std::vector<Character2D>();
    this->in_camera_metrics = nullptr;
    this->in_ingame_info = nullptr;
    this->next_spawn_id = 0;
  }
} spawn_system_state;

// To avoid dublicate symbol errors. Implementation in defines.h
extern const i32 level_curve[MAX_PLAYER_LEVEL + 1];
static spawn_system_state * state = nullptr;

#define DEATH_EFFECT_HEIGHT_SCALE 0.115f

#define CHECK_pSPAWN_COLLISION(REC1, SPAWN, NEW_POS) \
CheckCollisionRecs(REC1, \
  (Rectangle) { \
    NEW_POS.x, NEW_POS.y, \
    SPAWN->collision.width, SPAWN->collision.height\
})
#define SPAWN_RND_SCALE_MIN 1.5f
#define SPAWN_RND_SCALE_MAX 2.5f
#define SPAWN_SCALE_INCREASE_BY_LEVEL(LEVEL) LEVEL * .1

#define SPAWN_BASE_HEALTH 32
#define SPAWN_HEALTH_CURVE(LEVEL, SCALE, TYPE) (SPAWN_BASE_HEALTH + (SPAWN_BASE_HEALTH * ((SCALE * 0.4f) + (LEVEL * 0.6f) + (TYPE * 0.3f)) * level_curve[LEVEL] * 0.002f))

#define SPAWN_BASE_DAMAGE 16
#define SPAWN_DAMAGE_CURVE(LEVEL, SCALE, TYPE) (SPAWN_BASE_DAMAGE + (SPAWN_BASE_DAMAGE * ((SCALE * 0.45f) + (LEVEL * 0.55f) + (TYPE * 0.35f)) * level_curve[LEVEL] * 0.002f))

#define SPAWN_BASE_SPEED 50
#define SPAWN_SPEED_CURVE(LEVEL, SCALE, TYPE) (SPAWN_BASE_SPEED + ((SPAWN_BASE_SPEED * (LEVEL + (LEVEL * 0.2f)) + (TYPE + (TYPE * 0.25f))) / SCALE))

void spawn_play_anim(Character2D* spawn, spawn_movement_animations sheet);
void remove_spawn(i32 index);
void register_spawn_animation(Character2D* spawn, spawn_movement_animations movement);
void update_spawn_animation(Character2D* spawn);

bool spawn_system_initialize(const camera_metrics* _camera_metrics, const ingame_info* _ingame_info) {
  if (state and state != nullptr) {
    clean_up_spawn_state();
    return true;
  }
  state = (spawn_system_state *)allocate_memory_linear(sizeof(spawn_system_state), true);
  if (not state or state == nullptr) {
    IERROR("spawn::spawn_system_initialize()::Spawn system init failed");
    return false;
  }
  *state = spawn_system_state();
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;
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
  if (not character or character == nullptr) return INVALID_IDI32;
  if (not character->is_damagable) { return character->health_current; }

  if(character->health_current - damage > 0 && character->health_current - damage < MAX_SPAWN_HEALTH) {
    character->is_damagable = false;
    character->damage_break_time = character->take_damage_left_animation.fps / static_cast<f32>(TARGET_FPS);
    character->health_current -= damage;
    return character->health_current;
  }
  character->health_current = 0;
  character->is_dead = true;
  character->is_damagable = false;
  character->damage_break_time = character->take_damage_left_animation.fps / static_cast<f32>(TARGET_FPS);

  bool loot_rnd = get_random(0, 100) > 24;

  if (loot_rnd) {
    event_fire(EVENT_CODE_SPAWN_ITEM, event_context(
      static_cast<i16>(ITEM_TYPE_EXPERIENCE),
      static_cast<i16>(character->collision.x + character->collision.width  * .5f), // INFO: Position x
      static_cast<i16>(character->collision.y + character->collision.height * .5f), // INFO: Position y
      static_cast<i16>(character->collision.y + character->collision.height * .5f), // INFO: Loot drop animation position y begin
      static_cast<i16>(character->collision.height * .5f), // INFO: Loot drop animation position y change
      static_cast<i16>(
        level_curve[character->buffer.i32[1]] 
          * (static_cast<f32>(character->buffer.i32[0]) / static_cast<f32>(SPAWN_TYPE_MAX)) 
          * (character->scale / SPAWN_RND_SCALE_MAX)
      )
    ));
  } else {
    event_fire(EVENT_CODE_SPAWN_ITEM, event_context(
      static_cast<i16>(ITEM_TYPE_COIN),
      static_cast<i16>(character->collision.x + character->collision.width  * .5f), // INFO: Position x
      static_cast<i16>(character->collision.y + character->collision.height * .5f), // INFO: Position y
      static_cast<i16>(character->collision.y + character->collision.height * .5f), // INFO: Loot drop animation start
      static_cast<i16>(character->collision.height * .5f), // INFO: Loot drop animation end
      static_cast<i16>(
        level_curve[character->buffer.i32[1]] 
          * (static_cast<f32>(character->buffer.i32[0]) / static_cast<f32>(SPAWN_TYPE_MAX)) 
          * (character->scale / SPAWN_RND_SCALE_MAX)
      )
    ));
  }

  return 0;
}

i32 spawn_character(Character2D _character) {
  if (_character.buffer.i32[0] <= SPAWN_TYPE_UNDEFINED or _character.buffer.i32[0] >= SPAWN_TYPE_MAX) {
    return -1;
  }
  spawn_type spw_type = static_cast<spawn_type>(_character.buffer.i32[0]);
  _character.scale =  SPAWN_RND_SCALE_MIN + (SPAWN_RND_SCALE_MAX * _character.buffer.i32[2] / 100.f);
  _character.scale += SPAWN_SCALE_INCREASE_BY_LEVEL(_character.buffer.i32[1]);

  _character.health_max = SPAWN_HEALTH_CURVE(_character.buffer.i32[1], _character.scale, static_cast<f32>(spw_type));
  _character.damage = SPAWN_DAMAGE_CURVE(_character.buffer.i32[1], _character.scale, static_cast<f32>(spw_type));
  _character.speed =  SPAWN_SPEED_CURVE(_character.buffer.i32[1], _character.scale, static_cast<f32>(spw_type));
  
  _character.health_current = _character.health_max;

  register_spawn_animation(__builtin_addressof(_character), SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT);

  // INFO: Setting collision equal to the any animation dimentions, as we consider each are equal.
  _character.collision.width = _character.move_left_animation.current_frame_rect.width * _character.scale;
  _character.collision.height = _character.move_left_animation.current_frame_rect.height * _character.scale;
  _character.collision.x = _character.position.x;
  _character.collision.y = _character.position.y;
  if(CheckCollisionRecs(state->in_camera_metrics->frustum, _character.collision)) { return -1; }

  register_spawn_animation(__builtin_addressof(_character), SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT);
  register_spawn_animation(__builtin_addressof(_character), SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT);
  register_spawn_animation(__builtin_addressof(_character), SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT);
  
  _character.death_effect_animation.sheet_id = SHEET_ID_SPAWN_EXPLOSION;
  set_sprite(__builtin_addressof(_character.death_effect_animation), false, true);

  _character.w_direction = WORLD_DIRECTION_RIGHT;
  _character.last_played_animation = SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT;
  _character.character_id = state->next_spawn_id++;
  _character.initialized = true;

  for (size_t itr_000 = 0; itr_000 < state->spawns.size(); ++itr_000) {
    if(CheckCollisionRecs(state->spawns.at(itr_000).collision, _character.collision)) { return -1; }
  }
  if(_character.damage_break_time > _character.take_damage_left_animation.fps * static_cast<f32>(TARGET_FPS)) {
    _character.damage_break_time = _character.take_damage_left_animation.fps  * static_cast<f32>(TARGET_FPS);
  }

  state->spawns.push_back(_character);
  return _character.character_id;
}

bool update_spawns(Vector2 player_position) {
  if (not state or state == nullptr) { 
    IERROR("spawn::update_spawns()::State is not valid");
    return false; 
  }

  for (size_t itr_000 = 0; itr_000 < state->spawns.size(); ++itr_000) {
    Character2D * const character = __builtin_addressof(state->spawns.at(itr_000));
    if (not character->initialized) { 
      remove_spawn(itr_000);
      continue; 
    }
    if (not character->is_damagable) {
      if (character->damage_break_time >= 0) {
        character->damage_break_time -= GetFrameTime();
      }
      else {
        character->is_damagable = true;
        reset_sprite(__builtin_addressof(character->take_damage_left_animation), true);
        reset_sprite(__builtin_addressof(character->take_damage_right_animation), true);
      }
    }
    if (character->is_dead or (character->health_current <= 0) or character->health_current > MAX_SPAWN_HEALTH) {

      if (character->take_damage_left_animation.is_played or character->take_damage_right_animation.is_played) {
        if (character->death_effect_animation.is_played) {
          remove_spawn(itr_000);
          continue;
        }
        character->is_invisible = true;
      }
      if (not character->death_effect_animation.is_started) {
        remove_spawn(itr_000);
        continue;
      }
      
      update_sprite(__builtin_addressof(character->death_effect_animation));
      update_spawn_animation(character);
      
      continue;
    }
    if (character->is_dead) { continue; }

    character->is_on_screen = CheckCollisionRecs(character->collision, state->in_camera_metrics->frustum);

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
    if (not x0_collide) {
      character->position.y = new_position.y;
      character->collision.y = character->position.y;
    }
    if (not y0_collide) {
      if (character->position.x - new_position.x > 0) {
        character->w_direction = WORLD_DIRECTION_LEFT;
      } else {
        character->w_direction = WORLD_DIRECTION_RIGHT;
      }
      character->position.x = new_position.x;
      character->collision.x = character->position.x;
    }
    update_spawn_animation(character);

    // WARN: This event fires 'clean_up_spawn_state()' function if player die from damage
    event_fire(EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE, event_context(
      static_cast<i16>(character->collision.x), static_cast<i16>(character->collision.y), static_cast<i16>(character->collision.width), static_cast<i16>(character->collision.height),
      static_cast<i16>(character->damage),
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));
    // WARN: This event fires 'clean_up_spawn_state()' function if player die from damage
  }
  return true;
}
bool render_spawns(void) {
  if (not state or state == nullptr) {
    IERROR("spawn::render_spawns()::State is not valid");
    return false;
  }
  
  for (size_t itr_000 = 0; itr_000 < state->spawns.size(); ++itr_000) {
    if (state->spawns.at(itr_000).initialized) {
      Character2D * const _character = __builtin_addressof(state->spawns.at(itr_000));

      if(not _character->is_dead and not _character->is_invisible) 
      {
        BeginShaderMode(get_shader_by_enum(SHADER_ID_SPAWN)->handle);
        set_shader_uniform(SHADER_ID_SPAWN, "spawn_id", data128(static_cast<f32>(state->spawns.at(itr_000).character_id)));

        if(_character->is_damagable)
        {
          switch (_character->w_direction) 
          {
            case WORLD_DIRECTION_LEFT: spawn_play_anim(__builtin_addressof(state->spawns.at(itr_000)), SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT);
            break;
            case WORLD_DIRECTION_RIGHT:spawn_play_anim(__builtin_addressof(state->spawns.at(itr_000)), SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT);
            break;
            default: {
              IWARN("spawn::render_spawns()::Unsupported direction");
              break;
            }
          }
        }	
        else 
        {
          (_character->w_direction == WORLD_DIRECTION_LEFT) 
            ? spawn_play_anim(__builtin_addressof(state->spawns.at(itr_000)), SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT)
            : spawn_play_anim(__builtin_addressof(state->spawns.at(itr_000)), SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT);
        }
        
        EndShaderMode();
      } 
      else 
      {
        if (not _character->is_invisible) {
          BeginShaderMode(get_shader_by_enum(SHADER_ID_SPAWN)->handle);
          set_shader_uniform(SHADER_ID_SPAWN, "spawn_id", data128(static_cast<f32>(state->spawns.at(itr_000).character_id)));

          if (_character->w_direction == WORLD_DIRECTION_LEFT) {
            spawn_play_anim(__builtin_addressof(state->spawns.at(itr_000)), SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT);
          }
          else {
            spawn_play_anim(__builtin_addressof(state->spawns.at(itr_000)), SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT);
          }
          EndShaderMode();
        }

        const f32 death_effect_height = state->in_camera_metrics->frustum.height * DEATH_EFFECT_HEIGHT_SCALE;
        const f32 death_effect_wh_ratio = _character->death_effect_animation.current_frame_rect.width / _character->death_effect_animation.current_frame_rect.height;
        const f32 death_effect_width = death_effect_height * death_effect_wh_ratio;

        play_sprite_on_site(__builtin_addressof(_character->death_effect_animation), WHITE, Rectangle {
          _character->position.x + (_character->collision.width  * .5f) - (death_effect_width  * .5f), 
          _character->position.y + (_character->collision.height * .5f) - (death_effect_height * .5f),
          death_effect_height, 
          death_effect_width
        });
      }
    }
  }

  return true;
}

std::vector<Character2D>* get_spawns(void) {
  if (not state or state == nullptr) {
    IERROR("spawn::get_spawns()::State was null");
    return nullptr;
  }

  return __builtin_addressof(state->spawns);
}
const Character2D * get_spawn_by_id(i32 _id) {
  if (not state or state == nullptr) {
    IERROR("spawn::get_spawn_by_id()::State is invalid");
    return nullptr;
  }
  std::vector<Character2D>& spawns = state->spawns;

  for (size_t itr_000 = 0; itr_000 < spawns.size() ; ++itr_000) {
    if(spawns.at(itr_000).character_id == _id) {
      return __builtin_addressof(spawns.at(itr_000));
    }
  }

  return nullptr;
}

void clean_up_spawn_state(void) {
  for (Character2D& spw : state->spawns) {
    spw = Character2D();
  }
  state->spawns.clear();
  state->spawns.shrink_to_fit();

  state->next_spawn_id = 0;
}

void spawn_play_anim(Character2D* spawn, spawn_movement_animations movement) {
  if (not spawn or spawn == nullptr) {
    IWARN("spawn::spawn_play_anim()::Spawn is invalid");
    return;
  }
  Rectangle dest = spawn->collision;

  switch (movement) {
    case SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT: {
      play_sprite_on_site(&spawn->move_left_animation, WHITE, dest);
      spawn->last_played_animation = SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT;
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT: {
      play_sprite_on_site(&spawn->move_right_animation, WHITE, dest);
      spawn->last_played_animation = SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT;
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT:  {
      play_sprite_on_site(&spawn->take_damage_left_animation, WHITE, dest);
      spawn->last_played_animation = SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT:  {
      play_sprite_on_site(&spawn->take_damage_right_animation, WHITE, dest);
      spawn->last_played_animation = SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
      break;
    }
    default: {
      IWARN("spawn::spawn_play_anim()::Unsupported sheet id");
      break;
    }
  }
}
void update_spawn_animation(Character2D* spawn) {
  if (not spawn or spawn == nullptr) {
    IWARN("spawn::update_spawn_animation()::Spawn is invalid");
    return;
  }
  switch (spawn->last_played_animation) {
    case SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT: {
      update_sprite(__builtin_addressof(spawn->move_left_animation));
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT: {
      update_sprite(__builtin_addressof(spawn->move_right_animation));
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT:  {
      update_sprite(__builtin_addressof(spawn->take_damage_left_animation));
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT:  {
      update_sprite(__builtin_addressof(spawn->take_damage_right_animation));
      break;
    }
    default: {
      IWARN("spawn::update_spawn_animation()::Unsupported sheet id");
      break;
    }
  }
}
 
void remove_spawn(i32 index) {
  if (not state or state == nullptr) {
    IERROR("spawn::remove_spawn()::State is invalid"); 
    return; 
  }
  if (static_cast<size_t>(index) >= state->spawns.size() or index < 0) {
    IWARN("spawn::remove_spawn()::Index is out of bounds"); 
    return; 
  }
  state->spawns.erase(state->spawns.begin() + index);
  state->spawns.shrink_to_fit();
}

void register_spawn_animation(Character2D* spawn, spawn_movement_animations movement) {
  if (not spawn or spawn == nullptr) {
    IWARN("spawn::register_spawn_animation()::Spawn is invalid");
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
          IWARN("spawn::register_spawn_animation()::Unsupported spawn type");
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
          IWARN("spawn::register_spawn_animation()::Unsupported spawn type");
          return;
        }
      }
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT:  {
      switch (spawn->buffer.i32[0]) {
        case SPAWN_TYPE_BROWN: {
          spawn->take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(&spawn->take_damage_left_animation, false, true);
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
          IWARN("spawn::register_spawn_animation()::Unsupported spawn type");
          return;
        }
      }
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT:  {
      switch (spawn->buffer.i32[0]) {
        case SPAWN_TYPE_BROWN: {
          spawn->take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(&spawn->take_damage_right_animation, false, true);
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
          IWARN("spawn::register_spawn_animation()::Unsupported spawn type");
          return;
        }
      }
      break;
    }
    default: {
      IWARN("spawn::register_spawn_animation()::Unsupported sheet id");
      return;
    }
  }

  IERROR("spawn::register_spawn_animation()::Function has ended unexpectedly");
}
