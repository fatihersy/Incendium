

#ifndef ABILITY_FIREBALL_H
#define ABILITY_FIREBALL_H

#include "defines.h"

ability_fireball get_ability_fireball();

void upgrade_fireball(ability_fireball* _ability);

void update_fireball(ability_fireball* _ability, Character2D owner);
void render_fireball(ability_fireball* _ability);

#endif

