#include "window.h"

#include "defines.h"
#include "game/camera.h"
#include "raylib.h"

bool create_window(const char* title) {
    if (IsWindowReady()) return false;

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, title);

    create_camera((Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, 0);

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    SetTargetFPS(TARGET_FPS);  // Set our game to run at 60 frames-per-second

    return true;
}

Vector2 get_screen_size() {
    return (Vector2) { SCREEN_WIDTH, SCREEN_HEIGHT};
}
Vector2 get_screen_half_size() {
    return (Vector2) { SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2};
}

void pre_draw() 
{
    BeginDrawing();

    ClearBackground((Color){19, 15, 64, 1.f});
    BeginMode2D(get_active_camera());
}

void post_draw() 
{
    EndMode2D();
}