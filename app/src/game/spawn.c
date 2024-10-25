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
    character.character_id = current_spawn_count;
    
    character.collision_rect = (Rectangle){
        .width = tex.width,
        .height = tex.height,
        .x = character.position.x,
        .y = character.position.y};

    character.initialized = true;
    spawns[current_spawn_count++] = character;

    return true;
}

Character2D* get_spawn_data() {
    return spawns;
}

bool update_spawns() {
    for (i32 i = 0; i < current_spawn_count; i++) 
    {
        spawns[i].collision_rect.x = spawns[i].position.x;
        spawns[i].collision_rect.y = spawns[i].position.y;
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

        #if DEBUG_COLLISIONS

        DrawRectangleLines
        (
            spawns[i].collision_rect.x,
            spawns[i].collision_rect.y,
            spawns[i].collision_rect.width,
            spawns[i].collision_rect.height,
            WHITE
        );

        #endif
    }

    return true;
}
