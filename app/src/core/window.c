#include "window.h"

#include "game/camera.h"

#include "user_interface.h"

const int screenWidth = 1280;
const int screenHeight = 720;

const Vector2 screen_half_size = (Vector2){ screenWidth / 2, screenHeight / 2};

bool create_window(const char* title) {
    if (IsWindowReady()) return false;

    InitWindow(screenWidth, screenHeight, title);

    create_camera(screen_half_size, screenWidth, screenHeight, 0);

    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second

    return true;
}

Vector2 get_screen_size() {
    return (Vector2) { screenWidth, screenHeight};
}
Vector2 get_screen_half_size() {
    return (Vector2) { screenWidth, screenHeight};
}

void update_ui() {
    update_user_interface((Vector2){ 5, 5});
}

void begin_draw() 
{
    BeginDrawing();

    ClearBackground((Color){19, 15, 64, 1.f});
    BeginMode2D(get_active_camera());
}

void end_draw() {

    DrawPixel(0, 0, WHITE);  // Center of the world
    EndMode2D();

    render_user_interface();

    EndDrawing();
}