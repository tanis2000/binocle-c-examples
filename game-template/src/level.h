//
// Created by Valerio Santinelli on 20/07/23.
//

#ifndef GAME_TEMPLATE_LEVEL_H
#define GAME_TEMPLATE_LEVEL_H

#include "types.h"

bool level_has_wall_collision(int32_t cx, int32_t cy);
void level_load_tilemap(level_t *level, const char *filename);
void level_destroy_tilemap(level_t *level);
void level_render(ecs_iter_t *it);

#endif //GAME_TEMPLATE_LEVEL_H
