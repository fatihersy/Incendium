#include "window.h"

#include "camera.h"

const int screenWidth = 1280;
const int screenHeight = 720;

const Vector2 center_of_screen = (Vector2){ screenWidth / 2, screenHeight / 2};

bool create_window(const char* title) {
    if (IsWindowReady()) return false;

    InitWindow(screenWidth, screenHeight, title);

    create_camera(center_of_screen, screenWidth, screenHeight, 0);

    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second

    return true;
}

Vector2 get_screen_size() {
    return (Vector2) { screenWidth, screenHeight};
}

void begin_draw() 
{
    BeginDrawing();

    ClearBackground((Color){19, 15, 64, 1.f});
    BeginMode2D(get_active_camera());
}

void end_draw() {

    DrawPixel(center_of_screen.x, center_of_screen.y, WHITE);  // Center of the screen

    DrawFPS(10, 10);
    EndMode2D();
    EndDrawing();
}