
#ifndef COLLISION_H
#define COLLISION_H

#include "defines.h"

typedef struct collision 
{
    unsigned int Id;
    bool is_active;

    actor_type type;
    Rectangle rect;

} collision;

typedef struct collision_system
{
    collision* enemy_collisions;
    collision* enemy_projectiles;
    collision* player_projectiles;
    collision player_collision;

    i16 enemy_count;
    i16 enemy_projectiles_count;
    i16 player_projectiles_count;

} collision_system;

bool collision_system_initialize();

void add_collision(Rectangle rect, actor_type type);
void remove_collision(collision coll, unsigned int id);

bool update_collision();
bool render_collision();


#endif