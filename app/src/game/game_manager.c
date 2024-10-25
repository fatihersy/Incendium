#include "game_manager.h"

#include "player.h"
#include "spawn.h"

bool game_manager_initialized = false;

game_data* game;

bool game_manager_initialize() {
    if (!game_manager_initialized) return false;

    game = (game_data*)malloc(sizeof(game_data));

    player_system_initialize();

    game_manager_initialized = true;

    return true;
}

void set_player_position(i16 x, i16 y) {
    get_player_state()->position.x = x;
    get_player_state()->position.y = y;
}

Character2D* get_actor_by_id(u32 ID) {

    Character2D* spawn_data = get_spawn_data();

    for (i32 i = 0; i < MAX_SPAWN_COUNT; i++) {
        if (spawn_data[i].character_id == ID) return &spawn_data[i];
    }

    return (Character2D*){0};
}