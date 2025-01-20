#include "ability_manager.h"

#include "ability_fireball.h"

ability_package get_ability(ability_type _type) {
  ability_package package = {0};

  switch (_type) {
    case ABILITY_TYPE_FIREBALL: {
      package.data.fireball = get_ability_fireball();
      break;
    }
    case ABILITY_TYPE_RADIATION: break;
      
    default: break; // TODO: Log unknown type of ability
  }

  return package;
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
      
      default: break; // TODO: Log unknown type of ability
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
      
      default: break; // TODO: Log unknown type of ability
    }
  }
}
