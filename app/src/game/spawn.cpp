#include "spawn.h"
#include <unordered_map>

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"
#include "core/logger.h"
#include "core/ftime.h"

#include "spritesheet.h"
#include "fshader.h"
#include <cmath>

constexpr i32 SPAWN_ID_NEXT_START = 1;
constexpr f32 DEATH_EFFECT_HEIGHT_SCALE = 0.115f;
constexpr f32 CELL_SIZE = 512.0f;
constexpr f32 SPAWN_RND_SCALE_MIN = 1.f;
constexpr f32 SPAWN_RND_SCALE_MAX = 1.5f;
constexpr i32 SPAWN_BASE_HEALTH = 32;
constexpr i32 SPAWN_BASE_DAMAGE = 32;
constexpr i32 SPAWN_BASE_SPEED = 50;

const f32 MAP_X = -3000.0f;
const f32 MAP_Y = -3000.0f; // Assuming square bounds
const f32 MAP_WIDTH = 6000.0f;  // Total width (-3000 to 3000)
const f32 MAP_HEIGHT = 6000.0f; 

#define SPAWN_SCALE_INCREASE_BY_LEVEL(LEVEL) LEVEL * .1

#define SPAWN_HEALTH_CURVE(LEVEL, SCALE, TYPE) (SPAWN_BASE_HEALTH + (SPAWN_BASE_HEALTH * ((SCALE * 0.4f) + (LEVEL * 0.6f) + (TYPE * 0.3f)) * level_curve[LEVEL] * 0.002f))
#define SPAWN_DAMAGE_CURVE(LEVEL, SCALE, TYPE) (SPAWN_BASE_DAMAGE + (SPAWN_BASE_DAMAGE * ((SCALE * 0.45f) + (LEVEL * 0.55f) + (TYPE * 0.35f)) * level_curve[LEVEL] * 0.002f))
#define SPAWN_SPEED_CURVE(LEVEL, SCALE, TYPE)  (SPAWN_BASE_SPEED + ((SPAWN_BASE_SPEED * (LEVEL + (LEVEL * 0.2f)) + (TYPE + (TYPE * 0.25f))) / SCALE))

#define GET_SPW_EXP(CHARACTER)  level_curve[CHARACTER->buffer.i32[1]] * (static_cast<f32>(CHARACTER->type) / static_cast<f32>(SPAWN_TYPE_MAX)) * (CHARACTER->scale / SPAWN_RND_SCALE_MAX)
#define GET_SPW_COIN(CHARACTER) level_curve[CHARACTER->buffer.i32[1]] * (static_cast<f32>(CHARACTER->type) / static_cast<f32>(SPAWN_TYPE_MAX)) * (CHARACTER->scale / SPAWN_RND_SCALE_MAX)

struct SpatialGrid1D {
  i32 cols;
  i32 rows;
  f32 cell_size;
  Vector2 world_origin;
  std::vector<std::vector<Character2D*>> cells;

  SpatialGrid1D(i32 width, i32 height, f32 size, Vector2 origin) : cols(width), rows(height), cell_size(size), world_origin(origin) {
    cells.resize(cols * rows);
  }
  inline i32 get_index(Vector2 pos) const {
    i32 x = static_cast<i32>((pos.x - world_origin.x) / cell_size);
    i32 y = static_cast<i32>((pos.y - world_origin.y) / cell_size);
    
    x = std::max(0, std::min(x, cols - 1));
    y = std::max(0, std::min(y, rows - 1));

    return (y * cols) + x;
  }
  void insert(Character2D* unit) {
    i32 idx = get_index(unit->position);
    cells[idx].push_back(unit);
  }
  void remove(Character2D* unit, Vector2 current_pos) {
    i32 idx = get_index(current_pos);
    std::vector<Character2D*>& bucket = cells[idx];
    
    for (size_t i = 0; i < bucket.size(); ++i) {
      if (bucket[i] == unit) {
        bucket[i] = bucket.back();
        bucket.pop_back();
        return;
      }
    }
  }
  void update(Character2D* unit, Vector2 old_pos) {
    i32 old_idx = get_index(old_pos);
    i32 new_idx = get_index(unit->position);
    if (old_idx != new_idx) {
      std::vector<Character2D*>& old_bucket = cells[old_idx];
      for (size_t i = 0; i < old_bucket.size(); ++i) {
        if (old_bucket[i] == unit) {
          old_bucket[i] = old_bucket.back();
          old_bucket.pop_back();
          break;
        }
      }
      cells[new_idx].push_back(unit);
    }
  }
  void clear() {
    for (auto& bucket : cells) {
      bucket.clear(); 
    }
  }
};

typedef struct spawn_system_state {
  std::vector<Character2D> spawns; // NOTE: See also clean-up function
  const camera_metrics * in_camera_metrics;
  const ingame_info * in_ingame_info;
  i32 next_spawn_id {};
  f32 spawn_follow_distance {};
  SpatialGrid1D spatial_grid;
  std::unordered_map<i32, size_t> spawn_id_to_index_map;

  element_handle nearest_spawn_handle;
  element_handle first_spawn_on_screen_handle;

  spawn_system_state(void) : spatial_grid(
    static_cast<i32>(std::ceil(MAP_WIDTH / CELL_SIZE)),
    static_cast<i32>(std::ceil(MAP_HEIGHT / CELL_SIZE)),
    CELL_SIZE,
    Vector2 { MAP_X, MAP_Y }
  ) {
    this->in_camera_metrics = nullptr;
    this->in_ingame_info = nullptr;
    this->next_spawn_id = SPAWN_ID_NEXT_START;
  }
} spawn_system_state;

static spawn_system_state * state = nullptr;

void spawn_play_anim(Character2D& spawn, spawn_movement_animations sheet);
void remove_spawn(i32 index);
void register_spawn_animation(Character2D& spawn, spawn_movement_animations movement);
void update_spawn_animation(Character2D& spawn);
void spawn_item(std::vector<std::tuple<item_type, i32, data128>> items, data128 context);

bool spawn_on_event(i32 code, event_context context);

bool spawn_system_initialize(const camera_metrics* _camera_metrics, const ingame_info* _ingame_info) {
  if (state and state != nullptr) {
    clean_up_spawn_state();
    return true;
  }
  state = (spawn_system_state *)allocate_memory_linear(sizeof(spawn_system_state), true);
  if (not state or state == nullptr) {
    IERROR("spawn::spawn_system_initialize()::Spawn system init failed");
    return false;
  }
  *state = spawn_system_state();
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;

  event_register(EVENT_CODE_SET_SPAWN_FOLLOW_DISTANCE, spawn_on_event);
  event_register(EVENT_CODE_SET_SPAWN_TINT, spawn_on_event);
  event_register(EVENT_CODE_HALT_SPAWN_MOVEMENT, spawn_on_event);
  event_register(EVENT_CODE_DAMAGE_SPAWN_BY_ID, spawn_on_event);
  event_register(EVENT_CODE_DAMAGE_SPAWN_ROTATED_RECT, spawn_on_event);

  state->spawns.reserve(MAX_SPAWN_COUNT);
  return true;
}

damage_deal_result damage_spawn(i32 _id, i32 damage) {
  auto it = state->spawn_id_to_index_map.find(_id);
  if (it == state->spawn_id_to_index_map.end()) {
    return DAMAGE_DEAL_RESULT_ERROR;
  }
  size_t index = it->second;
  if (index >= state->spawns.size()) {
    return DAMAGE_DEAL_RESULT_ERROR;
  }
  Character2D* character = &state->spawns[index];

  if (not character or character == nullptr) return DAMAGE_DEAL_RESULT_ERROR;

  if (character->type >= SPAWN_TYPE_MAX or character->type <= SPAWN_TYPE_UNDEFINED) {
    (*character) = Character2D();
    return DAMAGE_DEAL_RESULT_ERROR;
  }
  if (not character->is_damagable) { return damage_deal_result(DAMAGE_DEAL_RESULT_IN_DAMAGE_BREAKE, 0, character->health_current); }

  if(character->health_current - damage > 0 && character->health_current - damage < MAX_SPAWN_HEALTH) {
    character->is_damagable = false;
    character->damage_break_time = character->take_damage_left_animation.fps / static_cast<f32>(TARGET_FPS);
    character->health_current -= damage;

    event_fire(EVENT_CODE_SPAWN_COMBAT_FEEDBACK_FLOATING_TEXT, event_context(
      static_cast<f32>(character->collision.x + character->collision.width * .5f), 
      static_cast<f32>(character->collision.y - character->collision.height * .15f),
      static_cast<f32>(damage)
    ));
    return damage_deal_result(DAMAGE_DEAL_RESULT_SUCCESS, damage, character->health_current);
  }
  const i32 remaining_health = character->health_current;
  character->health_current = 0;
  character->is_dead = true;
  character->is_damagable = false;
  character->damage_break_time = character->take_damage_left_animation.fps / static_cast<f32>(TARGET_FPS);
  event_fire(EVENT_CODE_PLAY_SOUND_GROUP, event_context(SOUNDGROUP_ID_ZOMBIE_DIE, static_cast<i32>(true)));
  event_fire(EVENT_CODE_ADD_CURRENCY_SOULS, event_context(static_cast<i32>(1)));

  data128 context = data128(
    static_cast<i16>(character->collision.x + character->collision.width  * .5f), // INFO: Position x
    static_cast<i16>(character->collision.y + character->collision.height * .5f), // INFO: Position y
    static_cast<i16>(character->collision.y + character->collision.height * .5f), // INFO: Loot drop animation position y begin
    static_cast<i16>(character->collision.height * .5f) // INFO: Loot drop animation position y change
  );
  switch (character->type) {
    case SPAWN_TYPE_BROWN: {
      spawn_item(std::vector<std::tuple<item_type, i32, data128>>({
          std::tuple<item_type, i32, data128>(ITEM_TYPE_EXPERIENCE, 100, data128(static_cast<i32>(GET_SPW_EXP(character)))),
          std::tuple<item_type, i32, data128>(ITEM_TYPE_COIN, 65, data128(static_cast<i32>(GET_SPW_COIN(character)))),
        }), 
        context
      );
      break;
    }
    case SPAWN_TYPE_ORANGE: {
      spawn_item(std::vector<std::tuple<item_type, i32, data128>>({
          std::tuple<item_type, i32, data128>(ITEM_TYPE_EXPERIENCE, 100, data128(static_cast<i32>(GET_SPW_EXP(character)))),
          std::tuple<item_type, i32, data128>(ITEM_TYPE_COIN, 65, data128(static_cast<i32>(GET_SPW_COIN(character)))),
        }), 
        context
      );
      break;
    }
    case SPAWN_TYPE_YELLOW: {
      spawn_item(std::vector<std::tuple<item_type, i32, data128>>({
          std::tuple<item_type, i32, data128>(ITEM_TYPE_EXPERIENCE, 100, data128(static_cast<i32>(GET_SPW_EXP(character)))),
          std::tuple<item_type, i32, data128>(ITEM_TYPE_COIN, 65, data128(static_cast<i32>(GET_SPW_COIN(character)))),
        }), 
        context
      );
      break;
    }
    case SPAWN_TYPE_RED: {
      spawn_item(std::vector<std::tuple<item_type, i32, data128>>({
          std::tuple<item_type, i32, data128>(ITEM_TYPE_EXPERIENCE, 100, data128(static_cast<i32>(GET_SPW_EXP(character)))),
          std::tuple<item_type, i32, data128>(ITEM_TYPE_COIN, 65, data128(static_cast<i32>(GET_SPW_COIN(character)))),
        }), 
        context
      );
      break;
    }
    case SPAWN_TYPE_BOSS: {
      spawn_item(std::vector<std::tuple<item_type, i32, data128>>({std::tuple<item_type, i32, data128>(ITEM_TYPE_CHEST, 100, data128())}), context);
      break;
    }
    default: {}
  }
  event_fire(EVENT_CODE_SPAWN_COMBAT_FEEDBACK_FLOATING_TEXT, event_context(
    static_cast<f32>(character->collision.x + character->collision.width * .5f), 
    static_cast<f32>(character->collision.y - character->collision.height * .15f),
    static_cast<f32>(remaining_health)
  ));
  return damage_deal_result(DAMAGE_DEAL_RESULT_SUCCESS, remaining_health, 0);
}
damage_deal_result damage_spawn_by_collision(Rectangle rect, i32 damage, collision_type coll_type) {
  Vector2 min_pos, max_pos;
  switch (coll_type) {
    case COLLISION_TYPE_RECTANGLE_RECTANGLE: {
      min_pos = { rect.x, rect.y };
      max_pos = { rect.x + rect.width, rect.y + rect.height };
      break;
    }
    case COLLISION_TYPE_CIRCLE_RECTANGLE: {
      float radius = rect.width;
      min_pos = { rect.x - radius, rect.y - radius };
      max_pos = { rect.x + radius, rect.y + radius };
      break;
    }
    default: {
      return DAMAGE_DEAL_RESULT_ERROR;
    }
  }

  SpatialGrid1D& grid = state->spatial_grid;

  i32 start_x = static_cast<i32>((min_pos.x - grid.world_origin.x) / grid.cell_size);
  i32 start_y = static_cast<i32>((min_pos.y - grid.world_origin.y) / grid.cell_size);
  i32 end_x   = static_cast<i32>((max_pos.x - grid.world_origin.x) / grid.cell_size);
  i32 end_y   = static_cast<i32>((max_pos.y - grid.world_origin.y) / grid.cell_size);

  start_x = std::max(0, start_x);
  start_y = std::max(0, start_y);
  end_x   = std::min(grid.cols - 1, end_x);
  end_y   = std::min(grid.rows - 1, end_y);

  for (i32 y = start_y; y <= end_y; ++y) {
    i32 row_offset = y * grid.cols;
    for (i32 x = start_x; x <= end_x; ++x) {
      i32 cell_index = row_offset + x;
      
      const std::vector<Character2D*>& bucket = grid.cells[cell_index];
      for (Character2D* neighbor : bucket) {
            
        if (coll_type == COLLISION_TYPE_RECTANGLE_RECTANGLE) {
          if (CheckCollisionRecs(neighbor->collision, rect)) {
            damage_spawn(neighbor->character_id, damage);
          }
        } 
        else if (coll_type == COLLISION_TYPE_CIRCLE_RECTANGLE) {
          if (CheckCollisionCircleRec(Vector2{rect.x, rect.y}, rect.width, neighbor->collision)) {
            damage_spawn(neighbor->character_id, damage);
          }
        }
      }
    }
  }
  return DAMAGE_DEAL_RESULT_SUCCESS; 
}

damage_deal_result damage_spawn_rotated_rect(Rectangle rect, i32 damage, f32 rotation, Vector2 origin) {
  Rectangle search_aabb = get_rotated_rect_aabb(rect, rotation, origin);
  Vector2 min_pos = Vector2{ search_aabb.x, search_aabb.y };
  Vector2 max_pos = Vector2{ search_aabb.x + search_aabb.width, search_aabb.y + search_aabb.height };

  SpatialGrid1D& grid = state->spatial_grid;

  i32 start_x = static_cast<i32>((min_pos.x - grid.world_origin.x) / grid.cell_size);
  i32 start_y = static_cast<i32>((min_pos.y - grid.world_origin.y) / grid.cell_size);
  i32 end_x   = static_cast<i32>((max_pos.x - grid.world_origin.x) / grid.cell_size);
  i32 end_y   = static_cast<i32>((max_pos.y - grid.world_origin.y) / grid.cell_size);

  start_x = std::max(0, start_x);
  start_y = std::max(0, start_y);
  end_x   = std::min(grid.cols - 1, end_x);
  end_y   = std::min(grid.rows - 1, end_y);

  for (i32 y = start_y; y <= end_y; ++y) {
    i32 row_offset = y * grid.cols;
    for (i32 x = start_x; x <= end_x; ++x) {
      i32 cell_index = row_offset + x;
      const std::vector<Character2D*>& bucket = grid.cells[cell_index];
      for (Character2D* neighbor : bucket) {
        if (!CheckCollisionRecs(search_aabb, neighbor->collision)) {
          continue;
        }
        if (check_collision_sat(rect, rotation, origin, neighbor->collision)) {
          damage_spawn(neighbor->character_id, damage);
        }
      }
    }
  }
  return DAMAGE_DEAL_RESULT_SUCCESS;
}

i32 spawn_character(Character2D _character) {
  if (_character.type <= SPAWN_TYPE_UNDEFINED or _character.type >= SPAWN_TYPE_MAX) {
    return -1;
  }
  _character.scale =  SPAWN_RND_SCALE_MIN + (SPAWN_RND_SCALE_MAX * _character.buffer.i32[2] / 100.f);
  _character.scale += SPAWN_SCALE_INCREASE_BY_LEVEL(_character.buffer.i32[1]);

  _character.health_max = SPAWN_HEALTH_CURVE(_character.buffer.i32[1], _character.scale, static_cast<f32>(_character.type));
  _character.damage = SPAWN_DAMAGE_CURVE(_character.buffer.i32[1], _character.scale, static_cast<f32>(_character.type));
  _character.speed =  SPAWN_SPEED_CURVE(_character.buffer.i32[1], _character.scale, static_cast<f32>(_character.type));
  
  _character.health_current = _character.health_max;

  register_spawn_animation(_character, SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT);

  // INFO: Setting collision equal to the any animation dimentions, as we consider each are equal.
  _character.collision.width = _character.move_left_animation.current_frame_rect.width * _character.scale;
  _character.collision.height = _character.move_left_animation.current_frame_rect.height * _character.scale;
  _character.collision.x = _character.position.x;
  _character.collision.y = _character.position.y;
 
  register_spawn_animation(_character, SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT);
  register_spawn_animation(_character, SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT);
  register_spawn_animation(_character, SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT);
  
  _character.death_effect_animation.sheet_id = SHEET_ID_SPAWN_EXPLOSION;
  set_sprite(_character.death_effect_animation, false, true);

  _character.tint = WHITE;
  _character.w_direction = WORLD_DIRECTION_RIGHT;
  _character.last_played_animation = SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT;
  _character.character_id = state->next_spawn_id++;
  _character.initialized = true;

  i32 center_x = static_cast<i32>((_character.position.x - state->spatial_grid.world_origin.x) / state->spatial_grid.cell_size);
  i32 center_y = static_cast<i32>((_character.position.y - state->spatial_grid.world_origin.y) / state->spatial_grid.cell_size);

  for (i32 y = center_y - 1; y <= center_y + 1; ++y) {
    if (y < 0 or y >= state->spatial_grid.rows) continue;

    for (i32 x = center_x - 1; x <= center_x + 1; ++x) {
      if (x < 0 or x >= state->spatial_grid.cols) continue;

      i32 cell_index = (y * state->spatial_grid.cols) + x;
      const std::vector<Character2D*>& bucket = state->spatial_grid.cells[cell_index];

      for (const Character2D* neighbor : bucket) {
        if (CheckCollisionRecs(neighbor->collision, _character.collision)) {
          return -1;
        }
      }
    }
  }
  state->spawns.push_back(_character);
  SpatialGrid1D& grid = state->spatial_grid;

  Character2D* new_spawn_ptr = &state->spawns.back();
  grid.insert(new_spawn_ptr);

  if (_character.collision.width >= CELL_SIZE or _character.collision.height >= CELL_SIZE) {
    IWARN("spawn::spawn_character::Character size is bigger than cell size");
  }

  return _character.character_id;
}

bool update_spawns(Vector2 player_position) {
  if (not state or state == nullptr) { 
    IERROR("spawn::update_spawns()::State is not valid");
    return false; 
  }
  f32 max_spawn_distance = static_cast<f32>(std::numeric_limits<i32>::max());
  
  state->first_spawn_on_screen_handle.id = 0;
  state->first_spawn_on_screen_handle.index = std::numeric_limits<i32>::max();
  state->nearest_spawn_handle.id = 0;
  state->nearest_spawn_handle.index = std::numeric_limits<i32>::max();

  for (size_t spw_index = 0; spw_index < state->spawns.size(); spw_index++) {
    Character2D& spw = state->spawns[spw_index];
    if (not spw.initialized) { 
      continue; 
    }
    if (not spw.is_damagable) {
      if (spw.damage_break_time >= 0) {
        spw.damage_break_time -= (*state->in_ingame_info->delta_time);
      }
      else {
        spw.is_damagable = true;
        reset_sprite(spw.take_damage_left_animation, true);
        reset_sprite(spw.take_damage_right_animation, true);
      }
    }

    if (spw.is_dead ) {
      if (spw.is_on_screen) {
        update_sprite(spw.death_effect_animation, (*state->in_ingame_info->delta_time) );
        update_spawn_animation(spw);
      }
      else {
        spw.initialized = false; // INFO: Forcing to remove
      }
      continue; 
    }

    f32 distance = vec2_distance_sq(spw.position, player_position);
    f32 follow_dist = state->spawn_follow_distance * state->spawn_follow_distance;
    
    if (distance < follow_dist && !spw.cond_halt_move.is_active) {
      const f32 dt = *state->in_ingame_info->delta_time;
      SpatialGrid1D& grid = state->spatial_grid;
      Vector2 old_position = spw.position;

      Vector2 new_position = move_towards(spw.position, player_position, spw.speed * dt);

      bool x0_collide = false;
      bool y0_collide = false;
  
      const Rectangle spw_col = spw.collision;
      const Rectangle x0 = {spw_col.x, new_position.y, spw_col.width, spw_col.height};
      const Rectangle y0 = {new_position.x, spw_col.y, spw_col.width, spw_col.height};
  
    
      i32 center_x = static_cast<i32>((spw.position.x - grid.world_origin.x) / grid.cell_size);
      i32 center_y = static_cast<i32>((spw.position.y - grid.world_origin.y) / grid.cell_size);

      for (i32 y = center_y - 1; y <= center_y + 1; ++y) {
        if (y < 0 or y >= grid.rows) continue;

        for (i32 x = center_x - 1; x <= center_x + 1; ++x) {
          if (x < 0 or x >= grid.cols) continue;
          i32 cell_index = (y * grid.cols) + x;

          const std::vector<Character2D*>& bucket = grid.cells[cell_index];

          for (const Character2D* neighbor : bucket) {
            if (neighbor == &spw) { 
              continue; 
            }

            if (!x0_collide && CheckCollisionRecs(neighbor->collision, x0)) {
              x0_collide = true;
            }
            if (!y0_collide && CheckCollisionRecs(neighbor->collision, y0)) {
              y0_collide = true;
            }

            if (x0_collide && y0_collide) goto collision_resolution;
          }
        }
      }
      collision_resolution:; 
      
      if (!x0_collide) {
        spw.position.y = new_position.y;
        spw.collision.y = spw.position.y;
      }
      if (!y0_collide) {
        spw.w_direction = (spw.position.x > new_position.x) ? WORLD_DIRECTION_LEFT : WORLD_DIRECTION_RIGHT;
        spw.position.x = new_position.x;
        spw.collision.x = spw.position.x;
      }

      state->spatial_grid.update(&spw, old_position);
    }

    if (spw.cond_halt_move.is_active) {
      if (spw.cond_halt_move.accumulator > spw.cond_halt_move.duration) {
        spw.cond_halt_move.accumulator = 0.f;
        spw.cond_halt_move.duration = 0.f;
        spw.cond_halt_move.is_active = false;
      }
      else spw.cond_halt_move.accumulator += (*state->in_ingame_info->delta_time);
    }

    update_spawn_animation(spw);

    // WARN: This event fires 'clean_up_spawn_state()' function if player die from damage
    event_fire(EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE, event_context(
      static_cast<i16>(spw.collision.x), static_cast<i16>(spw.collision.y), static_cast<i16>(spw.collision.width), static_cast<i16>(spw.collision.height),
      static_cast<i16>(spw.damage),
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));
    // WARN: This event fires 'clean_up_spawn_state()' function if player die from damage

    if (state->spawns.empty()) {
      break;
    }
    spw.is_on_screen = CheckCollisionRecs(spw.collision, state->in_camera_metrics->frustum);

    if (spw.is_on_screen and state->first_spawn_on_screen_handle.id < SPAWN_ID_NEXT_START) {
      state->first_spawn_on_screen_handle.id = spw.character_id;
      state->first_spawn_on_screen_handle.index = spw_index;
    }
    if (distance < max_spawn_distance) {
      state->nearest_spawn_handle.id = spw.character_id;
      state->nearest_spawn_handle.index = spw_index;
      max_spawn_distance = distance;
    }
  }
  for (size_t itr_000 = state->spawns.size(); itr_000-- > 0;) {
    const Character2D& spw = state->spawns[itr_000];
    bool should_be_deleted = false;
    if (not spw.initialized) {
      should_be_deleted = true;
    }
    if (spw.is_dead) {
      if (spw.take_damage_left_animation.is_played or spw.take_damage_right_animation.is_played) {
        should_be_deleted = spw.death_effect_animation.is_played;  
      }
      if (not spw.death_effect_animation.is_started) {
        should_be_deleted = true;
      }
    }
    if (not should_be_deleted) {
      continue;
    }

    Character2D* ptr_to_remove = &state->spawns[itr_000];
    state->spatial_grid.remove(ptr_to_remove, ptr_to_remove->position);

    if (itr_000 != state->spawns.size() - 1u) {

      Character2D& moved_spawn = state->spawns.back();

      SpatialGrid1D& grid = state->spatial_grid;
      i32 last_spawn_grid_idx = grid.get_index(moved_spawn.position);

      if (last_spawn_grid_idx != -1) {
        std::vector<Character2D*>& bucket = grid.cells[last_spawn_grid_idx];

        for (size_t k = 0u; k < bucket.size(); ++k) {
          if (bucket[k] == &moved_spawn) {
            bucket[k] = bucket.back();
            bucket.pop_back();
            break; 
          }
        }
      }

      std::swap(state->spawns[itr_000], state->spawns.back());

      i32 moved_id = state->spawns[itr_000].character_id;
      state->spawn_id_to_index_map[moved_id] = itr_000;

      if (last_spawn_grid_idx != -1) {
        grid.cells[last_spawn_grid_idx].push_back(&state->spawns[itr_000]);
      }
    }

    i32 dead_spawn_id = state->spawns.back().character_id;
    state->spawn_id_to_index_map.erase(dead_spawn_id);
    state->spawns.pop_back();
  }

  state->spawn_id_to_index_map.clear();
  for (size_t i = 0u; i < state->spawns.size(); ++i) {
    state->spawn_id_to_index_map[state->spawns[i].character_id] = i;
  }
  return true;
}
void update_spawns_animation_only(void) {

  for (auto& character : state->spawns) {
    update_spawn_animation(character);
  }

}

bool render_spawns(void) {
  if (not state or state == nullptr) {
    IERROR("spawn::render_spawns()::State is not valid");
    return false;
  }
  
  for (Character2D& _character : state->spawns) {
    if (_character.initialized) {

      if(not _character.is_dead and not _character.is_invisible) 
      {
        BeginShaderMode(get_shader_by_enum(SHADER_ID_SPAWN)->handle);
        set_shader_uniform(SHADER_ID_SPAWN, "spawn_id", data128(static_cast<f32>(_character.character_id)));

        if(_character.is_damagable)
        {
          switch (_character.w_direction) 
          {
            case WORLD_DIRECTION_LEFT: spawn_play_anim(_character, SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT);
            break;
            case WORLD_DIRECTION_RIGHT:spawn_play_anim(_character, SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT);
            break;
            default: {
              IWARN("spawn::render_spawns()::Unsupported direction");
              break;
            }
          }
        }	
        else 
        {
          (_character.w_direction == WORLD_DIRECTION_LEFT) 
            ? spawn_play_anim(_character, SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT)
            : spawn_play_anim(_character, SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT);
        }
        EndShaderMode();
      } 
      else 
      {
        if (not _character.is_invisible) {
          BeginShaderMode(get_shader_by_enum(SHADER_ID_SPAWN)->handle);
          set_shader_uniform(SHADER_ID_SPAWN, "spawn_id", data128(static_cast<f32>(_character.character_id)));

          if (_character.w_direction == WORLD_DIRECTION_LEFT) {
            spawn_play_anim(_character, SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT);
          }
          else {
            spawn_play_anim(_character, SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT);
          }
          EndShaderMode();
        }

        const f32 death_effect_height = state->in_camera_metrics->frustum.height * DEATH_EFFECT_HEIGHT_SCALE;
        const f32 death_effect_wh_ratio = _character.death_effect_animation.current_frame_rect.width / _character.death_effect_animation.current_frame_rect.height;
        const f32 death_effect_width = death_effect_height * death_effect_wh_ratio;

        play_sprite_on_site(_character.death_effect_animation, WHITE, Rectangle {
          _character.position.x + (_character.collision.width  * .5f) - (death_effect_width  * .5f), 
          _character.position.y + (_character.collision.height * .5f) - (death_effect_height * .5f),
          death_effect_height, 
          death_effect_width
        });
      }
    }
  }

  return true;
}

std::vector<Character2D>* get_spawns(void) {
  if (not state or state == nullptr) {
    IERROR("spawn::get_spawns()::State was null");
    return nullptr;
  }

  return __builtin_addressof(state->spawns);
}
const Character2D * get_spawn_by_id(i32 _id) {
  if (not state or state == nullptr) {
    IERROR("spawn::get_spawn_by_id()::State is invalid");
    return nullptr;
  }
  auto it = state->spawn_id_to_index_map.find(_id);
  if (it == state->spawn_id_to_index_map.end()) {
    return nullptr;
  }
  size_t index = it->second;
  if (index >= state->spawns.size()) {
    return nullptr;
  }

  return __builtin_addressof(state->spawns.at(index));;
}
const element_handle * get_nearest_spawn(void) {
  return &state->nearest_spawn_handle;
}
const element_handle * get_first_spawn_on_screen(void) {
  return &state->first_spawn_on_screen_handle;
}

void clean_up_spawn_state(void) {
  state->spawns.clear(); 
  state->spatial_grid.clear();
  state->spawn_id_to_index_map.clear();
  state->next_spawn_id = SPAWN_ID_NEXT_START;
}

void spawn_play_anim(Character2D& spawn, spawn_movement_animations movement) {
  Rectangle dest = spawn.collision;

  switch (movement) {
    case SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT: {
      play_sprite_on_site(spawn.move_left_animation, spawn.tint, dest);
      spawn.last_played_animation = SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT;
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT: {
      play_sprite_on_site(spawn.move_right_animation, spawn.tint, dest);
      spawn.last_played_animation = SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT;
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT:  {
      play_sprite_on_site(spawn.take_damage_left_animation, spawn.tint, dest);
      spawn.last_played_animation = SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT:  {
      play_sprite_on_site(spawn.take_damage_right_animation, spawn.tint, dest);
      spawn.last_played_animation = SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
      break;
    }
    default: {
      IWARN("spawn::spawn_play_anim()::Unsupported sheet id");
      break;
    }
  }
}
void update_spawn_animation(Character2D& spawn) {
  switch (spawn.last_played_animation) {
    case SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT: {
      update_sprite(spawn.move_left_animation, (*state->in_ingame_info->delta_time) );
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT: {
      update_sprite(spawn.move_right_animation, (*state->in_ingame_info->delta_time) );
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT:  {
      update_sprite(spawn.take_damage_left_animation, (*state->in_ingame_info->delta_time) );
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT:  {
      update_sprite(spawn.take_damage_right_animation, (*state->in_ingame_info->delta_time) );
      break;
    }
    default: {
      IWARN("spawn::update_spawn_animation()::Unsupported sheet id");
      break;
    }
  }
}
void spawn_item(std::vector<std::tuple<item_type, i32, data128>> items, data128 context) {

  for (auto& item : items) {
    item_type type = std::get<0>(item);
    i32 chance = std::get<1>(item);
    data128 item_values = std::get<2>(item);

    if (get_random(0, 100) < chance) {
      event_fire(EVENT_CODE_SPAWN_ITEM, event_context(static_cast<i16>(type), context.i16[0], context.i16[1], context.i16[2], context.i16[3], item_values.i16[0]));
    }
  }
}

void remove_spawn(i32 index) {
  if (index < 0 or static_cast<size_t>(index) >= state->spawns.size()) {
    IWARN("spawn::remove_spawn()::Index is out of bounds");
    return;
  }
  Character2D& victim = state->spawns[index];

  state->spatial_grid.remove(&victim, victim.position);
  state->spawn_id_to_index_map.erase(victim.character_id);

  if (static_cast<size_t>(index) != state->spawns.size() - 1) {
    Character2D& moved_spawn = state->spawns.back();
    state->spatial_grid.remove(&moved_spawn, moved_spawn.position);
    std::swap(state->spawns[index], state->spawns.back());
    state->spawn_id_to_index_map[moved_spawn.character_id] = index;
    state->spatial_grid.insert(&state->spawns[index]);
  }
  state->spawns.pop_back();
}

void register_spawn_animation(Character2D& spawn, spawn_movement_animations movement) {
  switch (movement) {
    case SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT: {
      switch (spawn.type) {
        case SPAWN_TYPE_BROWN: {
          spawn.move_left_animation.sheet_id = SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(spawn.move_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_ORANGE: {
          spawn.move_left_animation.sheet_id = SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(spawn.move_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_YELLOW: {
          spawn.move_left_animation.sheet_id = SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(spawn.move_left_animation, true, false);
          return;
        } 
        case SPAWN_TYPE_RED: {
          spawn.move_left_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(spawn.move_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_BOSS: {
          spawn.move_left_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(spawn.move_left_animation, true, false);
          return;
        }
        default: {
          IWARN("spawn::register_spawn_animation()::Unsupported spawn type");
          return;
        }
      }
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT: {
      switch (spawn.type) {
        case SPAWN_TYPE_BROWN: {
          spawn.move_right_animation.sheet_id = SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_MOVE_RIGHT;
          set_sprite(spawn.move_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_ORANGE: {
          spawn.move_right_animation.sheet_id = SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_MOVE_RIGHT;
          set_sprite(spawn.move_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_YELLOW: {
          spawn.move_right_animation.sheet_id = SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_MOVE_RIGHT;
          set_sprite(spawn.move_right_animation, true, false);
          return;
        } 
        case SPAWN_TYPE_RED: {
          spawn.move_right_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_RIGHT;
          set_sprite(spawn.move_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_BOSS: {
          spawn.move_right_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_RIGHT;
          set_sprite(spawn.move_right_animation, true, false);
          return;
        }
        default: {
          IWARN("spawn::register_spawn_animation()::Unsupported spawn type");
          return;
        }
      }
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT:  {
      switch (spawn.type) {
        case SPAWN_TYPE_BROWN: {
          spawn.take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(spawn.take_damage_left_animation, false, true);
          return;
        }
        case SPAWN_TYPE_ORANGE: {
          spawn.take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(spawn.take_damage_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_YELLOW: {
          spawn.take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(spawn.take_damage_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_RED: {
          spawn.take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(spawn.take_damage_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_BOSS: {
          spawn.take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(spawn.take_damage_left_animation, true, false);
          return;
        }
        default: {
          IWARN("spawn::register_spawn_animation()::Unsupported spawn type");
          return;
        }
      }
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT:  {
      switch (spawn.type) {
        case SPAWN_TYPE_BROWN: {
          spawn.take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(spawn.take_damage_right_animation, false, true);
          return;
        }
        case SPAWN_TYPE_ORANGE: {
          spawn.take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(spawn.take_damage_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_YELLOW: {
          spawn.take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(spawn.take_damage_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_RED: {
          spawn.take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(spawn.take_damage_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_BOSS: {
          spawn.take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(spawn.take_damage_right_animation, true, false);
          return;
        }
        default: {
          IWARN("spawn::register_spawn_animation()::Unsupported spawn type");
          return;
        }
      }
      break;
    }
    default: {
      IWARN("spawn::register_spawn_animation()::Unsupported sheet id");
      return;
    }
  }

  IERROR("spawn::register_spawn_animation()::Function has ended unexpectedly");
}

bool spawn_on_event(i32 code, event_context context) {
  switch (code) {
    case EVENT_CODE_SET_SPAWN_FOLLOW_DISTANCE: {
      //state->spawn_follow_distance = (*state->in_ingame_info->game_rules)[GAME_RULE_ZOMBIE_FOLLOW_DISTANCE].mm_ex.f32[3];
      state->spawn_follow_distance = context.data.f32[0];
      return true;
    }
    case EVENT_CODE_SET_SPAWN_TINT: {
      const i16& spw_id = context.data.i16[0];
      const i16& spw_index = context.data.i16[1];
      if (static_cast<size_t>(spw_index) >= state->spawns.size() or state->spawns[spw_index].character_id != spw_id) {
        return false;
      }
      state->spawns[spw_index].tint = Color { 
        static_cast<u8>(context.data.i16[2]), 
        static_cast<u8>(context.data.i16[3]), 
        static_cast<u8>(context.data.i16[4]), 
        static_cast<u8>(context.data.i16[5])
      };
      return true;
    }
    case EVENT_CODE_HALT_SPAWN_MOVEMENT: {
      const i32 spw_id = static_cast<i32>(context.data.f32[0]);
      const i32 spw_index = static_cast<i32>(context.data.f32[1]);
      const f32 duration = context.data.f32[2];
      if (spw_index < 0 or static_cast<size_t>(spw_index) >= state->spawns.size()) {
        return false;
      }
      if (state->spawns[spw_index].character_id != spw_id) {
        return false;
      }
      state->spawns[spw_index].cond_halt_move.is_active = true;
      state->spawns[spw_index].cond_halt_move.duration = duration;
      state->spawns[spw_index].cond_halt_move.accumulator = 0.f;
      return true;
    }
    case EVENT_CODE_DAMAGE_SPAWN_BY_ID: {
      damage_spawn(context.data.i32[0], context.data.i32[1]);
      return true;
    }
    case EVENT_CODE_DAMAGE_SPAWN_ROTATED_RECT: {
      const Rectangle rect = {
        static_cast<f32>(context.data.i16[0]), 
        static_cast<f32>(context.data.i16[1]), 
        static_cast<f32>(context.data.i16[2]), 
        static_cast<f32>(context.data.i16[3])
      };
      const i32 damage = static_cast<i32>(context.data.i16[4]);
      const f32 rotation = static_cast<i32>(context.data.i16[5]);
      const Vector2 origin = {
        static_cast<f32>(context.data.i16[6]), 
        static_cast<f32>(context.data.i16[7])
      };
      damage_spawn_rotated_rect(rect, damage, rotation, origin);
      return true;
    }
    default: {
      IWARN("spawn::spawn_on_event()::Unsuppported code.");
      return false;
    }
  }
  IERROR("spawn::spawn_on_event()::Fire event ended unexpectedly");
  return false;
}

#undef DEATH_EFFECT_HEIGHT_SCALE
#undef SPAWN_FOLLOW_DISTANCE
#undef CELL_SIZE
#undef SPAWN_RND_SCALE_MIN
#undef SPAWN_RND_SCALE_MAX
#undef SPAWN_SCALE_INCREASE_BY_LEVEL
#undef SPAWN_BASE_HEALTH
#undef SPAWN_HEALTH_CURVE
#undef SPAWN_BASE_DAMAGE
#undef SPAWN_DAMAGE_CURVE
#undef SPAWN_BASE_SPEED
#undef SPAWN_SPEED_CURVE
#undef GET_SPW_EXP
#undef GET_SPW_COIN
