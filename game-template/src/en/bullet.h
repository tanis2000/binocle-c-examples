//
// Created by Valerio Santinelli on 24/08/23.
//

#ifndef GAME_TEMPLATE_BULLET_H
#define GAME_TEMPLATE_BULLET_H

#include "types.h"

entity_t *bullet_new(entity_t *owner);
void bullet_on_update(entity_t *e, float dt);
void bullet_on_pre_update(entity_t *e, float dt);

#endif //GAME_TEMPLATE_BULLET_H
