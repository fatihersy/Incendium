#include "game.h"

#include "core/window.h"
#include "core/camera.h"
#include "core/fmath.h"
#include "core/ftime.h"
#include "game/player.h"
#include "game/spawn.h"
#include "game/resource.h"
#include "game/collision.h"

const int gridsize = 33;
const int map_size = gridsize * 100;

u16 radius = 0x250;  // 592

void draw_background();

bool game_initialize() {
    create_window("title");

    player_system_initialize();
    spawn_system_initialize();
    resource_system_initialize();
    time_system_initialize();

    get_player_state()->position.x = get_screen_size().x / 2;
    get_player_state()->position.y = get_screen_size().y / 2;

    unsigned int texId = load_texture("D:\\Workspace\\CLang\\Resources\\fudesumi.png", true, (Vector2){64, 64});
    Texture2D tex = get_texture_by_id(texId);

/*     for (size_t i = 0; i < 360; i += 20) {
        Vector2 position = get_a_point_of_a_circle(get_active_camera().offset, 500, i);

        spawn_character((Character2D){
                            .texId = texId,
                            .initialized = true,
                            .position = position,
                            .speed = 1,
                            .health = 100,
                            .damage = 10,
                        },
                        ENEMY);
    } */

    return true;
}

bool game_update() {
    
    update_camera(get_player_state()->position);

    update_player();
    update_spawns();
    update_time();

    return true;
}

bool game_render() {
    begin_draw();

    draw_background();

    render_player();
    render_spawns();

    end_draw();
    return true;
}

void game_on_resize() {
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
