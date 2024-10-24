#include "spawn.h"

#include "resource.h"
#include "collision.h"

Character2D* spawns = {0};
i16 current_spawn_count = 0;

bool spawn_system_initialized = false;

bool spawn_system_initialize() {
    if (spawn_system_initialized) return false;

    spawns = (Character2D*)malloc(MAX_SPAWN_COUNT * sizeof(Character2D));

    spawn_system_initialized = true;
    return true;
}

bool spawn_character(Character2D character, actor_type type) {
    Texture2D tex = get_texture_by_id(character.texId);
    character.ID = current_spawn_count;
    
    character.collision = (Rectangle){
        .width = tex.width,
        .height = tex.height,
        .x = character.position.x,
        .y = character.position.y};

    add_collision((Rectangle){
        .x = character.position.x,
        .y = character.position.y,
        .width = tex.width,
        .height = tex.height,
    },
    type);

    character.initialized = true;
    spawns[current_spawn_count++] = character;

    return true;
}

Character2D get_character(u32 ID) {
    for (i32 i = 0; i < MAX_SPAWN_COUNT; i++) {
        if (spawns[i].ID == ID) return spawns[i];
    }

    return (Character2D){.initialized = false};
}

bool update_spawns() {
    for (i32 i = 0; i < current_spawn_count; i++) {
    }

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
