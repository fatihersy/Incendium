#include "player.h"
#include <defines.h>

#include "core/event.h"
#include "core/fmemory.h"

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL+1];
static player_state* player;

#define PSPRITESHEET_SYSTEM player->spritesheet_system // Don't forget undef at the bottom of the file
#include "game/spritesheet.h"

void play_anim(spritesheet_type player_anim_sheet);
void add_exp_to_player(u32 exp);
void take_damage(u16 damage);

bool player_system_on_event(u16 code, void* sender, void* listener_inst, event_context context);

bool player_system_initialize() {
    if (player) return false;

    player = (player_state*)allocate_memory_linear(sizeof(player_state), true);

    if (!player) {
        TraceLog(LOG_FATAL, "PLAYER_SYSTEM ALLOCATION FAILED");
        return false;
    }

    event_register(EVENT_CODE_PLAYER_ADD_EXP, 0, player_system_on_event);
    event_register(EVENT_CODE_PLAYER_SET_POSITION, 0, player_system_on_event);
    event_register(EVENT_CODE_PLAYER_TAKE_DAMAGE, 0, player_system_on_event);

    player->position.x = 0;
    player->position.y = 0;
    player->dimentions = (Vector2) {86, 86}; // TODO: Hardcoded dimentions
    player->dimentions_div2 = (Vector2) {player->dimentions.x/2, player->dimentions.y/2};

    //player->ability_system = ability_manager_initialize(PLAYER, player->dimentions);

    player->collision = (Rectangle)
    {
        .x = player->position.x - player->dimentions.x / 2.f,
        .y = player->position.y - player->dimentions.y / 2.f,
        .width = player->dimentions.x,
        .height = player->dimentions.y
    };

    //add_ability(&player->ability_system, FIREBALL);
    // add_ability(ability_system, salvo);
    // add_ability(ability_system, radiation);
    // add_ability(ability_system, direct_fire);

    player->is_dead = false;
    player->is_moving = false;
    player->w_direction = WORLD_DIRECTION_LEFT;
    player->is_damagable = true;
    player->damage_break_time = .2; //ms
    player->damage_break_current = player->damage_break_time;

    player->level = 1;
    player->exp_to_next_level = level_curve[player->level];
    player->exp_current = 0;
    player->health_max = 255;
    player->health_current = player->health_max;
    player->health_perc = (float) player->health_current / player->health_max;

    player->move_left_sprite_queue_index = register_sprite(PLAYER_ANIMATION_MOVE_LEFT, true, false, true);
    player->move_right_sprite_queue_index = register_sprite(PLAYER_ANIMATION_MOVE_RIGHT, true, false, true);
    player->idle_left_sprite_queue_index = register_sprite(PLAYER_ANIMATION_IDLE_LEFT, true, false, true);
    player->idle_right_sprite_queue_index = register_sprite(PLAYER_ANIMATION_IDLE_RIGHT, true, false, true);
    player->take_damage_left_sprite_queue_index = register_sprite(PLAYER_ANIMATION_TAKE_DAMAGE_LEFT, true, false, true);
    player->take_damage_right_sprite_queue_index = register_sprite(PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT, true, false, true);
    player->wreck_left_sprite_queue_index = register_sprite(PLAYER_ANIMATION_WRECK_LEFT, true, false, true);
    player->wreck_right_sprite_queue_index = register_sprite(PLAYER_ANIMATION_WRECK_RIGHT, true, false, true);
    player->last_played_sprite_id = player->idle_left_sprite_queue_index; // The position player starts. To avoid from the error when move firstly called
    
    player->starter_ability = ABILITY_TYPE_COMET;
    player->is_initialized = true;
    return true;
}

player_state* get_player_state() {
    if (!player) {
        return (player_state*)0;
    }
    //TraceLog(LOG_INFO, "player->position:{%f, %f}", player->position.x, player->position.y);
    return player;
}

Vector2 get_player_position(bool centered) {
    Vector2 pos = {0};
    if(centered) {
        pos = (Vector2) {
            .x = player->position.x - player->dimentions_div2.x,
            .y = player->position.y - player->dimentions_div2.y
        };
    }
    else { 
        pos = player->position;
    }

    //TraceLog(LOG_INFO, "(Vector2){%f, %f}", pos.x, pos.y);
    return pos;
}

void add_exp_to_player(u32 exp) {
    u32 curr = player->exp_current;
    u32 to_next = player->exp_to_next_level;

    if ( curr + exp >= to_next) 
    {
        player->exp_current = (curr + exp) - to_next;
        player->level++;
        player->exp_to_next_level = level_curve[player->level];
        player->is_player_have_skill_points = true;
        //play_sprite_on_player(player->);
    }
    else {
        player->exp_current += exp;
    }

    player->exp_perc = (float) player->exp_current / player->exp_to_next_level;
}
void take_damage(u16 damage) {
    if(!player->is_damagable) return;
    if(player->health_current - damage > 0) {
        player->health_current -= damage;
        player->is_damagable = false;
    }
    else {
        player->health_current = 0;
        player->is_dead = true;
    }

    player->health_perc = (float) player->health_current / player->health_max;
}

bool update_player() {
    if (!player) return false;

    if (player->is_dead) {
        event_fire(EVENT_CODE_PAUSE_GAME, 0, (event_context){0});
    }

    if(!player->is_damagable) {
        if(player->damage_break_current - GetFrameTime() > 0) player->damage_break_current -= GetFrameTime();
        else
        {
            player->damage_break_current = player->damage_break_time;
            player->is_damagable = true;
        }
    }

    if (IsKeyDown(KEY_W)) {
        player->position.y -= 2;
        player->is_moving = true;
    }
    if (IsKeyDown(KEY_A)) {
        player->position.x -= 2;
        player->w_direction = WORLD_DIRECTION_LEFT;
        player->is_moving = true;
    }
    if (IsKeyDown(KEY_S)) {
        player->position.y += 2;
        player->is_moving = true;
    }
    if (IsKeyDown(KEY_D)) {
        player->position.x += 2;
        player->w_direction = WORLD_DIRECTION_RIGHT;
        player->is_moving = true;
    }
    if (IsKeyUp(KEY_W) && IsKeyUp(KEY_A) && IsKeyUp(KEY_S) && IsKeyUp(KEY_D)) {    
        player->is_moving = false;
    }

    player->collision.x = player->position.x - player->dimentions_div2.x;
    player->collision.y = player->position.y - player->dimentions_div2.y;
    player->collision.width = player->dimentions.x;
    player->collision.height = player->dimentions.y;

    //update_abilities(&player->ability_system, player->position);
    update_sprite_renderqueue();

    return true;
}

bool render_player() {
    if (!player) { return false; }

    if(!player->is_dead) {
        if(player->is_damagable){
            if(player->is_moving) switch (player->w_direction) 
            {
                case WORLD_DIRECTION_LEFT: play_anim(PLAYER_ANIMATION_MOVE_LEFT);
                break;
                case WORLD_DIRECTION_RIGHT: play_anim(PLAYER_ANIMATION_MOVE_RIGHT);
                break;
                default: {
                    TraceLog(LOG_WARNING, "player::render_player()::Player has no directions");
                    break;
                }
            }
            else switch (player->w_direction) 
            {
                case WORLD_DIRECTION_LEFT: play_anim(PLAYER_ANIMATION_IDLE_LEFT);
                break;
                case WORLD_DIRECTION_RIGHT:play_anim(PLAYER_ANIMATION_IDLE_RIGHT);
                break;
                default: {
                    TraceLog(LOG_WARNING, "player::render_player()::Player has no directions");
                    break;
                }
            }
        }
        else{
            (player->w_direction == WORLD_DIRECTION_LEFT) 
                ? play_anim(PLAYER_ANIMATION_TAKE_DAMAGE_LEFT)
                : play_anim(PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT);
        }
    }
    else {
        (player->w_direction == WORLD_DIRECTION_LEFT) 
            ? play_anim(PLAYER_ANIMATION_WRECK_LEFT)
            : play_anim(PLAYER_ANIMATION_WRECK_RIGHT);
    }
    
    //render_abilities(&player->ability_system);
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

void play_anim(spritesheet_type player_anim_sheet) {
    Rectangle dest = (Rectangle) {
        .x = player->position.x,
        .y = player->position.y,
        .width = player->dimentions.x,
        .height = player->dimentions.y
    };
    switch (player_anim_sheet) {
        case PLAYER_ANIMATION_MOVE_LEFT: {
            play_sprite_on_site(player->move_left_sprite_queue_index, dest, WHITE);
            player->last_played_sprite_id = player->move_left_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_MOVE_RIGHT: {
            play_sprite_on_site(player->move_right_sprite_queue_index, dest, WHITE);
            player->last_played_sprite_id = player->move_right_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_IDLE_LEFT:  {
            play_sprite_on_site(player->idle_left_sprite_queue_index, dest, WHITE);
            player->last_played_sprite_id = player->idle_left_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_IDLE_RIGHT:  {
            play_sprite_on_site(player->idle_right_sprite_queue_index, dest, WHITE);
            player->last_played_sprite_id = player->idle_right_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_TAKE_DAMAGE_LEFT:  {
            play_sprite_on_site(player->take_damage_left_sprite_queue_index, dest, WHITE);
            player->last_played_sprite_id = player->take_damage_left_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT:  {
            play_sprite_on_site(player->take_damage_right_sprite_queue_index, dest, WHITE);
            player->last_played_sprite_id = player->take_damage_right_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_WRECK_LEFT:  {
            play_sprite_on_site(player->wreck_left_sprite_queue_index, dest, WHITE);
            player->last_played_sprite_id = player->wreck_left_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_WRECK_RIGHT:  {
            play_sprite_on_site(player->wreck_right_sprite_queue_index, dest, WHITE);
            player->last_played_sprite_id = player->wreck_right_sprite_queue_index;
            break;
        }
        
        default: TraceLog(LOG_ERROR, "ERROR::player::move()::move function called with unspecified value.");
        break;
    }
}

bool player_system_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_PLAYER_ADD_EXP: {
            add_exp_to_player(context.data.u32[0]);
            event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, 0, (event_context){
                .data.f32[0] = PRG_BAR_ID_PLAYER_EXPERIANCE,
                .data.f32[1] = player->exp_perc,
            });
            return true;
        }
        case EVENT_CODE_PLAYER_SET_POSITION: {
            player->position.x = context.data.f32[0]; 
            player->position.y = context.data.f32[1]; 
            player->collision.x = player->position.x; 
            player->collision.y = player->position.y; 
            return true;
        }
        case EVENT_CODE_PLAYER_TAKE_DAMAGE: {
            take_damage(context.data.u8[0]);
            event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, 0, (event_context){
                .data.f32[0] = PRG_BAR_ID_PLAYER_HEALTH,
                .data.f32[1] = player->health_perc,
            });
            return true;
        }
        default: return false; // TODO: Warn
    }

    return false;
}

#undef PSPRITESHEET_SYSTEM

