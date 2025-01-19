#include "ability_manager.h"

#include "ability_fireball.h"

void add_ability(ability_type _type, ability_play_system* system) {
  switch (_type) {
    case ABILITY_TYPE_FIREBALL: { 
      system->abilities[ABILITY_TYPE_FIREBALL].data.fireball = get_ability_fireball();
      system->abilities[ABILITY_TYPE_FIREBALL].type = ABILITY_TYPE_FIREBALL;

      system->abilities[ABILITY_TYPE_FIREBALL].data.fireball.is_active = true;
      break;
    }


    default: break;
  }
}

void upgrade_ability(ability_type _type, ability_play_system* system) {

}

void update_abilities(ability_play_system* system, Character2D owner) {
  for (int i=0; i<ABILITY_TYPE_MAX; ++i) {
    switch (system->abilities[i].type) {
      case ABILITY_TYPE_FIREBALL: {
        update_fireball(&system->abilities[i].data.fireball, owner);
        break;
      }
      case ABILITY_TYPE_RADIATION: break;
      
      default: break;
    }
  }
}

void render_abilities(ability_play_system* system) {

  for (int i=0; i<ABILITY_TYPE_MAX; ++i) {
    switch (system->abilities[i].type) {
      case ABILITY_TYPE_FIREBALL: {
        render_fireball(&system->abilities[i].data.fireball);
        break;
      }
      case ABILITY_TYPE_RADIATION: break;
      
      default: break;
    }
  }
}
