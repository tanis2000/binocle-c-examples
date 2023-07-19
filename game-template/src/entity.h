//
// Created by Valerio Santinelli on 19/07/23.
//

#ifndef GAME_TEMPLATE_ENTITY_H
#define GAME_TEMPLATE_ENTITY_H

#include "types.h"

void entity_system_init(pools_t *pools, size_t pool_size);
void entity_system_shutdown(pools_t *pools);

entity_handle_t entity_make(pools_t *pools);
void entity_load_image(pools_t *pools, entity_handle_t en, const char *filename, uint32_t width, uint32_t height);
void entity_set_pos_grid(pools_t *pools, entity_handle_t handle, int32_t x, int32_t y);
void entity_set_pos_pixel(pools_t *pools, entity_handle_t handle, int32_t x, int32_t y);
void entity_set_speed(pools_t *pools, entity_handle_t handle, float x, float y);
void entity_system_update(pools_t *pools, void func(entity_handle_t handle, entity_t *));

#endif //GAME_TEMPLATE_ENTITY_H