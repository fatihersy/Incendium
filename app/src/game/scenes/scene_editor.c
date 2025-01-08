#include "scene_editor.h"
#include <defines.h>
#include <settings.h>

#include "core/event.h"
#include "core/fmemory.h"

#include "game/tilemap.h"
#include "game/user_interface.h"
#include "raylib.h"


typedef struct scene_editor_state {
  Camera2D* camera;
  Vector2 target;
  tilemap map;
  tilesheet palette;
  tilemap_tile selected_tile;
  tilemap_stringtify_package package;
  
  bool b_show_tilesheet_tile_selection_screen;
  bool b_is_a_tile_selected;
} scene_editor_state;

static scene_editor_state *state;

void update_bindings();
void update_movement();
void update_zoom_controls();
void update_mouse_bindings();
void update_keyboard_bindings();

void initialize_scene_editor(Camera2D* _camera) {
  if (state) {
    TraceLog(LOG_ERROR, "ERROR::scene_in_game_edit::initialize_scene_editor()::initialize function called multiple times");
    return;
  }
  state = (scene_editor_state*)allocate_memory_linear(sizeof(scene_editor_state), true);
  
  create_tilemap(TILESHEET_TYPE_MAP, (Vector2) {0, 0}, 100, 16*3, WHITE, &state->map);
  if(!state->map.is_initialized) {
    TraceLog(LOG_WARNING, "WARNING::scene_in_game_edit::initialize_scene_in_game_edit()::tilemap initialization failed");
  }
  create_tilesheet(TILESHEET_TYPE_MAP, 16*2, 1.1, &state->palette);
  if(!state->palette.is_initialized) {
    TraceLog(LOG_WARNING, "WARNING::scene_in_game_edit::initialize_scene_in_game_edit()::palette initialization failed");
  }

  user_interface_system_initialize();
  state->camera = _camera;
  state->palette.position = (Vector2) {5, 5};

}

void update_scene_editor() {
  update_bindings();
  update_user_interface();
}

void render_scene_editor() {
  render_tilemap(&state->map);
  DrawPixel(0, 0, RED);
}

void render_interface_editor() {
  
  if(state->b_show_tilesheet_tile_selection_screen) 
  { 
    DrawRectangle(
      0, 0, 
      get_tilesheet_dim(&state->palette).x + state->palette.position.x + state->palette.offset, 
      GetScreenHeight(), 
      WHITE
    );
    render_tilesheet(&state->palette);
  }

  if (state->b_is_a_tile_selected) {
    render_tile(state->selected_tile, 
    (tilemap_tile) 
    {
      .x = GetMousePosition().x,
      .y = GetMousePosition().y,
      .tile_size = state->map.tile_size
    });
  }

  render_user_interface();
}

void update_bindings() {
  update_mouse_bindings();
  update_keyboard_bindings();
}

void update_mouse_bindings() { 
  update_zoom_controls();

  if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && state->b_show_tilesheet_tile_selection_screen && !state->b_is_a_tile_selected) {
    tilemap_tile tile = get_tile_from_sheet_by_mouse_pos(&state->palette, GetMousePosition());
    if (tile.is_initialized) {
      state->selected_tile = tile;
      state->b_is_a_tile_selected = true;
    }
  }
  
  if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && !state->b_show_tilesheet_tile_selection_screen && state->b_is_a_tile_selected) {
    tilemap_tile tile = get_tile_from_map_by_mouse_pos(&state->map, GetScreenToWorld2D(GetMousePosition(), *state->camera));
    state->map.tiles[tile.x][tile.y] = state->selected_tile;
    TraceLog(LOG_INFO, "tile.%d,tile.%d is %d:%d", tile.x, tile.y, state->selected_tile.tile_symbol.c[0], state->selected_tile.tile_symbol.c[1]);
  };
  
  if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && state->b_is_a_tile_selected) {
    tilemap_tile tile = (tilemap_tile) {0};
    state->b_is_a_tile_selected = false;
  }
}
void update_keyboard_bindings() {
  update_movement();

  if (IsKeyReleased(KEY_TAB)) {
    state->b_show_tilesheet_tile_selection_screen = !state->b_show_tilesheet_tile_selection_screen;
  }
  if (IsKeyReleased(KEY_ESCAPE)) {
    event_fire(EVENT_CODE_UI_SHOW_PAUSE_MENU, 0, (event_context){0});
  }


}

void update_zoom_controls() {

  state->camera->zoom += ((float)GetMouseWheelMove()*0.05f);

  if (state->camera->zoom > 3.0f) state->camera->zoom = 3.0f;
  else if (state->camera->zoom < 0.1f) state->camera->zoom = 0.1f;
}
void update_movement() {
  f32 speed = 10;

  if (IsKeyDown(KEY_W)) {
    state->target.y -= speed;
    event_context context = (event_context)
    {
      .data.f32[0] = state->target.x, 
      .data.f32[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, context);
  }
  if (IsKeyDown(KEY_A)) {
    state->target.x -= speed;
    event_context context = (event_context)
    {
      .data.f32[0] = state->target.x, 
      .data.f32[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, context);
  }
  if (IsKeyDown(KEY_S)) {
    state->target.y += speed;
    event_context context = (event_context)
    {
      .data.f32[0] = state->target.x, 
      .data.f32[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, context);
  }
  if (IsKeyDown(KEY_D)) {
    state->target.x += speed;
    event_context context = (event_context)
    {
      .data.f32[0] = state->target.x, 
      .data.f32[1] = state->target.y
    };
    event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, context);
  }
}
