#include "camera.h"

static Camera2D camera;

Camera2D create_camera(Vector2 position, u16 screenWidth, u16 screenHeight, u8 rotation) {
    
    Camera2D cam = {0};

    cam.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};
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
    camera.target = (Vector2) {position.x, position.y};

    return true;
}
