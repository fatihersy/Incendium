#include "app.h"
#include "raylib.h"
#include "defines.h"

#include "core/event.h"
#include "core/ftime.h"
#include "core/fmemory.h"

#include "settings.h"
#include "game/camera.h"
#include "game/resource.h"
#include "game/scenes/scene_manager.h"

bool application_on_event(u16 code, void* sender, void* listener_inst, event_context context);

bool app_runing = false;

bool app_initialize() {

    // Subsystems
    memory_system_initialize();
    event_system_initialize();
    time_system_initialize();

    // Platform    
    settings_initialize();
    set_settings_from_ini_file("config.ini");
    app_settings* settings = get_app_settings();
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_BORDERLESS_WINDOWED_MODE | FLAG_WINDOW_TOPMOST | FLAG_WINDOW_UNDECORATED);
    InitWindow(
        settings->resolution[0], 
        settings->resolution[1], 
        settings->title);
    SetTargetFPS(TARGET_FPS); 
    SetExitKey(0);

    // Game
    resource_system_initialize();
    create_camera(get_resolution_div2(), 0);
    if (!scene_manager_initialize()) {
        TraceLog(LOG_ERROR, "scene_manager() initialization failed");
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
    if (GetFPS() > TARGET_FPS) return true;

    update_scene_scene();

    update_time();

    return true;
}

bool app_render() {
    if (GetFPS() > TARGET_FPS) return true;

    BeginDrawing();
    ClearBackground(CLEAR_BACKGROUND_COLOR);
    BeginMode2D(*get_active_camera());

    render_scene_world();

    EndMode2D();

    render_scene_interface();

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
