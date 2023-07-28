//
// Created by Valerio Santinelli on 27/07/23.
//

#ifndef GAME_TEMPLATE_L_POINT_H
#define GAME_TEMPLATE_L_POINT_H

#include "types.h"

l_point_t l_point_new();
void l_point_set_level_case(l_point_t *p, int32_t x, int32_t y, float xr, float yr);
float l_point_get_level_x(l_point_t *p);
float l_point_get_level_y(l_point_t *p);
void l_point_set_level_x(l_point_t *p, int32_t x);
void l_point_set_level_y(l_point_t *p, int32_t y);
float l_point_ang_to(l_point_t *self, physics_t *physics, l_point_t *p, int32_t lvl_x, int32_t lvl_y);

#endif //GAME_TEMPLATE_L_POINT_H
