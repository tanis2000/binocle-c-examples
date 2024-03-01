//
// Created by Valerio Santinelli on 28/02/24.
//

#ifndef GAME_TEMPLATE_SYSTEMS_H
#define GAME_TEMPLATE_SYSTEMS_H

#include "types.h"

void system_input_update(ecs_iter_t *it);
void system_animations_update(ecs_iter_t *it);
void system_animation_controller(ecs_iter_t *it);
void system_cooldowns_update(ecs_iter_t *it);
void system_projectile_movement_update(ecs_iter_t *it);

#endif //GAME_TEMPLATE_SYSTEMS_H
