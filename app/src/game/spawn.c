#include "spawn.h"

#include "resource.h"

Character2D* spawns = {0};
i16 current_spawn_count = 0;

bool spawn_system_initialized = false;

bool spawn_system_initialize() {
    if (spawn_system_initialized) return false;

    spawns = (Character2D*)malloc(MAX_SPAWN_COUNT * sizeof(Character2D));

    spawn_system_initialized = true;
    return true;
}

bool spawn_character(Character2D character) {

    spawns[current_spawn_count] = character;
    character.initialized = true;

    current_spawn_count++;

    return true;
}

Character2D get_character(u32 ID) {
    for (i32 i = 0; i < MAX_SPAWN_COUNT; i++) {
        if (spawns[i].ID == ID) return spawns[i];
    }

    return (Character2D){.initialized = false};
}

bool update_spawns() {

    

    return true;
}

bool render_spawns() {

    // Enemies
    for (i32 i = 0; i < current_spawn_count; i++) {
        DrawTexture(
            get_texture_by_id(spawns[i].texId),
            spawns[i].position.x,
            spawns[i].position.y,
            WHITE);
    }

    return true;
}
