

#ifndef ABILITY_H
#define ABILITY_H

#include "defines.h"

ability_system_state* ability_system_initialize(actor_type _owner_type);

void add_ability(ability_system_state* ability_system, ability_type type);

bool update_abilities(ability_system_state* system, Vector2 position);
bool render_abilities(ability_system_state* system);


#endif

