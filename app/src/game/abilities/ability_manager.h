
#ifndef ABILITY_MANAGER_H
#define ABILITY_MANAGER_H

#include "defines.h"

void add_ability(ability_type _type, ability_play_system* system);
void upgrade_ability(ability_type _type, ability_play_system* system);

void update_abilities(ability_play_system* system, Character2D owner);
void render_abilities(ability_play_system* system);


#endif