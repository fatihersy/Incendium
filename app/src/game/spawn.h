

#ifndef SPAWN_H
#define SPAWN_H

#include "defines.h"

// This is the maximum amount of elements (quads) per batch
// NOTE: This value is defined in [rlgl] module and can be changed there
#define MAX_SPAWN_COUNT 50000

typedef struct Character2D {
    unsigned int texId;
    u32 ID;
    bool initialized;

    Vector2 position;
    u16 health;
    u16 speed;
} Character2D;

bool spawn_system_initialize();
bool spawn_character(Character2D character);

Character2D get_character(u32 ID);

bool update_spawns();
bool render_spawns();


#endif
