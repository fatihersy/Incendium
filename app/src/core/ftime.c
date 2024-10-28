#include "ftime.h"

timer* _timer;
bool time_system_initialized = false;

void time_system_initialize() {

    _timer = (timer*)malloc(sizeof(timer));

    _timer->total_delay = 1;
    _timer->remaining = _timer->total_delay;
    _timer->type = SALVO_ETA;
    time_system_initialized = true;
}

timer* get_timer()
{
    if (!time_system_initialized) return (timer*){0};
    
    return _timer;
}

void reset_time(elapse_time_type type) {
    if (!time_system_initialized) return;

    _timer->remaining = _timer->total_delay;
}

void update_time() 
{
    _timer->remaining -= GetFrameTime();
    if(_timer->remaining < 0) _timer->remaining = 0;
}
