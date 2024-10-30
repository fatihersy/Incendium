#include "player.h"

#include "core/fmemory.h"

#include "ability.h"

bool player_system_initialized = false;

static player_state* player;

bool player_system_initialize() {
    if (player_system_initialized) return false;

    player = (player_state*)allocate_memory_linear(sizeof(player_state), true);

    if (!player) {
        TraceLog(LOG_FATAL, "PLAYER_SYSTEM ALLOCATION FAILED");
        return false;
    }

    player->position.x = 0;
    player->position.y = 0;

    player->texture_path = "D:\\Workspace\\CLang\\Resources\\wabbit_alpha.png";

    player->player_texture = LoadTexture(player->texture_path);

    player->ability_system = ability_system_initialize(PLAYER);

    player->collision = (Rectangle){
        .x = player->position.x - player->player_texture.width / 2,
        .y = player->position.y - player->player_texture.height / 2,
        .width = player->player_texture.width,
        .height = player->player_texture.height};

    add_ability(&player->ability_system, fireball);
    // add_ability(ability_system, salvo);
    // add_ability(ability_system, radiation);
    // add_ability(ability_system, direct_fire);

    player_system_initialized = true;

    return true;
}

player_state* get_player_state() {
    if (!player_system_initialized) {
        return (player_state*)0;
    }

    return player;
}

bool update_player() {
    if (!player_system_initialized) return false;

    if (IsKeyDown(KEY_W)) {
        player->position.y -= 2;
    }
    if (IsKeyDown(KEY_A)) {
        player->position.x -= 2;
    }
    if (IsKeyDown(KEY_S)) {
        player->position.y += 2;
    }
    if (IsKeyDown(KEY_D)) {
        player->position.x += 2;
    }

    update_abilities(&player->ability_system, player->position);

    player->collision.x = player->position.x - player->player_texture.width / 2;
    player->collision.y = player->position.y - player->player_texture.height / 2;
    player->collision.width = player->player_texture.width;
    player->collision.height = player->player_texture.height;

    return true;
}

bool render_player() {
    if (!player_system_initialized) {
        return false;
    }

    DrawTexture(
        player->player_texture,
        player->position.x - player->player_texture.width / 2,
        player->position.y - player->player_texture.height / 2,
        WHITE);

    render_abilities(&player->ability_system);

#if DEBUG_COLLISIONS
    DrawRectangleLines(
        player->collision.x,
        player->collision.y,
        player->collision.width,
        player->collision.height,
        WHITE);
#endif

    return true;
}
