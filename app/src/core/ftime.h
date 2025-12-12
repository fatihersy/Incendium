
#ifndef FTIME_H
#define FTIME_H

#include "defines.h"

#define RANDOM_TABLE_NUMBER_COUNT 507

bool time_system_initialize(void);
void update_time(void);

[[__nodiscard__]] i32 get_random(i32 min, i32 max);
[[__nodiscard__]] i32 get_random_ssl(i32 min, i32 max);
[[__nodiscard__]] bool get_random_chance_ssl(f32 chance);

void set_ingame_delta_time_multiplier(f32 val);

f32 delta_time_ingame(void);
f64 ftime_get_app_time(void);

std::string get_time_now(std::string format);

#endif
