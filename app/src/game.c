#include "game.h"

#include "core/window.h"
#include "core/camera.h"
#include "core/ftime.h"
#include "core/fmemory.h"

#include "game/game_manager.h"

bool game_initialize() {
    // Essentials
    create_window("title");

    // Subsystems
    memory_system_initialize();
    time_system_initialize();

    // Game
    if(!game_manager_initialize(get_screen_size())) {
        TraceLog(LOG_ERROR, "game_manager() initialization failed");
        return false;
    }

    return true;
}

bool game_update() {
    update_camera(get_player_position());

    update_game_manager();
    update_time();

    return true;
}

bool game_render() {
    begin_draw();

    render_game_manager();

    end_draw();
    return true;
}
