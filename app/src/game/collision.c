#include "collision.h"

collision_system* state;

bool collision_system_initialized = false;

bool collision_system_initialize() {
    if (collision_system_initialized) return false;

    state = (collision_system*)malloc(sizeof(collision_system));
    state->enemy_collisions = (collision*)malloc(MAX_SPAWN_COUNT * sizeof(collision));
    state->enemy_projectiles = (collision*)malloc(MAX_PROJECTILE_COUNT * sizeof(collision));
    state->player_projectiles = (collision*)malloc(MAX_PROJECTILE_COUNT * sizeof(collision));

    state->enemy_count = 0;
    state->enemy_projectiles_count = 0;
    state->player_projectiles_count = 0;

    collision_system_initialized = true;
    return true;
}

bool update_collision() 
{


    return true;
}

bool render_collision() {
    for (size_t i = 0; i < state->enemy_count; i++) {
        DrawRectangleLines(
            state->enemy_collisions[i].rect.x,
            state->enemy_collisions[i].rect.y,
            state->enemy_collisions[i].rect.width,
            state->enemy_collisions[i].rect.height,
            WHITE);
    }

    for (size_t i = 0; i < state->enemy_projectiles_count; i++) {
        DrawRectangleLines(
            state->enemy_projectiles[i].rect.x,
            state->enemy_projectiles[i].rect.y,
            state->enemy_projectiles[i].rect.width,
            state->enemy_projectiles[i].rect.height,
            WHITE);
    }

    DrawRectangleLines(
        state->player_collision.rect.x,
        state->player_collision.rect.y,
        state->player_collision.rect.width,
        state->player_collision.rect.height,
        WHITE);

    for (size_t i = 0; i < state->player_projectiles_count; i++) {
        DrawRectangleLines(
            state->player_projectiles[i].rect.x,
            state->player_projectiles[i].rect.y,
            state->player_projectiles[i].rect.width,
            state->player_projectiles[i].rect.height,
            WHITE);
    }

    return true;
}

void add_collision(Rectangle rect, actor_type type) {
    switch (type) {
        case ENEMY: {
            state->enemy_collisions[state->enemy_count] = (collision){
                .Id = state->enemy_count,
                .rect = rect,
                .type = type};
            state->enemy_count++;
            break;
        }
        case PROJECTILE_ENEMY: {
            state->enemy_projectiles[state->enemy_projectiles_count] = (collision){
                .Id = state->enemy_projectiles_count,
                .rect = rect,
                .type = type};
            state->enemy_projectiles++;
            break;
        } break;
        case PLAYER: {
            state->player_collision = (collision){
                .Id = 0,
                .rect = rect,
                .type = type};
            break;
        }
        case PROJECTILE_PLAYER: {
            state->player_projectiles[state->player_projectiles_count] = (collision){
                .Id = state->player_projectiles_count,
                .rect = rect,
                .type = type};
            state->player_projectiles_count++;
            break;
        }
        default:
            break;
    }
}

void remove_collision(collision coll, unsigned int id) {
    switch (coll.type) {
        case ENEMY: {
            for (size_t i = 0; i < state->enemy_count; i++) {
                if (state->enemy_collisions[i].Id == id) {
                    if (id != state->enemy_count - 1) {
                        state->enemy_collisions[id] = state->enemy_collisions[state->enemy_count--];
                        state->enemy_collisions[state->enemy_count + 1] = (collision){0};
                    }
                }
            }
            break;
        }
        case PROJECTILE_ENEMY: {
            for (size_t i = 0; i < state->enemy_projectiles_count; i++) {
                if (state->enemy_projectiles[i].Id == id) {
                    if (id != state->enemy_projectiles_count - 1) {
                        state->enemy_projectiles[id] = state->enemy_projectiles[state->enemy_projectiles_count--];
                        state->enemy_projectiles[state->enemy_projectiles_count + 1] = (collision){0};
                    }
                }
            }
            break;
        } break;
        case PLAYER: {
            state->player_collision = (collision){0};
            break;
        }
        case PROJECTILE_PLAYER: {
            for (size_t i = 0; i < state->player_projectiles_count; i++) {
                if (state->player_projectiles[i].Id == id) {
                    if (id != state->player_projectiles_count - 1) {
                        state->player_projectiles[id] = state->player_projectiles[state->player_projectiles_count--];
                        state->player_projectiles[state->player_projectiles_count + 1] = (collision){0};
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}
