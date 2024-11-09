#include "window.h"

#include "game/camera.h"

#define SCREEN_WIDTH 1920 
#define SCREEN_HEIGHT 1080
#define SCREEN_WIDTH_OFFSET 38
#define SCREEN_HEIGHT_OFFSET 20
#define SCREEN_WIDTH_DIV2 SCREEN_WIDTH / 2.f
#define SCREEN_HEIGHT_DIV2 SCREEN_HEIGHT / 2.f

bool create_window(const char* title) {
    if (IsWindowReady()) return false;

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, title);

    create_camera((Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2}, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second

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
    ClearBackground((Color){19, 15, 64, 1.f});

    BeginDrawing();
    BeginMode2D(get_active_camera());
}

void post_draw() 
{
    EndMode2D();
}