#include "ftime.h"
#include "raylib.h"
#include "core/fmemory.h"

#include <openssl/rand.h>
#include <openssl/err.h>

const u32 random_table[RANDOM_TABLE_NUMBER_COUNT] = {
   843, 5141, 4902,  309, 6743,   95, 7384, 8173, 3152, 8213,  375, 9200, 2725, 2151, 6530,  736,
  5462, 3940, 3193, 5436, 8707, 1550, 5675, 3300, 5115, 9668, 8666, 4327, 4688, 950,  4821, 5863,
  7425, 5395, 1484, 3219, 3325, 3005, 3966, 8427, 9734, 1123, 6957, 9454, 9307, 2405, 3112, 3646,
  2044, 6850, 9775, 8346, 5289, 1804, 2257, 6209, 1697, 2578, 5248, 4581, 1270, 3860, 8747, 8773,
  4007, 5649, 8066, 3473, 7252, 3259, 7893, 1336, 6316, 1377, 3539, 2364, 5889, 2659, 7038, 3900,
  5970, 7104, 6357, 8600, 1871, 4714, 6183, 6463, 8279, 2832, 1830, 7491, 7852, 4114, 5329, 9027,
   589, 8559, 4648, 3727, 3834, 3513, 4474, 1591, 2898, 4287, 7465, 2618, 2471, 5569, 3620, 4154,
  2552, 7358, 2939, 8854, 8452, 4968, 2765, 6718, 4861, 3086, 5756, 7745, 4434, 4368, 1911, 1937,
  7171, 8813, 1230, 6637,  416, 6423, 1056, 4500, 6824, 4541, 4047, 2872, 6397, 5823,  202, 7064,
  9134,  268, 9521, 1764, 5034, 5222, 9347, 9627, 1443, 5996, 2338, 7999, 1016, 7278, 8493, 2191,
  3753, 1723, 7812, 6891, 6998, 6677, 7638, 4754, 3406, 4795,  629, 5782, 2979, 6077, 6784, 7318,
  5716,  522, 6103, 2018, 8961, 8132, 5929, 9882, 8025, 6250, 8920,  909, 7598, 7532, 5075, 2445,
  7679, 1977, 4394, 7145, 3580, 6931, 4220, 5009, 9988, 7705, 7211, 6036, 9561, 8987, 3366, 7572,
  2298, 3432, 2685, 4928, 5543, 8386, 2511, 2791, 4607, 6504, 5502, 1163, 4180, 7786, 1657, 5355,
  4261, 2231, 8320,   55,  162, 9841,  802, 7918, 6570, 7959, 3793, 6290, 6143, 9241, 9948,  482,
  3522, 9831,  369, 9622, 5058,  212, 7710, 2113, 9926, 1221, 6665, 4795, 2488, 2554, 3833, 5550, 
  5349, 4204, 2072, 2705, 4035, 9208, 4741, 4177, 2826, 1522, 5773, 6052, 7066,  361, 8486, 2242, 
  8850, 5042, 4912, 4987, 3558, 5515, 9071, 3802, 4614, 6256, 8720, 4868, 7750, 9022, 5405, 2756, 
  7348, 2202, 3941,  416, 5463, 3771, 4582, 1039, 2621, 3701, 3980, 6366, 5234, 9300,  200, 8489, 
  7899, 9942, 3904, 9368, 1147, 4376,    5, 5934,  576,  184, 5974, 4050, 5458,  665, 1691, 2317, 
  9118, 9279, 6890, 9481, 4574, 8514, 5355, 3629, 4806, 2740, 5209, 9231, 5173, 9507, 6852, 5572, 
  2755, 8487, 1610, 9481, 9215, 5097, 9654, 7774, 5684, 2709, 5841, 4125,  669, 6074, 7777,  812, 
  1010, 3242, 8928, 2146, 2428, 9635, 1061, 3369, 9441, 7967, 3170, 1764, 8001,  271, 8169, 9126, 
  8974, 7863, 5886, 7666, 6699, 5427, 1200, 4904, 7119, 5455, 1436, 7155, 6586, 1976, 2312,  249, 
  2694, 3463, 3870, 2296, 1405, 2940, 8674, 5206, 1140, 2068,  128, 3284, 5605, 9427, 6358,  802, 
  2703, 2922, 5630, 3052, 9131, 5184, 6995, 7527, 9179, 5273, 8141, 4392, 2784, 7547, 5425, 7991, 
  2689, 5045, 5342, 9912, 1294, 7194, 7069, 1079, 1350, 6093, 2921, 7441, 1152, 5242, 5985, 3229, 
   622, 5031,  495, 6014, 3333,  943, 2071, 7669, 7661, 3296, 6732, 9490,  108, 3301, 1453, 7479, 
  2025, 7222, 2988,  201, 7423,  599, 4298, 7823, 7682, 2693, 7965, 1597, 6704, 4209, 2066,  497, 
  5643,  626, 1805, 7390, 2532, 3475,  116, 6952, 8635, 9208, 2955, 1801, 1925,  999, 7106, 4373, 
  2332, 7709, 8167, 2508, 7729, 3693, 5773, 5203, 2899, 1275,  680, 5202, 4071, 8238, 6036, 1308, 
  2510, 7444,  757, 5826, 5364, 5484, 9060, 5822, 5789, 8498, 8639
};

typedef struct time_system_state {
  u16 rand_start_index;
  u16 rand_ind;

  f32 ingame_delta_time_multiplier;

  time_system_state(void) {
    this->rand_start_index = 0u;
    this->rand_ind = 0u;
    this->ingame_delta_time_multiplier = 0.f;
  }
} time_system_state;

static time_system_state * state = nullptr;

bool time_system_initialize(void) {
  if (state and state != nullptr) {
    return true;
  }
  state = (time_system_state *)allocate_memory_linear(sizeof(time_system_state), true);
  if (not state or state == nullptr) {
    return false;
  }
  *state = time_system_state();

  return true;
}
void update_time(void) {
  if(state == nullptr) {
    return;
  }
}

[[__nodiscard__]] i32 get_random(i32 min, i32 max) {
  if (not state or state == nullptr || RANDOM_TABLE_NUMBER_COUNT == 0) {
    return INT32_MAX;
  }

  if (min > max) {
    return INT32_MAX;
  }

  if (static_cast<std::int64_t>(max) - min + 1 > INT32_MAX) {
    return INT32_MAX;
  }
  if (state->rand_ind >= RANDOM_TABLE_NUMBER_COUNT) {
    state->rand_ind = 0;
  } else {
    state->rand_ind++;
  }

  const i32 ind = (state->rand_start_index + state->rand_ind) % RANDOM_TABLE_NUMBER_COUNT;
  if (ind < 0 or ind >= RANDOM_TABLE_NUMBER_COUNT) {
    return INT32_MAX;
  }

  i32 rnd = random_table[ind];
  rnd = (rnd % (max - min + 1)) + min;

  return rnd;
}

[[__nodiscard__]] i32 get_random_ssl(i32 min, i32 max) {
  if (min > max) {
    return INT32_MAX;
  }
  u64 range = static_cast<u64>(max) - static_cast<u64>(min) + 1;
  
  if (range == 0 or range > static_cast<u64>(INT32_MAX)) {
    return INT32_MAX;
  }
  u64 randomValue;
  if (RAND_bytes(reinterpret_cast<unsigned char*>(&randomValue), sizeof(randomValue)) != 1) {
    return get_random(min, max); 
  }
  u64 result = randomValue % range;
  
  return static_cast<i32>(result + min);
}

[[__nodiscard__]] bool get_random_chance_ssl(f32 chance) {
  if (chance < 0.0f) return false;
  if (chance >= 1.0f) return true;

  u64 randomValue;
  if (RAND_bytes(reinterpret_cast<unsigned char*>(&randomValue), sizeof(randomValue)) != 1) {
    i32 _chance = static_cast<i32>(chance * 100.f);
    return _chance > get_random(0, 100); 
  }
  f64 generatedChance = static_cast<f64>(randomValue) / static_cast<f64>(std::numeric_limits<u64>::max());
  return generatedChance < static_cast<f64>(chance);
}

void set_ingame_delta_time_multiplier(f32 val) {
  if (not state or state == nullptr) {
    return;
  }
  f32 v = val;
  if (v < 0.1f) { v = 0.1f; }
  if (v > 1.0f) { v = 1.0f; }
  state->ingame_delta_time_multiplier = v;
}

f32 delta_time_ingame(void) {
  if (not state or state == nullptr) {
    return 0.f;
  }

  #ifdef _DEBUG 
    const f32 delta = GetFrameTime() * state->ingame_delta_time_multiplier;
    constexpr f32 max_framerate = 75.f;
    constexpr f32 max_delta = 1 / max_framerate;

    if (delta > max_delta) {
      return max_delta;
    }
    else return delta;
    
  #else
    return GetFrameTime() * state->ingame_delta_time_multiplier;
  #endif
}
