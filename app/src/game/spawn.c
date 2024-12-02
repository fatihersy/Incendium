#include "spawn.h"

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"

#include "game/game_manager.h"


static spawn_system_state *spawn_system;

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL + 1];

bool spawn_system_initialized = false;

bool spawn_system_initialize() {
  if (spawn_system_initialized)
    return false;

  spawn_system = (spawn_system_state *)allocate_memory_linear(sizeof(spawn_system_state), true);

  spawn_system->current_spawn_count = 0;

  spawn_system_initialized = true;
  return true;
}

u16 damage_spawn(u16 _id, u16 damage) {
  Character2D *character = (Character2D*){0};
  Character2D *spawns = spawn_system->spawns;
  u16 spawn_count = spawn_system->current_spawn_count;
  u16 index = 0;

  for (int i = 1; i <= spawn_count ; ++i) {
    if(spawns[i].character_id == _id) {
        character = &spawns[i];
        index = i;
        break;
    }
  }
  if (!character) return INVALID_ID16;

  if(character->health - damage > 0) 
  {
    character->health -= damage;
    return character->health;
  }
  character->health = 0;

  if (index != spawn_count) {
    *character = spawns[spawn_count];

    spawns[spawn_count] = (Character2D){0};

    spawn_system->current_spawn_count--;
  } else {
    *character = (Character2D){0};
    spawn_system->current_spawn_count--;
  }
  event_fire(EVENT_CODE_PLAYER_ADD_EXP, 0, (event_context){.data.u32[0] = 32});
  return 0;
}

u16 spawn_character(Character2D _character) {
  spawn_system->current_spawn_count++;
  _character.character_id = spawn_system->current_spawn_count;
  _character.initialized = true;

  spawn_system->spawns[spawn_system->current_spawn_count] = _character;
  return _character.character_id;
}

bool update_spawns(Vector2 player_position) {

  for (i32 i = 1; i <= spawn_system->current_spawn_count; ++i) {
    Vector2 new_position =
        move_towards(spawn_system->spawns[i].position, player_position,
                     spawn_system->spawns[i].speed);

    spawn_system->spawns[i].position = new_position;
    spawn_system->spawns[i].collision.x = new_position.x;
    spawn_system->spawns[i].collision.y = new_position.y;
    event_fire(EVENT_CODE_RELOCATE_SPAWN_COLLISION, 0, (event_context) 
    {
        .data.u16[0] = spawn_system->spawns[i].character_id,
        .data.u16[1] = spawn_system->spawns[i].position.x,
        .data.u16[2] = spawn_system->spawns[i].position.y,
    });
    damage_any_collider_by_type(&spawn_system->spawns[i], PLAYER);
  }

  return true;
}

bool render_spawns() {
  // Enemies
  for (i32 i = 1; i <= spawn_system->current_spawn_count; ++i) {
    if (spawn_system->spawns[i].initialized) {
      DrawTexture(*spawn_system->spawns[i].tex,
                  spawn_system->spawns[i].position.x,
                  spawn_system->spawns[i].position.y, WHITE);

#if DEBUG_COLLISIONS
      DrawRectangleLines(spawn_system->spawns[i].collision.x,
                         spawn_system->spawns[i].collision.y,
                         spawn_system->spawns[i].collision.width,
                         spawn_system->spawns[i].collision.height, WHITE);
#endif
    }
  }
  return true;
}

spawn_system_state *get_spawn_system() { return spawn_system; }

void clean_up_spawn_system() {
  for (u16 i = 0; i < spawn_system->current_spawn_count; i++) {
    spawn_system->spawns[i] = (Character2D){0};
  }

  spawn_system->current_spawn_count = 0;

  // spawn_system = (spawn_system_state*){0};
}
