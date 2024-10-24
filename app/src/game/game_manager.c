#include "game_manager.h"

#include "player.h"

bool game_manager_initialized = false;

game_data* game;

bool game_manager_initialize() {
    if (!game_manager_initialized) return false;

    game = (game_data*)malloc(sizeof(game_data));

    player_system_initialize();

    game_manager_initialized = true;

    return true;
}

Vector2 get_player_position() {
    if (!game_manager_initialized) return (Vector2) {-1};

    return get_player_state()->position;
}

bool update() {
    if (!game_manager_initialized) return false;
    
    
    return true;
}
