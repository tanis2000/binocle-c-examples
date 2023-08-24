//
// Created by Valerio Santinelli on 27/07/23.
//

#include "l_point.h"

l_point_t l_point_new() {
  return (l_point_t){0};
}

float l_point_get_cxf(l_point_t *p) {
  return p->cx + p->xr;
}

float l_point_get_cyf(l_point_t *p) {
  return p->cy + p->yr;
}

float l_point_get_level_x(l_point_t *p) {
  return (p->cx + p->xr) * GRID;
}

float l_point_get_level_y(l_point_t *p) {
  return (p->cy + p->yr) * GRID;
}

void l_point_set_level_pixel_x(l_point_t *p, float x) {
  p->cx = floorf(x / GRID);
  p->xr = (float)((int32_t)x % GRID) / GRID;
}

void l_point_set_level_pixel_y(l_point_t *p, float y) {
  p->cy = floorf(y / GRID);
  p->yr = (float)((int32_t)y % GRID) / GRID;
}

void l_point_set_level_pixel_x_y(l_point_t *p, float x, float y) {
  l_point_set_level_pixel_x(p, x);
  l_point_set_level_pixel_y(p, y);
}

void l_point_set_level_x(l_point_t *p, float x) {
  l_point_set_level_pixel_x(p, x);
}

void l_point_set_level_y(l_point_t *p, float x) {
  l_point_set_level_pixel_y(p, x);
}

int32_t l_point_get_level_xi(l_point_t *p) {
  return (int32_t)floorf(l_point_get_level_x(p));
}

int32_t l_point_get_level_yi(l_point_t *p) {
  return (int32_t)floorf(l_point_get_level_y(p));
}

float l_point_get_screen_x(l_point_t *p) {
  return l_point_get_level_x(p) * SCALE;
}

float l_point_get_screen_y(l_point_t *p) {
  return l_point_get_level_y(p) * SCALE;
}

void l_point_set_level_case(l_point_t *p, int32_t x, int32_t y, float xr, float yr) {
  p->cx = x;
  p->cy = y;
  p->xr = xr;
  p->yr = yr;
}

int32_t l_point_dist_case(physics_t *physics, l_point_t *p, int32_t tcx, int32_t tcy, float txr, float tyr) {
  //TODO: implement
  return 0;
}

int32_t l_point_dist_px(physics_t *physics, l_point_t *p, int32_t lvl_x, int32_t lvl_y) {
  //TODO: implement
  return 0;
}

float l_point_ang_to(l_point_t *self, physics_t *physics, l_point_t *p, int32_t lvl_x, int32_t lvl_y) {
  if (physics != NULL) {
    return atan2f((physics->cy + physics->yr) - l_point_get_cyf(self), (physics->cx + physics->xr) - l_point_get_cxf(self));
  } else if (p != NULL) {
    return atan2f(l_point_get_cyf(p) - l_point_get_cyf(self), l_point_get_cxf(p) - l_point_get_cxf(self));
  }  else {
    return atan2f(lvl_y - l_point_get_level_y(self), lvl_x - l_point_get_level_x(self));
  }
}