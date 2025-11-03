#include "spawn.h"
#include <unordered_map>
#include <functional>

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"
#include "core/logger.h"
#include "core/ftime.h"

#include "spritesheet.h"

#include "fshader.h"

struct grid_cell_id {
  i32 x;
  i32 y;

  bool operator==(const grid_cell_id& other) const {
    return x == other.x && y == other.y;
  }
};
namespace std {
  template <>
  struct hash<grid_cell_id> {
    std::size_t operator()(const grid_cell_id& cell) const {

      std::size_t h1 = std::hash<i32>{}(cell.x);
      std::size_t h2 = std::hash<i32>{}(cell.y);
      return h1 ^ (h2 << 1);
    }
  };
}
using SpatialHash = std::unordered_map<grid_cell_id, std::vector<Character2D*>>;

typedef struct spawn_system_state {
  std::vector<Character2D> spawns; // NOTE: See also clean-up function
  const camera_metrics * in_camera_metrics;
  const ingame_info * in_ingame_info;
  i32 next_spawn_id;
  f32 spawn_follow_distance;
  SpatialHash spatial_grid;
  std::unordered_map<i32, size_t> spawn_id_to_index_map;

  spawn_system_state(void) {
    this->spawns = std::vector<Character2D>();
    this->in_camera_metrics = nullptr;
    this->in_ingame_info = nullptr;
    this->next_spawn_id = 0;
    this->spawn_follow_distance = 0.f;
    this->spatial_grid = SpatialHash();
  }
} spawn_system_state;

// To avoid dublicate symbol errors. Implementation in defines.h
extern const i32 level_curve[MAX_PLAYER_LEVEL + 1];
static spawn_system_state * state = nullptr;

#define DEATH_EFFECT_HEIGHT_SCALE 0.115f
#define SPAWN_FOLLOW_DISTANCE 1000.f;
#define CELL_SIZE 256.0f

#define CHECK_pSPAWN_COLLISION(REC1, SPAWN, NEW_POS) \
CheckCollisionRecs(REC1, \
  (Rectangle) { \
    NEW_POS.x, NEW_POS.y, \
    SPAWN->collision.width, SPAWN->collision.height\
})
#define SPAWN_RND_SCALE_MIN 1.5f
#define SPAWN_RND_SCALE_MAX 2.5f
#define SPAWN_SCALE_INCREASE_BY_LEVEL(LEVEL) LEVEL * .1

#define SPAWN_BASE_HEALTH 32
#define SPAWN_HEALTH_CURVE(LEVEL, SCALE, TYPE) (SPAWN_BASE_HEALTH + (SPAWN_BASE_HEALTH * ((SCALE * 0.4f) + (LEVEL * 0.6f) + (TYPE * 0.3f)) * level_curve[LEVEL] * 0.002f))

#define SPAWN_BASE_DAMAGE 16
#define SPAWN_DAMAGE_CURVE(LEVEL, SCALE, TYPE) (SPAWN_BASE_DAMAGE + (SPAWN_BASE_DAMAGE * ((SCALE * 0.45f) + (LEVEL * 0.55f) + (TYPE * 0.35f)) * level_curve[LEVEL] * 0.002f))

#define SPAWN_BASE_SPEED 50
#define SPAWN_SPEED_CURVE(LEVEL, SCALE, TYPE) (SPAWN_BASE_SPEED + ((SPAWN_BASE_SPEED * (LEVEL + (LEVEL * 0.2f)) + (TYPE + (TYPE * 0.25f))) / SCALE))

#define GET_SPW_EXP(CHARACTER)  level_curve[CHARACTER->genp.i32[1]] * (static_cast<f32>(CHARACTER->type) / static_cast<f32>(SPAWN_TYPE_MAX)) * (CHARACTER->scale / SPAWN_RND_SCALE_MAX)
#define GET_SPW_COIN(CHARACTER) level_curve[CHARACTER->genp.i32[1]] * (static_cast<f32>(CHARACTER->type) / static_cast<f32>(SPAWN_TYPE_MAX)) * (CHARACTER->scale / SPAWN_RND_SCALE_MAX)

void spawn_play_anim(Character2D& spawn, spawn_movement_animations sheet);
void remove_spawn(i32 index);
void register_spawn_animation(Character2D& spawn, spawn_movement_animations movement);
void update_spawn_animation(Character2D& spawn);
void spawn_item(std::vector<std::tuple<item_type, i32, data128>> items, data128 context);

grid_cell_id get_cell_id(Vector2 position) {
  return {
    static_cast<int>(position.x / CELL_SIZE),
    static_cast<int>(position.y / CELL_SIZE)
  };
}
void populate_grid(SpatialHash& grid, std::vector<Character2D>& spawns) {
  grid.clear();

  for (auto& spawn : spawns) {
    if (spawn.is_dead or not spawn.initialized) {
      continue; 
    }
    grid_cell_id cell = get_cell_id(spawn.position);
    grid[cell].push_back(&spawn);
  }
}

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
  state->spawn_follow_distance = SPAWN_FOLLOW_DISTANCE;

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
    return damage_deal_result(DAMAGE_DEAL_RESULT_SUCCESS, damage, character->health_current);
  }
  const i32 remaining_health = character->health_current;
  character->health_current = 0;
  character->is_dead = true;
  character->is_damagable = false;
  character->damage_break_time = character->take_damage_left_animation.fps / static_cast<f32>(TARGET_FPS);
  event_fire(EVENT_CODE_PLAY_SOUND_GROUP, event_context(SOUNDGROUP_ID_ZOMBIE_DIE, static_cast<i32>(true)));

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

  return damage_deal_result(DAMAGE_DEAL_RESULT_SUCCESS, remaining_health, 0);
}

i32 spawn_character(Character2D _character) {
  if (_character.type <= SPAWN_TYPE_UNDEFINED or _character.type >= SPAWN_TYPE_MAX) {
    return -1;
  }
  _character.scale =  SPAWN_RND_SCALE_MIN + (SPAWN_RND_SCALE_MAX * _character.genp.i32[2] / 100.f);
  _character.scale += SPAWN_SCALE_INCREASE_BY_LEVEL(_character.genp.i32[1]);

  _character.health_max = SPAWN_HEALTH_CURVE(_character.genp.i32[1], _character.scale, static_cast<f32>(_character.type));
  _character.damage = SPAWN_DAMAGE_CURVE(_character.genp.i32[1], _character.scale, static_cast<f32>(_character.type));
  _character.speed =  SPAWN_SPEED_CURVE(_character.genp.i32[1], _character.scale, static_cast<f32>(_character.type));
  
  _character.health_current = _character.health_max;

  register_spawn_animation(_character, SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT);

  // INFO: Setting collision equal to the any animation dimentions, as we consider each are equal.
  _character.collision.width = _character.move_left_animation.current_frame_rect.width * _character.scale;
  _character.collision.height = _character.move_left_animation.current_frame_rect.height * _character.scale;
  _character.collision.x = _character.position.x;
  _character.collision.y = _character.position.y;
  if(CheckCollisionRecs(state->in_camera_metrics->frustum, _character.collision)) { return -1; }

  register_spawn_animation(_character, SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT);
  register_spawn_animation(_character, SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT);
  register_spawn_animation(_character, SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT);
  
  _character.death_effect_animation.sheet_id = SHEET_ID_SPAWN_EXPLOSION;
  set_sprite(__builtin_addressof(_character.death_effect_animation), false, true);

  _character.w_direction = WORLD_DIRECTION_RIGHT;
  _character.last_played_animation = SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT;
  _character.character_id = state->next_spawn_id++;
  _character.initialized = true;

  grid_cell_id cell = get_cell_id(_character.position);
  for (i32 x = cell.x - 1; x <= cell.x + 1; ++x) {
    for (i32 y = cell.y - 1; y <= cell.y + 1; ++y) {
      auto it = state->spatial_grid.find({x, y});
      if (it == state->spatial_grid.end()) {
        continue;
      }
      for (Character2D* neighbor : it->second) {
        if (CheckCollisionRecs(neighbor->collision, _character.collision)) {
            return -1;
        }
      }
    }
  }
  size_t old_capacity = state->spawns.capacity();
  state->spawns.push_back(_character);
  size_t character_index = state->spawns.size() - 1u;
  state->spawn_id_to_index_map[_character.character_id] = character_index;
  if (state->spawns.capacity() != old_capacity) {
    populate_grid(state->spatial_grid, state->spawns);
  } else {
    state->spatial_grid[cell].push_back(&state->spawns[character_index]);
  }

  return _character.character_id;
}

bool update_spawns(Vector2 player_position) {
  if (not state or state == nullptr) { 
    IERROR("spawn::update_spawns()::State is not valid");
    return false; 
  }

  for (auto& character : state->spawns) {
    if (not character.initialized) { 
      continue; 
    }
    if (not character.is_damagable) {
      if (character.damage_break_time >= 0) {
        character.damage_break_time -= (*state->in_ingame_info->delta_time);
      }
      else {
        character.is_damagable = true;
        reset_sprite(__builtin_addressof(character.take_damage_left_animation), true);
        reset_sprite(__builtin_addressof(character.take_damage_right_animation), true);
      }
    }
    character.is_on_screen = CheckCollisionRecs(character.collision, state->in_camera_metrics->frustum);

    if (character.is_dead) {
      if (character.is_on_screen) {
        update_sprite(__builtin_addressof(character.death_effect_animation), (*state->in_ingame_info->delta_time) );
        update_spawn_animation(character);
      }
      else {
        character.initialized = false; // INFO: Forcing to remove
      }
      continue; 
    }

    grid_cell_id old_cell = get_cell_id(character.position);
    Vector2 new_position = move_towards(character.position, player_position, character.speed * (*state->in_ingame_info->delta_time) );
    bool x0_collide = false;
    bool y0_collide = false;

    grid_cell_id current_cell = get_cell_id(character.position);

    for (int x = current_cell.x - 1; x <= current_cell.x + 1; ++x) { 
      for (int y = current_cell.y - 1; y <= current_cell.y + 1; ++y) {

        auto it = state->spatial_grid.find({x, y});
        if (it == state->spatial_grid.end()) {
          continue;
        }
        for (Character2D* neighbor : it->second) {
          if (neighbor->character_id == character.character_id) {
            continue;
          }
          const Rectangle spw_col = character.collision;
          const Rectangle x0 = {spw_col.x, new_position.y, spw_col.width, spw_col.height};
          const Rectangle y0 = {new_position.x, spw_col.y, spw_col.width, spw_col.height};
          if (CheckCollisionRecs(neighbor->collision, x0)) {
            x0_collide = true;
          }
          if (CheckCollisionRecs(neighbor->collision, y0)) {
            y0_collide = true;
          }
        }
      }
    }

    if (not x0_collide) {
      character.position.y = new_position.y;
      character.collision.y = character.position.y;
    }
    if (not y0_collide) {
      if (character.position.x - new_position.x > 0) {
        character.w_direction = WORLD_DIRECTION_LEFT;
      } else {
        character.w_direction = WORLD_DIRECTION_RIGHT;
      }
      character.position.x = new_position.x;
      character.collision.x = character.position.x;
    }
    update_spawn_animation(character);

    grid_cell_id new_cell = get_cell_id(character.position);
    if (new_cell.x != old_cell.x or new_cell.y != old_cell.y) {
      auto it = state->spatial_grid.find(old_cell);
      if (it != state->spatial_grid.end()) {
        std::erase(it->second, &character);
      }
      state->spatial_grid[new_cell].push_back(&character);
    }

    // WARN: This event fires 'clean_up_spawn_state()' function if player die from damage
    event_fire(EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE, event_context(
      static_cast<i16>(character.collision.x), static_cast<i16>(character.collision.y), static_cast<i16>(character.collision.width), static_cast<i16>(character.collision.height),
      static_cast<i16>(character.damage),
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));
    // WARN: This event fires 'clean_up_spawn_state()' function if player die from damage

    if (state->spawns.empty()) {
      break;
    }
  }
  for (size_t itr_000 = state->spawns.size(); itr_000-- > 0;) {
    const Character2D& spw = state->spawns[itr_000];
    bool should_be_deleted = false;
    if (not spw.initialized) {
      should_be_deleted = true;
    }
    if (spw.is_dead or (spw.health_current <= 0) or spw.health_current > MAX_SPAWN_HEALTH) {
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

    i32 dead_spawn_id = state->spawns[itr_000].character_id;
    grid_cell_id dead_cell = get_cell_id(spw.position);
    auto it_dead = state->spatial_grid.find(dead_cell);
    if (it_dead != state->spatial_grid.end()) {
      std::erase(it_dead->second, &spw);
    }

    if (itr_000 != state->spawns.size() - 1u) {
      // Compute the cell for the last element before swap
      grid_cell_id last_cell = get_cell_id(state->spawns.back().position);
      auto it_last = state->spatial_grid.find(last_cell);
      if (it_last != state->spatial_grid.end()) {
          std::erase(it_last->second, &state->spawns.back());
      }
    
      // Proceed with swap
      i32 last_spawn_id = state->spawns.back().character_id;
      std::swap(state->spawns[itr_000], state->spawns.back());
      state->spawn_id_to_index_map[last_spawn_id] = itr_000;
    
      // After swap, re-add the (now moved) live element's new pointer to its cell
      // (cell hasn't changed since position is part of the swapped data)
      state->spatial_grid[last_cell].push_back(&state->spawns[itr_000]);
    }
    state->spawns.pop_back();
    state->spawn_id_to_index_map.erase(dead_spawn_id);
  }

  state->spawn_id_to_index_map.clear();
  for (size_t i = 0u; i < state->spawns.size(); ++i) {
    state->spawn_id_to_index_map[state->spawns[i].character_id] = i;
  }
  return true;
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

        play_sprite_on_site(__builtin_addressof(_character.death_effect_animation), WHITE, Rectangle {
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

void clean_up_spawn_state(void) {
  for (Character2D& spw : state->spawns) {
    spw = Character2D();
  }
  state->spawns.clear();
  state->spawns.shrink_to_fit();

  state->spatial_grid.clear();
  state->spawn_id_to_index_map.clear();
  state->next_spawn_id = 0;
}

void spawn_play_anim(Character2D& spawn, spawn_movement_animations movement) {
  Rectangle dest = spawn.collision;

  switch (movement) {
    case SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT: {
      play_sprite_on_site(&spawn.move_left_animation, WHITE, dest);
      spawn.last_played_animation = SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT;
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT: {
      play_sprite_on_site(&spawn.move_right_animation, WHITE, dest);
      spawn.last_played_animation = SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT;
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT:  {
      play_sprite_on_site(&spawn.take_damage_left_animation, WHITE, dest);
      spawn.last_played_animation = SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT:  {
      play_sprite_on_site(&spawn.take_damage_right_animation, WHITE, dest);
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
      update_sprite(&spawn.move_left_animation, (*state->in_ingame_info->delta_time) );
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT: {
      update_sprite(&spawn.move_right_animation, (*state->in_ingame_info->delta_time) );
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT:  {
      update_sprite(&spawn.take_damage_left_animation, (*state->in_ingame_info->delta_time) );
      break;
    }
    case SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT:  {
      update_sprite(&spawn.take_damage_right_animation, (*state->in_ingame_info->delta_time) );
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
  if (not state or state == nullptr) {
    IERROR("spawn::remove_spawn()::State is invalid"); 
    return; 
  }
  if (static_cast<size_t>(index) >= state->spawns.size() or index < 0) {
    IWARN("spawn::remove_spawn()::Index is out of bounds"); 
    return; 
  }
  state->spawns.erase(state->spawns.begin() + index);
}

void register_spawn_animation(Character2D& spawn, spawn_movement_animations movement) {
  switch (movement) {
    case SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT: {
      switch (spawn.type) {
        case SPAWN_TYPE_BROWN: {
          spawn.move_left_animation.sheet_id = SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(&spawn.move_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_ORANGE: {
          spawn.move_left_animation.sheet_id = SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(&spawn.move_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_YELLOW: {
          spawn.move_left_animation.sheet_id = SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(&spawn.move_left_animation, true, false);
          return;
        } 
        case SPAWN_TYPE_RED: {
          spawn.move_left_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(&spawn.move_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_BOSS: {
          spawn.move_left_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_LEFT;
          set_sprite(&spawn.move_left_animation, true, false);
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
          set_sprite(&spawn.move_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_ORANGE: {
          spawn.move_right_animation.sheet_id = SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_MOVE_RIGHT;
          set_sprite(&spawn.move_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_YELLOW: {
          spawn.move_right_animation.sheet_id = SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_MOVE_RIGHT;
          set_sprite(&spawn.move_right_animation, true, false);
          return;
        } 
        case SPAWN_TYPE_RED: {
          spawn.move_right_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_RIGHT;
          set_sprite(&spawn.move_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_BOSS: {
          spawn.move_right_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_RIGHT;
          set_sprite(&spawn.move_right_animation, true, false);
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
          set_sprite(&spawn.take_damage_left_animation, false, true);
          return;
        }
        case SPAWN_TYPE_ORANGE: {
          spawn.take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(&spawn.take_damage_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_YELLOW: {
          spawn.take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(&spawn.take_damage_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_RED: {
          spawn.take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(&spawn.take_damage_left_animation, true, false);
          return;
        }
        case SPAWN_TYPE_BOSS: {
          spawn.take_damage_left_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT;
          set_sprite(&spawn.take_damage_left_animation, true, false);
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
          set_sprite(&spawn.take_damage_right_animation, false, true);
          return;
        }
        case SPAWN_TYPE_ORANGE: {
          spawn.take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(&spawn.take_damage_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_YELLOW: {
          spawn.take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(&spawn.take_damage_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_RED: {
          spawn.take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(&spawn.take_damage_right_animation, true, false);
          return;
        }
        case SPAWN_TYPE_BOSS: {
          spawn.take_damage_right_animation.sheet_id = SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT;
          set_sprite(&spawn.take_damage_right_animation, true, false);
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

#undef DEATH_EFFECT_HEIGHT_SCALE
#undef SPAWN_FOLLOW_DISTANCE
#undef CELL_SIZE
#undef CHECK_pSPAWN_COLLISION
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
