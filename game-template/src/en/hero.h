//
// Created by Valerio Santinelli on 20/07/23.
//

#ifndef GAME_TEMPLATE_HERO_H
#define GAME_TEMPLATE_HERO_H

#include "types.h"

entity_t *hero_new();
void hero_input_update(entity_t *e, float dt);

#endif //GAME_TEMPLATE_HERO_H
