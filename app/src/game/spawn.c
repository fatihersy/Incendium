#include "spawn.h"

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"

typedef struct spawn_system_state {
  Character2D spawns[MAX_SPAWN_COUNT];
  u16 current_spawn_count;
} spawn_system_state;

static spawn_system_state *spawn_system;

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL + 1];

bool spawn_system_initialized = false;

bool spawn_system_initialize(void) {
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
  const u16 spawn_count = spawn_system->current_spawn_count;

  for (int i = 0; i < spawn_count ; ++i) {
    if(spawns[i].character_id == _id) {
        character = &spawns[i];
        break;
    }
  }
  if (!character) return INVALID_ID16;

  if(character->health - damage > 0 && character->health - damage < 100) {
    character->health -= damage;
    return character->health;
  }
  character->health = 0;

  event_fire(EVENT_CODE_PLAYER_ADD_EXP, (event_context){.data.u32[0] = 32}); // TODO: Make exp gain dynamic 
  return 0;
}

u16 spawn_character(Character2D _character) {
  if (spawn_system->current_spawn_count >= MAX_SPAWN_COUNT) {
    TraceLog(LOG_WARNING, "spawn_character()::Spawn count is out of bounds");
    return INVALID_ID16;
  }

  _character.character_id = spawn_system->current_spawn_count;
  _character.initialized = true;
  
  spawn_system->spawns[spawn_system->current_spawn_count] = _character;
  spawn_system->current_spawn_count++;
  return _character.character_id;
}

bool update_spawns(Vector2 player_position) {

  for (i32 i = 0; i < spawn_system->current_spawn_count; ++i) {
    Character2D *character = &spawn_system->spawns[i];
    if (character->health <= 0 || character->health > 100) {
      *character = spawn_system->spawns[spawn_system->current_spawn_count-1];
      spawn_system->spawns[spawn_system->current_spawn_count-1] = (Character2D){0};
      if (spawn_system->current_spawn_count > 0) {
        spawn_system->current_spawn_count--;
      }
      event_fire(EVENT_CODE_DELETE_SPAWN_COLLISION, (event_context) {
          .data.u16[0] = character->character_id});
      continue;
    }

    Vector2 new_position =
        move_towards(character->position, player_position,
                     character->speed);

    character->position = new_position;
    character->collision.x = new_position.x;
    character->collision.y = new_position.y;
    event_fire(EVENT_CODE_RELOCATE_SPAWN_COLLISION, (event_context) 
    {
        .data.u16[0] = character->character_id,
        .data.u16[1] = character->position.x,
        .data.u16[2] = character->position.y,
    });
  }

  return true;
}

bool render_spawns(void) {

  if (spawn_system->current_spawn_count > MAX_SPAWN_COUNT) {
    TraceLog(LOG_WARNING, "render_spawns()::Spawn count is out of bounds");
    return false;
  }

  // Enemies
  for (i32 i = 0; i < spawn_system->current_spawn_count; ++i) {
    if (spawn_system->spawns[i].initialized) {
      DrawTexture(
        *spawn_system->spawns[i].tex,
        spawn_system->spawns[i].position.x,
        spawn_system->spawns[i].position.y, WHITE
      );
      DrawRectangleLines(
        spawn_system->spawns[i].collision.x,
        spawn_system->spawns[i].collision.y,
        spawn_system->spawns[i].collision.width,
        spawn_system->spawns[i].collision.height, 
        WHITE
      );
    }
  }
  return true;
}

Character2D *get_spawn(u16 _id) {
  for (i32 i = 0; i < spawn_system->current_spawn_count; ++i) {
    if (spawn_system->spawns[i].character_id == _id) {
      return &spawn_system->spawns[i];
    }
  }
  return (Character2D *){0};
}

void clean_up_spawn_system(void) {
  for (u16 i = 0; i < spawn_system->current_spawn_count; i++) {
    spawn_system->spawns[i] = (Character2D){0};
  }

  spawn_system->current_spawn_count = 0;

  // spawn_system = (spawn_system_state*){0};
}
 
