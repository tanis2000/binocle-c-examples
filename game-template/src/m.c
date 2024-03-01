//
// Created by Valerio Santinelli on 24/08/23.
//

#include "m.h"
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

// [-180, 180]
float m_normalize_deg(float a) {
  while (a<-180) {
    a+=360;
  }
  while (a>180) {
    a-=360;
  }
  return a;
}

// [-PI, PI]
float m_normalize_rad(float a) {
  while (a<-M_PI) {
    a+=M_2_PI;
  }
  while (a>M_PI) {
    a-=M_2_PI;
  }
  return a;
}

// returns a-b (normalized)
float m_deg_subtract(float a, float b) {
  return m_normalize_deg(m_normalize_deg(a) - m_normalize_deg(b));
}

float m_deg_distance(float a, float b) {
  return fabsf(m_deg_subtract(a, b));
}

// returns a-b (normalized)
float m_rad_subtract(float a, float b) {
  return m_normalize_rad(m_normalize_rad(a) - m_normalize_rad(b));
}

float m_rad_clamp(float a, float ref_ang, float max_delta) {
  float d = m_rad_subtract(a, ref_ang);
  if (d > max_delta) {
    return ref_ang + max_delta;
  }
  if (d < -max_delta) {
    return ref_ang - max_delta;
  }
  return a;
}

float m_rad_distance(float a, float b) {
  return fabsf(m_rad_subtract(a, b));
}

bool m_rad_close_to(float cur_ang, float target, float max_ang_dist) {
  return m_rad_distance(cur_ang, target) <= fabsf(max_ang_dist);
}

float m_ang_to(float fx, float fy, float tx, float ty) {
  return atan2f(ty-fy, tx-fx);
}

float m_dist_sqr(float ax, float ay, float bx, float by) {
  return (ax-bx)*(ax-bx) + (ay-by)*(ay-by);
}

int32_t m_idist_sqr(float ax, float ay, float bx, float by) {
  return ((int32_t)floorf(ax)-(int32_t)floorf(bx))*((int32_t)floorf(ax)-(int32_t)floorf(bx)) + ((int32_t)floorf(ay)-(int32_t)floorf(by))*((int32_t)floorf(ay)-(int32_t)floorf(by));
}

float m_dist(float ax, float ay, float bx, float by) {
  return sqrtf(m_dist_sqr(ax, ay, bx, by));
}

int m_rand_range_int(int lower, int upper) {
  return (rand() % (upper-lower+1)) + lower;
}

float m_rand_range_float(float lower, float upper) {
  float scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
  return lower + scale * ( upper - lower );      /* [min, max] */
}