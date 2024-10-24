

#ifndef SPAWN_H
#define SPAWN_H

#include "defines.h"


typedef struct Character2D {
    unsigned int texId;
    u32 ID;
    bool initialized;

    Rectangle collision;
    Vector2 position;
    u16 health;
    u16 speed;
} Character2D;

bool spawn_system_initialize();
bool spawn_character(Character2D character, actor_type type);

Character2D get_character(u32 ID);

bool update_spawns();
bool render_spawns();


#endif
