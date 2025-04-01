
#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

bool app_initialize(void);

bool window_should_close(void);

bool app_update(void);
bool app_render(void);

#endif
