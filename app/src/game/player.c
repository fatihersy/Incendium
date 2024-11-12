#include "player.h"

#include "core/event.h"
#include "core/fmemory.h"

#include "defines.h"
#include "game/ability.h"
#include "game/resource.h"
#include "game_manager.h"
#include <stdbool.h>

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL+1];

bool player_system_initialized = false;

static player_state* player;

bool player_system_on_event(u16 code, void* sender, void* listener_inst, event_context context);

bool player_system_initialize() {
    if (player_system_initialized) return false;

    player = (player_state*)allocate_memory_linear(sizeof(player_state), true);

    if (!player) {
        TraceLog(LOG_FATAL, "PLAYER_SYSTEM ALLOCATION FAILED");
        return false;
    }

    event_register(EVENT_CODE_PLAYER_ADD_EXP, 0, player_system_on_event);

    player->position.x = 0;
    player->position.y = 0;

    player->player_texture = get_texture_by_enum(PLAYER_TEXTURE);

    player->ability_system = ability_system_initialize(PLAYER);

    player->collision = (Rectangle){
        .x = player->position.x - player->player_texture.width / 2.f,
        .y = player->position.y - player->player_texture.height / 2.f,
        .width = player->player_texture.width,
        .height = player->player_texture.height};

    add_ability(&player->ability_system, fireball);
    // add_ability(ability_system, salvo);
    // add_ability(ability_system, radiation);
    // add_ability(ability_system, direct_fire);

    player->health_current = 100;
    player->health_max = 100;
    player->level = 1;
    player->exp_current = 0;
    player->exp_to_next_level = level_curve[player->level];

    player_system_initialized = true;

    return true;
}

player_state* get_player_state() {
    if (!player_system_initialized) {
        return (player_state*)0;
    }

    return player;
}

void add_exp_to_player(u32 exp) {
    u32 curr = player->exp_current;
    u32 to_next = player->exp_to_next_level;

    if ( curr + exp >= to_next) 
    {
        player->exp_current = (curr + exp) - to_next;
        player->level++;
        player->exp_to_next_level = level_curve[player->level];
        player->player_have_skill_points = true;
        play_sprite(LEVEL_UP_SHEET, get_player_position());
    }
    else {
        player->exp_current += exp;
    }
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

    player->collision.x = player->position.x - player->player_texture.width / 2.f;
    player->collision.y = player->position.y - player->player_texture.height / 2.f;
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
        player->position.x - player->player_texture.width / 2.f,
        player->position.y - player->player_texture.height / 2.f,
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

bool player_system_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_PLAYER_ADD_EXP:
        add_exp_to_player(context.data.u32[0]);

        default: return false;
    }

    return false;
}
