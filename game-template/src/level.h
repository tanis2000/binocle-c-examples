//
// Created by Valerio Santinelli on 20/07/23.
//

#ifndef GAME_TEMPLATE_LEVEL_H
#define GAME_TEMPLATE_LEVEL_H

#include "types.h"

bool level_has_wall_collision(level_component_t *level, int32_t cx, int32_t cy);
void level_load_tilemap(level_component_t *level, const char *filename);
void level_destroy_tilemap(level_component_t *level);
void level_render(ecs_iter_t *it);
spawner_t *level_get_hero_spawner(level_component_t *level);
int level_get_px_wid(level_component_t *level);
int level_get_px_hei(level_component_t *level);
bool level_is_valid(const level_component_t *level, int cx, int cy);
bool level_has_collision(const level_component_t *level, int32_t cx, int32_t cy);

#endif //GAME_TEMPLATE_LEVEL_H
