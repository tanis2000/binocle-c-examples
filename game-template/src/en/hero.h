//
// Created by Valerio Santinelli on 20/07/23.
//

#ifndef GAME_TEMPLATE_HERO_H
#define GAME_TEMPLATE_HERO_H

#include "types.h"

ecs_entity_t hero_new();
bool hero_is_shooting(cooldowns_component_t *cds);

#endif //GAME_TEMPLATE_HERO_H
