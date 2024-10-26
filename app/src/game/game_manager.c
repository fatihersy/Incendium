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

Vector2 get_player_position() {

    return get_player_state()->position;
}

Character2D* get_actor_by_id(u16 ID) {

    spawn_system_state* spawn_data = get_spawn_system();

    for (i32 i = 0; i < MAX_SPAWN_COUNT; i++) {
        if (spawn_data->spawns[i].character_id == ID) return &spawn_data->spawns[i];
    }

    return (Character2D*){0};
}

void damage_any_collade(Character2D* _character) 
{
    spawn_system_state* spawn_system = get_spawn_system();

    for (size_t i = 0; i < spawn_system->current_spawn_count; i++)
    {
        if(CheckCollisionRecs(spawn_system->spawns[i].collision_rect, _character->collision_rect)) 
        {
            kill_spawn(spawn_system->spawns[i].character_id);
        }
    }
}