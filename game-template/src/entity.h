//
// Created by Valerio Santinelli on 19/07/23.
//

#ifndef GAME_TEMPLATE_ENTITY_H
#define GAME_TEMPLATE_ENTITY_H

#include "types.h"

void entity_system_init();

void entity_update(entity_t *e, float dt);

void entity_post_update(entity_t *e, float dt);

entity_t *entity_spawn();

void entity_free(entity_t *e);

void entity_load_image(entity_t *e, const char *filename, uint32_t width, uint32_t height);

void entity_set_pos_grid(entity_t *e, int32_t x, int32_t y);

void entity_set_pos_pixel(entity_t *e, int32_t x, int32_t y);

void entity_set_speed(entity_t *e, float x, float y);

void entity_bump(entity_t *e, float x, float y);

void entity_cancel_velocities(entity_t *e);

void entity_on_pre_step_x(entity_t *e, level_t *level);

void entity_on_pre_step_y(entity_t *e, level_t *level);

float entity_get_center_x(entity_t *e);

float entity_get_center_y(entity_t *e);

bool entity_is_alive(entity_t *e);

bool entity_on_ground(entity_t *e);

void entity_add_animation(entity_t *e, ANIMATION_ID id, int frames[], int frames_count, float period, bool loop);

void entity_play_animation(entity_t *e, ANIMATION_ID id, bool force);

void entity_update_animation(entity_t *e, float dt);

void entity_draw(entity_t *e);

#endif //GAME_TEMPLATE_ENTITY_H
