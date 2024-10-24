#include "player.h"

#include "ability.h"
#include "collision.h"

bool player_system_initialized = false;

player_state* player = (player_state*){0};

ability_system_state* ability_system;

bool player_system_initialize() {
    if (player_system_initialized) return false;

    player = (player_state*)malloc(sizeof(player_state));

    player->position.x = 0;
    player->position.y = 0;

    player->texture_path = "D:\\Workspace\\CLang\\Resources\\wabbit_alpha.png";

    player->player_texture = LoadTexture(player->texture_path);

    ability_system = ability_system_initialize();

    add_ability(ability_system, fireball);

    player_system_initialized = true;

    return true;
}

player_state* get_player_state() {
    if (!player_system_initialized) {
        return (player_state*)0;
    }

    return player;
}

void set_player_position(i16 x, i16 y) {
    player->position.x = x;
    player->position.y = y;
}

bool update_player() {
    if (!player_system_initialized) return false;

    if (IsKeyDown(KEY_W))
    {
        set_player_position(get_player_state()->position.x, get_player_state()->position.y-1);
    }
    if (IsKeyDown(KEY_A))
    {
        set_player_position(get_player_state()->position.x-1, get_player_state()->position.y);
    }
    if (IsKeyDown(KEY_S))
    {
        set_player_position(get_player_state()->position.x, get_player_state()->position.y+1);
    }
    if (IsKeyDown(KEY_D))
    {
        set_player_position(get_player_state()->position.x+1, get_player_state()->position.y);
    }
    
    update_abilities(ability_system, player->position);

    return true;
}

bool render_player() {
    if (!player_system_initialized) {
        return false;
    }

    DrawTexture(
        player->player_texture,
        player->position.x - player->player_texture.width/2,
        player->position.y - player->player_texture.height/2,
        WHITE);

    render_abilities(ability_system, player->position);

    return true;
}
