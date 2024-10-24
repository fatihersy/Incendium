

#ifndef ABILITY_H
#define ABILITY_H

#include "defines.h"

typedef enum ability_type {
    fireball,
} ability_type;

typedef struct ability
{
    ability_type type;
    i16 rotation;
} ability;

typedef struct ability_system_state 
{
    ability* abilities;
    i16 amount;

} ability_system_state;

ability_system_state* ability_system_initialize();

void add_ability(ability_system_state* ability_system, ability_type type);

bool update_abilities(ability_system_state* system, Vector2 position);
bool render_abilities(ability_system_state* system, Vector2 position);


#endif

