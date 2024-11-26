#include "camera.h"

#include "core/fmath.h"
#include "raylib.h"

static Camera2D camera;
static float camera_min_speed = 30;
static float camera_min_effect_lenght = 10;
static float camera_fraction_speed = 5.f;

Camera2D create_camera(Vector2 position, u8 rotation) {
    
    Camera2D cam = {0};

    cam.offset = (Vector2){SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2};
    cam.target = (Vector2){position.x, position.y};
    cam.rotation = rotation;
    cam.zoom = 1.0f;

    camera = cam;

    return camera;
} 

Camera2D get_active_camera() {
    return camera;
}

bool update_camera(Vector2 position) 
{
    camera.offset = (Vector2){SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2};

    Vector2 diff = vec2_subtract(position, camera.target);
    float length = vec2_lenght(diff);

    if (length > camera_min_effect_lenght)
    {
        float speed = FMAX(camera_fraction_speed*length, camera_min_speed);
        camera.target = vec2_add(camera.target, vec2_scale(diff, speed*GetFrameTime()/length));
    }

    return true;
}
