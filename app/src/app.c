#include "app.h"

#include "core/event.h"
#include "core/window.h"
#include "core/ftime.h"
#include "core/fmemory.h"

#include "game/camera.h"
#include "game/game_manager.h"

#include "raylib.h"
#include "user_interface.h"

bool application_on_event(u16 code, void* sender, void* listener_inst, event_context context);

bool app_runing = false;

bool app_initialize() {
    // Essentials
    create_window("title");

    SetExitKey(0);

    // Subsystems
    memory_system_initialize();
    event_system_initialize();
    time_system_initialize();

    // Game
    if (!game_manager_initialize(get_screen_size())) {
        TraceLog(LOG_ERROR, "game_manager() initialization failed");
        return false;
    }

    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);

    app_runing = true;

    return true;
}

bool window_should_close() {
    return app_runing;
}

bool app_update() {
    update_camera(get_player_position());

    update_game_manager();
    update_user_interface((Vector2){5, 5}, get_screen_half_size(), get_current_scene_type(), get_active_camera());

    update_time();

    return true;
}

bool app_render() {
    pre_draw();

    render_game_manager();

    post_draw();

    render_user_interface();

    EndDrawing();

    return true;
}

bool application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code)
    {
    case EVENT_CODE_APPLICATION_QUIT:
        app_runing = false;
        return true;
    default:
        break;
    }

    return false;
}
