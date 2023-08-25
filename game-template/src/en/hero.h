//
// Created by Valerio Santinelli on 20/07/23.
//

#ifndef GAME_TEMPLATE_HERO_H
#define GAME_TEMPLATE_HERO_H

#include "types.h"

entity_t *hero_new();
void hero_input_update(entity_t *e, float dt);
void hero_on_update(entity_t *e, float dt);
bool hero_on_is_alive(entity_t *e);
void hero_on_land(entity_t *e);
void hero_shoot(entity_t *e);
void hero_on_damage_taken(entity_t *e, int32_t amount, int32_t direction);

#endif //GAME_TEMPLATE_HERO_H
