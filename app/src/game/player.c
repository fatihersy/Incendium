#include "player.h"

#include "core/event.h"
#include "core/fmemory.h"

#include "game/ability.h"
#include "game/resource.h"
#include <stdbool.h>

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL+1];

bool player_system_initialized = false;

static player_state* player;

bool player_system_on_event(u16 code, void* sender, void* listener_inst, event_context context);

void move(spritesheet_type player_anim_sheet);

bool player_system_initialize() {
    if (player_system_initialized) return false;

    player = (player_state*)allocate_memory_linear(sizeof(player_state), true);

    if (!player) {
        TraceLog(LOG_FATAL, "PLAYER_SYSTEM ALLOCATION FAILED");
        return false;
    }

    event_register(EVENT_CODE_PLAYER_ADD_EXP, 0, player_system_on_event);
    event_register(EVENT_CODE_PLAYER_SET_POSITION, 0, player_system_on_event);

    player->position.x = 0;
    player->position.y = 0;
    player->dimentions = (Vector2) {86, 86};

    player->ability_system = ability_system_initialize(PLAYER, player->dimentions);

    player->collision = (Rectangle)
    {
        .x = player->position.x - player->dimentions.x / 2.f,
        .y = player->position.y - player->dimentions.y / 2.f,
        .width = player->dimentions.x,
        .height = player->dimentions.y
    };

    add_ability(&player->ability_system, FIREBALL);
    // add_ability(ability_system, salvo);
    // add_ability(ability_system, radiation);
    // add_ability(ability_system, direct_fire);

    player->health_current = 100;
    player->health_max = 100;
    player->level = 1;
    player->exp_current = 0;
    player->exp_to_next_level = level_curve[player->level];
    player->is_moving = false;
    player->w_direction = LEFT;
    player_system_initialized = true;
    player->initialized = true;
    player->move_left_sprite_queue_index = register_sprite(PLAYER_ANIMATION_MOVELEFT, SCENE_IN_GAME, false, true);
    player->move_right_sprite_queue_index = register_sprite(PLAYER_ANIMATION_MOVERIGHT, SCENE_IN_GAME, false, true);
    player->idle_left_sprite_queue_index = register_sprite(PLAYER_ANIMATION_IDLELEFT, SCENE_IN_GAME, false, true);
    player->idle_right_sprite_queue_index = register_sprite(PLAYER_ANIMATION_IDLERIGHT, SCENE_IN_GAME, false, true);
    
    return true;
}

player_state* get_player_state() {
    if (!player_system_initialized) {
        return (player_state*)0;
    }
    //TraceLog(LOG_INFO, "player.position {x:%d, y:%d}", player->position.x, player->position.y);
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
        //play_sprite_on_player(player->);
    }
    else {
        player->exp_current += exp;
    }
}

bool update_player() {
    if (!player_system_initialized) return false;

    if (IsKeyDown(KEY_W)) {
        player->position.y -= 2;
        player->is_moving = true;
    }
    if (IsKeyDown(KEY_A)) {
        player->position.x -= 2;
        player->w_direction = LEFT;
        player->is_moving = true;
    }
    if (IsKeyDown(KEY_S)) {
        player->position.y += 2;
        player->is_moving = true;
    }
    if (IsKeyDown(KEY_D)) {
        player->position.x += 2;
        player->w_direction = RIGHT;
        player->is_moving = true;
    }
    if (IsKeyUp(KEY_W) && IsKeyUp(KEY_A) && IsKeyUp(KEY_S) && IsKeyUp(KEY_D)) {    
        player->is_moving = false;
    }

    update_abilities(&player->ability_system, player->position);

    player->collision.x = player->position.x - player->dimentions.x / 2.f;
    player->collision.y = player->position.y - player->dimentions.y / 2.f;
    player->collision.width = player->dimentions.x;
    player->collision.height = player->dimentions.y;
    return true;
}

bool render_player() {
    if (!player_system_initialized) {
        return false;
    }

    if(player->is_moving) switch (player->w_direction) 
    {
        case LEFT: move(PLAYER_ANIMATION_MOVELEFT);
        break;
        case RIGHT: move(PLAYER_ANIMATION_MOVERIGHT);
        break;
    }
    else switch (player->w_direction) 
    {
        case LEFT:move(PLAYER_ANIMATION_IDLELEFT);
        break;
        case RIGHT:move(PLAYER_ANIMATION_IDLERIGHT);
        break;
    }

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
        case EVENT_CODE_PLAYER_ADD_EXP: {
            add_exp_to_player(context.data.u32[0]);
            break;
        }
        case EVENT_CODE_PLAYER_SET_POSITION: {
            player->position.x = context.data.f32[0]; 
            player->position.y = context.data.f32[1]; 
            player->collision.x = player->position.x; 
            player->collision.y = player->position.y; 
        }

        default: return false;
    }

    return false;
}

void move(spritesheet_type player_anim_sheet) {

    stop_sprite(player->last_played_sprite_id, false);
    switch (player_anim_sheet) {
        case SPRITESHEET_UNSPECIFIED: TraceLog(LOG_ERROR, "ERROR::player::move()::move function called with unspecified value.");
        break;
        case BUTTON_REFLECTION_SHEET: TraceLog(LOG_ERROR, "ERROR::player::move()::move function called with wrong value.");
        break;
        case BUTTON_CRT_SHEET: TraceLog(LOG_ERROR, "ERROR::player::move()::move function called with wrong value.");
        break;
        case LEVEL_UP_SHEET: TraceLog(LOG_ERROR, "ERROR::player::move()::move function called with wrong value.");
        break;

        case PLAYER_ANIMATION_MOVELEFT: {
            play_sprite_on_player(player->move_left_sprite_queue_index);
            player->last_played_sprite_id = player->move_left_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_MOVERIGHT: {
            play_sprite_on_player(player->move_right_sprite_queue_index);
            player->last_played_sprite_id = player->move_right_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_IDLELEFT:  {
            play_sprite_on_player(player->idle_left_sprite_queue_index);
            player->last_played_sprite_id = player->idle_left_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_IDLERIGHT:  {
            play_sprite_on_player(player->idle_left_sprite_queue_index);
            player->last_played_sprite_id = player->idle_left_sprite_queue_index;
            break;
        }
    }
}
