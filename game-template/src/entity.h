//
// Created by Valerio Santinelli on 19/07/23.
//

#ifndef GAME_TEMPLATE_ENTITY_H
#define GAME_TEMPLATE_ENTITY_H

#include "types.h"

void entity_system_update(ecs_iter_t *it);
void entity_system_post_update(ecs_iter_t *it);
void entity_system_debug_draw_entities(ecs_iter_t *it);

entity_handle_t entity_make(pools_t *pools);
void entity_load_image(graphics_component_t *g, const char *filename, uint32_t width, uint32_t height);
void entity_set_pos_grid(ecs_entity_t en, int32_t x, int32_t y);
void entity_set_pos_pixel(physics_component_t *p, int32_t x, int32_t y);
void entity_set_speed(pools_t *pools, entity_handle_t handle, float x, float y);
void entity_bump(entity_handle_t handle, float x, float y);
void entity_cancel_velocities(entity_handle_t handle);
void entity_on_pre_step_x(ecs_entity_t en, level_component_t *level, physics_component_t *physics, collider_component_t *collider);
void entity_on_pre_step_y(ecs_entity_t en, level_component_t *level, physics_component_t *physics, collider_component_t *collider);
float entity_get_center_x(physics_component_t *physics, graphics_component_t *graphics, collider_component_t *collider);
float entity_get_center_y(physics_component_t *physics, graphics_component_t *graphics, collider_component_t *collider);
bool entity_is_alive(health_component_t *health);
bool entity_on_ground(ecs_entity_t en);
void entity_add_animation(graphics_component_t *graphics, ANIMATION_ID id, int frames[], int frames_count, float period, bool loop);
void entity_play_animation(graphics_component_t *graphics, ANIMATION_ID id, bool force);
void entity_update_animation(graphics_component_t *graphics, float dt);
float entity_dir_to_ang(physics_component_t *p);
void entity_shoot(ecs_entity_t owner_entity, cooldowns_component_t *cds);
void entity_kill(ecs_entity_t en);
void entity_draw_debug(physics_component_t *physics, graphics_component_t *graphics, collider_component_t *collider);

void draw_entities(ecs_iter_t *it);
#endif //GAME_TEMPLATE_ENTITY_H
