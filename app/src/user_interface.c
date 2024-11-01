#include "user_interface.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "core/window.h"

#include "game/camera.h"

bool showMessageBox = false;
Vector2 screen_center = {0};
Vector2 screen_up_left = {0};

void update_user_interface(Vector2 offset) 
{
    Camera2D cam = get_active_camera();
    Vector2 screen_half_size = get_screen_half_size();

    screen_center = GetWorldToScreen2D(cam.target, cam);
    screen_up_left = (Vector2)
    {
        screen_center.x - screen_half_size.x / 2 + offset.x,
        screen_center.y - screen_half_size.y / 2 + offset.y
    };
}

void render_user_interface() {
    if (GuiButton((Rectangle){24, 24, 120, 30}, "#191#Show Message")) showMessageBox = true;

    if (showMessageBox) {
        int result = GuiMessageBox((Rectangle){85, 70, 250, 100},
                                   "#191#Message Box", "Hi! This is a message!", "Nice;Cool");

        if (result >= 0) showMessageBox = false;
    }

    DrawFPS(screen_up_left.x, screen_up_left.y);
}