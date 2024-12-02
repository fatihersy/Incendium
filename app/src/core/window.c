#include "window.h"

#include "game/camera.h"

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
