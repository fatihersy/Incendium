#include "game_manager.h"

#include "core/fmath.h"
#include "core/ftime.h"
#include "core/event.h"

#include "defines.h"
#include "resource.h"
#include "player.h"
#include "spawn.h"

#include "user_interface.h"

const int gridsize = 33;
const int map_size = gridsize * 100;
Vector2 screen_size = {0};
Vector2 screen_half_size = {0};

scene_type current_scene_type = 0;

bool game_manager_initialized = false;
bool is_game_paused = false;

void draw_background();
void show_pause_screen();

bool game_manager_on_event(u16 code, void* sender, void* listener_inst, event_context context);

bool game_manager_initialize(Vector2 _screen_size) {
    if (game_manager_initialized) return false;

    if (!resource_system_initialize()) return false;
    if (!player_system_initialize()) return false;
    if (!spawn_system_initialize()) return false;
    user_interface_system_initialize();

    screen_size = _screen_size;
    screen_half_size = (Vector2){ _screen_size.x / 2, _screen_size.y / 2 };

    current_scene_type = scene_main_menu;
    load_scene();

    game_manager_initialized = true;

    event_register(EVENT_CODE_SCENE_IN_GAME, 0, game_manager_on_event);
    event_register(EVENT_CODE_PAUSE_GAME, 0, game_manager_on_event);
    event_register(EVENT_CODE_UNPAUSE_GAME, 0, game_manager_on_event);

    return true;
}

void set_player_position(i16 x, i16 y) {
    get_player_state()->position.x = x;
    get_player_state()->position.y = y;
}
void set_current_scene_type(scene_type type) {
    current_scene_type = type;
}
Vector2 get_player_position() {
    return get_player_state()->position;
}
Character2D* get_actor_by_id(u16 ID) {
    spawn_system_state* spawn_data = get_spawn_system();

    for (i32 i = 0; i < MAX_SPAWN_COUNT; i++) {
        if (spawn_data->spawns[i].character_id == ID) return &spawn_data->spawns[i];
    }

    return (Character2D*){0};
}
float get_time_elapsed(elapse_time_type type) {
    timer* time = get_timer();

    if (time->remaining == 0) {
        reset_time(type);
        return 0;
    };

    return time->remaining;
}
scene_type get_current_scene_type() {
    return current_scene_type;
}

void update_game_manager() {
    switch (current_scene_type) {
        case scene_main_menu: {
            break;
        }
        case scene_in_game: {
            if (IsKeyPressed(KEY_ESCAPE) ) 
            {
              is_game_paused = !is_game_paused;
            }
            if (is_game_paused)
            {
              event_fire(EVENT_CODE_PAUSE_GAME, 0, (event_context) {0}); 
              break;
            }

            event_fire(EVENT_CODE_UNPAUSE_GAME, 0, (event_context) {0});
            update_player();
            update_spawns();

            break;
        }

        default:
            break;
    }
}

void render_game_manager() {
    draw_background();

    switch (current_scene_type) {
        case scene_main_menu: {
            break;
        }
        case scene_in_game: {
            render_player();
            render_spawns();
            break;
        }
        default:
            break;
    }
}

void load_scene() {
    set_player_position(screen_size.x / 2, screen_size.y / 2);

    switch (current_scene_type) {
        case scene_main_menu: {
            break;
        }
        case scene_in_game: {
            for (u32 i = 0; i < 360; i += 20) {
                Vector2 position = get_a_point_of_a_circle(get_player_position(), 500, i);

                spawn_character(
                    (Character2D){
                        .res_type = ENEMY_TEXTURE,
                        .initialized = true,
                        .position = position,
                        .speed = 1,
                        .health = 100,
                        .damage = 10,
                    },
                    ENEMY);
            }

            break;
        }

        default:
            break;
    }
}

bool damage_any_collade(Character2D* _character) {
    spawn_system_state* spawn_system = get_spawn_system();

    for (u32 i = 0; i < spawn_system->current_spawn_count; i++) {
        if (CheckCollisionRecs(spawn_system->spawns[i].collision_rect, _character->collision_rect)) {
            kill_spawn(spawn_system->spawns[i].character_id);
            return true;
        }
    }

    return false;
}

void draw_background() {
    switch (current_scene_type) {
        case scene_main_menu: {
            // Centering guidelines
            //DrawLine(screen_size.x / 2, 0, screen_size.x / 2, screen_size.y, (Color){255, 255, 255, 255});
            //DrawLine(0, screen_size.y / 2, screen_size.x, screen_size.y / 2, (Color){255, 255, 255, 255});
            break;
        }
        case scene_in_game: {
            for (i16 i = -map_size + 13; i < map_size; i += gridsize) {  // X Axis
                DrawLine(i, -map_size, i, i + (map_size), (Color){21, 17, 71, 255});
            }
            for (i16 i = -map_size - 3; i < map_size; i += gridsize) {  // Y Axis
                DrawLine(-map_size, i, i + (map_size), i, (Color){21, 17, 71, 255});
            }
            break;
        }

        default:
            break;
    }
}

bool game_manager_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code)
    {
    case EVENT_CODE_SCENE_IN_GAME:
    {
        current_scene_type = scene_in_game;
        load_scene();
        return true;
        break;
    }
    case EVENT_CODE_SCENE_MAIN_MENU:
    {
        current_scene_type = scene_main_menu;
        load_scene();
        return true;
        break;
    }
    case EVENT_CODE_PAUSE_GAME:
    {
        is_game_paused = true;
        event_fire(EVENT_CODE_UI_SHOW_PAUSE_SCREEN, 0, (event_context) {0});
        
        return true;
        break;
    }
    case EVENT_CODE_UNPAUSE_GAME:
    {
        is_game_paused = false;
        event_fire(EVENT_CODE_UI_SHOW_UNPAUSE_SCREEN, 0, (event_context) {0});
        
        return true;
        break;
    }
    default:
        break;
    }

    return false;
}