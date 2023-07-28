//
// Created by Valerio Santinelli on 19/07/23.
//

#ifndef GAME_TEMPLATE_ENTITY_H
#define GAME_TEMPLATE_ENTITY_H

#include "types.h"

void entity_system_init(pools_t *pools, size_t pool_size);
void entity_system_shutdown(pools_t *pools);
void entity_system_update(ecs_iter_t *it);
void entity_system_post_update(ecs_iter_t *it);

entity_handle_t entity_make(pools_t *pools);
void entity_load_image(graphics_t *g, const char *filename, uint32_t width, uint32_t height);
void entity_set_pos_grid(ecs_entity_t en, int32_t x, int32_t y);
void entity_set_pos_pixel(pools_t *pools, entity_handle_t handle, int32_t x, int32_t y);
void entity_set_speed(pools_t *pools, entity_handle_t handle, float x, float y);
void entity_bump(entity_handle_t handle, float x, float y);
void entity_cancel_velocities(entity_handle_t handle);
void entity_on_pre_step_x(ecs_entity_t en, level_t *level, physics_t *physics, collider_t *collider);
void entity_on_pre_step_y(ecs_entity_t en, level_t *level, physics_t *physics, collider_t *collider);
float entity_get_center_x(ecs_entity_t en);
float entity_get_center_y(ecs_entity_t en);

void draw_entities(ecs_iter_t *it);
#endif //GAME_TEMPLATE_ENTITY_H
