#include "game_manager.h"

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"
#include "core/ftime.h"

#include "defines.h"
#include "player.h"
#include "spawn.h"

void draw_background();
void show_pause_screen();
void clean_up();

static game_manager_system_state *game_manager_state;

#define PSPRITESHEET_SYSTEM game_manager_state
#include "game/spritesheet.h"

bool game_manager_initialized = false;
bool game_manager_on_event(u16 code, void *sender, void *listener_inst, event_context context);
          
bool game_manager_initialize(Vector2 _screen_size) {
  if (game_manager_initialized)
    return false;

  game_manager_state = (game_manager_system_state *)allocate_memory_linear(
      sizeof(game_manager_system_state), true);

  game_manager_state->gridsize = 33;
  game_manager_state->map_size = game_manager_state->gridsize * 100;
  game_manager_state->screen_size = (Vector2){0};
  game_manager_state->screen_half_size = (Vector2){0};
  game_manager_state->current_scene_type = 0;
  game_manager_state->is_game_paused = false;
  game_manager_initialized = true;

  if (!player_system_initialize()) return false;
  if (!spawn_system_initialize()) return false;

  game_manager_state->screen_size = _screen_size;
  game_manager_state->screen_half_size = (Vector2){_screen_size.x / 2, _screen_size.y / 2};
  game_manager_state->current_scene_type = SCENE_MAIN_MENU;
  load_scene();

  event_register(EVENT_CODE_SCENE_IN_GAME, 0, game_manager_on_event);
  event_register(EVENT_CODE_PAUSE_GAME, 0, game_manager_on_event);
  event_register(EVENT_CODE_UNPAUSE_GAME, 0, game_manager_on_event);
  event_register(EVENT_CODE_RETURN_MAIN_MENU_GAME, 0, game_manager_on_event);
  event_register(EVENT_CODE_RELOCATE_SPAWN_COLLISION, 0, game_manager_on_event);

  return true;
}

void set_player_position(i16 x, i16 y) {
  event_fire(EVENT_CODE_PLAYER_SET_POSITION, 0,
             (event_context){
                 .data.f32[0] = x,
                 .data.f32[1] = y,
             });
}

void set_current_scene_type(scene_type type) {
  game_manager_state->current_scene_type = type;
}
Vector2 get_player_position() { return get_player_state()->position; }

Character2D* get_actor_by_id(u16 ID) {
  spawn_system_state *spawn_data = get_spawn_system();

  for (i32 i = 0; i < MAX_SPAWN_COUNT; i++) {
    if (spawn_data->spawns[i].character_id == ID)
      return &spawn_data->spawns[i];
  }

  return (Character2D *){0};
}

float get_time_elapsed(elapse_time_type type) {
  timer *time = get_timer();

  if (time->remaining == 0) {
    reset_time(type);
    return 0;
  };

  return time->remaining;
}
scene_type get_current_scene_type() {
  return game_manager_state->current_scene_type;
}

Vector2 get_player_dimentions() { return get_player_state()->dimentions; }

void update_game_manager() {
  if (GetFPS() > TARGET_FPS) {return;}

  if (game_manager_state->current_scene_type == SCENE_IN_GAME) {
    if (IsKeyPressed(KEY_ESCAPE)) {
      game_manager_state->is_game_paused = !game_manager_state->is_game_paused;
      (game_manager_state->is_game_paused)
          ? event_fire(EVENT_CODE_PAUSE_GAME, 0, (event_context){0})
          : event_fire(EVENT_CODE_UNPAUSE_GAME, 0, (event_context){0});
    }
    if (!game_manager_state->is_game_paused) {
      update_player();
      update_spawns(get_player_position());
    }

    

  }
}

void render_game_manager() {

  if (GetFPS() > TARGET_FPS)
    return;
  draw_background();

  switch (game_manager_state->current_scene_type) {
  case SCENE_MAIN_MENU: {
    //render_resource_system();
    break;
  }
  case SCENE_IN_GAME: {
    render_player();
    render_spawns();
    //render_resource_system();
    break;
  }
  default:
    break;
  }
}

void load_scene() {
  set_player_position(game_manager_state->screen_size.x / 2,
                      game_manager_state->screen_size.y / 2);

  switch (game_manager_state->current_scene_type) {
  case SCENE_MAIN_MENU: {
    break;
  }
  case SCENE_IN_GAME: {
    for (u32 i = 0; i < 360; i += 20) {
      Vector2 position = get_a_point_of_a_circle(get_player_position(), 500, i);
      Texture2D *tex = get_texture_by_enum(ENEMY_TEXTURE);
      rectangle_collision rect_col = (rectangle_collision){
          .rect = (Rectangle){.x = position.x,
                              .y = position.y,
                              .width = tex->width,
                              .height = tex->height},
          .owner_type = ENEMY
      };
      rect_col.owner_id = spawn_character((Character2D){
          .character_id = 0,
          .tex = tex,
          .initialized = false,

          .collision = rect_col.rect,
          .position = position,
          .w_direction = LEFT,
          .type = ENEMY,

          .rotation = 0,
          .health = 100,
          .damage = 10,
          .speed = 1,
      });
      game_manager_state->spawn_collision_count++;
      game_manager_state->spawn_collisions[game_manager_state->spawn_collision_count] = rect_col;
      game_manager_state->spawn_collisions[game_manager_state->spawn_collision_count].is_active = true;
    }
    break;
  }

  default:
    break;
  }
}

void damage_collide_by_actor_type(Character2D *_character, actor_type _type) {
  if (_character->initialized == false) {
    TraceLog(LOG_ERROR, "ERROR::game_manager::check_any_collide()::Tried to check collide with uninitialized actor");
  }

  for (u16 i = 1; i <= game_manager_state->spawn_collision_count; ++i) {
    if (game_manager_state->spawn_collisions[i].is_active)
    if (CheckCollisionRecs( game_manager_state->spawn_collisions[i].rect, _character->collision))
    {
      u16 result = damage_spawn(game_manager_state->spawn_collisions[i].owner_id, 25);
      if(result == INVALID_ID16) TraceLog(
      LOG_ERROR,
      "ERROR::game_manager::damage_collide_by_actor_type()::Error while trying damage to spawn");
      if (result == 0) {
        u16 col_count = game_manager_state->spawn_collision_count;
        if (i == game_manager_state->spawn_collision_count) {
          game_manager_state->spawn_collisions[col_count] = (rectangle_collision){0};
          game_manager_state->spawn_collision_count--;
        }
        else 
        {
          game_manager_state->spawn_collisions[i] = game_manager_state->spawn_collisions[col_count];
          game_manager_state->spawn_collisions[col_count] = (rectangle_collision){0};
          game_manager_state->spawn_collision_count--;
        }
      }
      return;
    }
  }
}

void draw_background() {
  u16 ms = game_manager_state->map_size;

  switch (game_manager_state->current_scene_type) {
  case SCENE_MAIN_MENU: {
    // Centering guidelines
    /*     DrawLine(screen_size.x / 2, 0, screen_size.x / 2, screen_size.y,
        (Color){255, 255, 255, 255}); DrawLine(0, screen_size.y / 2,
        screen_size.x, screen_size.y / 2, (Color){255, 255, 255, 255}); */
    DrawTexturePro(
        *get_texture_by_enum(BACKGROUND),
        (Rectangle){.x = 0,
                    .y = 0,
                    .width = get_texture_by_enum(BACKGROUND)->width,
                    .height = get_texture_by_enum(BACKGROUND)->height},
        (Rectangle){.x = 0,
                    .y = 0,
                    .width = GetScreenWidth(),
                    .height = GetScreenHeight()},
        (Vector2){.x = 0, .y = 0}, 0,
        WHITE); // Draws the background to main menu
    break;
  }
  case SCENE_IN_GAME: {
    for (i16 i = -ms + 13; i < ms; i += ms) { // X Axis
      DrawLine(i, -ms, i, i + (ms), (Color){21, 17, 71, 255});
    }
    for (i16 i = -ms - 3; i < ms; i += ms) { // Y Axis
      DrawLine(-ms, i, i + (ms), i, (Color){21, 17, 71, 255});
    }
    break;
  }

  default:
    break;
  }
}

bool game_manager_on_event(u16 code, void *sender, void *listener_inst,
                           event_context context) {
  switch (code) {
  case EVENT_CODE_SCENE_IN_GAME: {
    game_manager_state->current_scene_type = SCENE_IN_GAME;
    load_scene();
    return true;
    break;
  }
  case EVENT_CODE_SCENE_MAIN_MENU: {
    game_manager_state->current_scene_type = SCENE_MAIN_MENU;
    load_scene();
    return true;
    break;
  }
  case EVENT_CODE_PAUSE_GAME: {
    game_manager_state->is_game_paused = true;
    event_fire(EVENT_CODE_UI_SHOW_PAUSE_SCREEN, 0, (event_context){0});

    return true;
    break;
  }
  case EVENT_CODE_UNPAUSE_GAME: {
    game_manager_state->is_game_paused = false;
    event_fire(EVENT_CODE_UI_SHOW_UNPAUSE_SCREEN, 0, (event_context){0});

    return true;
    break;
  }
  case EVENT_CODE_RETURN_MAIN_MENU_GAME: {
    game_manager_state->is_game_paused = false;
    game_manager_state->current_scene_type = SCENE_MAIN_MENU;
    clean_up();
    load_scene();

    return true;
    break;
  }
  case EVENT_CODE_RELOCATE_SPAWN_COLLISION: {
    for (int i = 1; i <= game_manager_state->spawn_collision_count; ++i) {
      if(game_manager_state->spawn_collisions[i].owner_id == context.data.u16[0]) {
        game_manager_state->spawn_collisions[i].rect.x = context.data.u16[1];
        game_manager_state->spawn_collisions[i].rect.y = context.data.u16[2];
        return true;
      }
    }
    break;
  }
  default: break;
  }

  return false;
}

void clean_up() { clean_up_spawn_system(); }

#undef PSPRITESHEET_SYSTEM
