#include "spawn.h"

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"

#include "resource.h"

static spawn_system_state* spawn_system;

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL+1];

bool spawn_system_initialized = false;

bool spawn_system_initialize() {
    if (spawn_system_initialized) return false;

    spawn_system = (spawn_system_state*)allocate_memory_linear(sizeof(spawn_system_state), true);

    spawn_system->current_spawn_count = 0;

    spawn_system_initialized = true;
    return true;
}

void kill_spawn(u16 _id) {
    Character2D* character = {0};
    Character2D* spawns = spawn_system->spawns;
    u16 spawn_count = spawn_system->current_spawn_count;

    for (u32 i = 0; i < spawn_count; i++) {
        if (spawns[i].character_id == _id) {
            character = &spawns[i];
            break;
        }
    }

    if (character->initialized == false) return;

    if (character->character_id != spawn_count - 1) {
        spawns[_id] = spawns[spawn_count - 1];

        spawns[spawn_count - 1] = (Character2D){0};

        spawn_system->current_spawn_count--;

        spawns[_id].character_id = _id;
    } else {
        spawns[_id] = (Character2D){0};
        spawn_system->current_spawn_count--;
    }

    event_fire(EVENT_CODE_PLAYER_ADD_EXP, 0, (event_context) { .data.u32[0] = 32});
}

bool spawn_character(Character2D character, actor_type _type) {
    Texture2D tex = get_texture_by_enum(ENEMY_TEXTURE);
    character.character_id = spawn_system->current_spawn_count;
    character.type = _type;

    character.collision_rect = (Rectangle){
        .width = tex.width,
        .height = tex.height,
        .x = character.position.x,
        .y = character.position.y};

    character.initialized = true;
    spawn_system->spawns[spawn_system->current_spawn_count++] = character;

    return true;
}

bool update_spawns(Vector2 player_position) {
 
    for (i32 i = 0; i < spawn_system->current_spawn_count; i++) 
    {
        Vector2 new_position = move_towards(spawn_system->spawns[i].position, player_position, spawn_system->spawns[i].speed);

        spawn_system->spawns[i].position = new_position;
        spawn_system->spawns[i].collision_rect.x = new_position.x;
        spawn_system->spawns[i].collision_rect.y = new_position.y;
    }

    return true;
}

bool render_spawns() {
    // Enemies
    for (i32 i = 0; i < spawn_system->current_spawn_count; i++) {
        if (spawn_system->spawns[i].initialized) {

            Texture2D tex = get_texture_by_enum(ENEMY_TEXTURE);

            if(tex.id == INVALID_ID32) {
                TraceLog(LOG_ERROR, "INVALID TEXTURE!");
            }

            DrawTexture(
                tex,
                spawn_system->spawns[i].position.x,
                spawn_system->spawns[i].position.y,
                WHITE);

#if DEBUG_COLLISIONS
            DrawRectangleLines(
                spawn_system->spawns[i].collision_rect.x,
                spawn_system->spawns[i].collision_rect.y,
                spawn_system->spawns[i].collision_rect.width,
                spawn_system->spawns[i].collision_rect.height,
                WHITE);
#endif
        }
    }
    return true;
}

spawn_system_state* get_spawn_system() {
    return spawn_system;
}

void clean_up_spawn_system() {
    for (u16 i = 0; i < spawn_system->current_spawn_count; i++) {
        spawn_system->spawns[i] = (Character2D){0};
    }

    spawn_system->current_spawn_count = 0;

    //spawn_system = (spawn_system_state*){0};
}
