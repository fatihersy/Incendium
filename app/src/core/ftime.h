
#ifndef FTIME_H
#define FTIME_H

#include "defines.h"

void time_system_initialize();

timer* get_timer();
void reset_time(elapse_time_type type);

void update_time();





#endif