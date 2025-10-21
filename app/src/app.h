
#ifndef GAME_H
#define GAME_H

bool app_initialize(int build_id);

bool window_should_close(void);

bool app_update(void);
bool app_render(void);

#endif
