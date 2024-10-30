#include "game_manager.h"

#include "core/fmath.h"
#include "core/ftime.h"
#include "core/fmemory.h"

#include "resource.h"
#include "player.h"
#include "spawn.h"

const int gridsize = 33;
const int map_size = gridsize * 100;

bool game_manager_initialized = false;

void draw_background();

bool game_manager_initialize(Vector2 screen_size) {
    if (game_manager_initialized) return false;

    if(!resource_system_initialize()) return false;
    if(!player_system_initialize()) return false;
    if(!spawn_system_initialize()) return false;

    set_player_position(screen_size.x / 2, screen_size.y / 2);

    unsigned int texId = load_texture("D:\\Workspace\\CLang\\Resources\\fudesumi.png", true, (Vector2){64, 64});

    for (u32 i = 0; i < 360; i += 20) {
        Vector2 position = get_a_point_of_a_circle(get_player_position(), 500, i);

        spawn_character((Character2D){
                            .texId = texId,
                            .initialized = true,
                            .position = position,
                            .speed = 1,
                            .health = 100,
                            .damage = 10,
                        },
                        ENEMY);
    }

    game_manager_initialized = true;

    return true;
}


void update_game_manager() {
    update_player();
    update_spawns();
}

void render_game_manager() {

    draw_background();

    render_player();
    render_spawns();
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

float get_time_elapsed(elapse_time_type type) {
    timer* time = get_timer();

    if (time->remaining == 0) {
        reset_time(type);
        return 0;
    };

    return time->remaining;
}

bool damage_any_collade(Character2D* _character) {
    spawn_system_state* spawn_system = get_spawn_system();

    for (u32 i = 0; i < spawn_system->current_spawn_count; i++) {
        if (CheckCollisionRecs(spawn_system->spawns[i].collision_rect, _character->collision_rect)) {
            kill_spawn(spawn_system->spawns[i].character_id);
            return true;
        }
    }

    return false;
}

void draw_background() {
    // Background
    for (i16 i = -map_size + 13; i < map_size; i += gridsize) {  // X Axis
        DrawLine(i, -map_size, i, i + (map_size), (Color){21, 17, 71, 255});
    }
    for (i16 i = -map_size - 3; i < map_size; i += gridsize) {  // Y Axis
        DrawLine(-map_size, i, i + (map_size), i, (Color){21, 17, 71, 255});
    }
}
