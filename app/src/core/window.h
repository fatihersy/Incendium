
#ifndef WINDOW_H
#define WINDOW_H

#include "defines.h"

bool create_window(const char* title);

Vector2 get_screen_size();
Vector2 get_screen_half_size();

void pre_draw();
void post_draw();

#endif