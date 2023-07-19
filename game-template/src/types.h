//
// Created by Valerio Santinelli on 18/07/23.
//

#ifndef GAME_TEMPLATE_TYPES_H
#define GAME_TEMPLATE_TYPES_H

#include <stdbool.h>
#include "binocle_pool.h"
#include "binocle_sprite.h"

// TODO: particles
// TODO: entities
// TODO: cooldown
// TODO: gui with imgui ... meh... both debug and in-game
// TODO: scene graph


#define FPS (60)
#define GRID (16)
#define SCALE (2)
#define DESIGN_WIDTH (320)
#define DESIGN_HEIGHT (240)
#define MAX_CACHED_IMAGES (256)

/// The handle of a cooldown
typedef struct cooldown_handle_t {
  int id;
} cooldown_handle_t;

/// The definition of the cooldown item
typedef struct cooldown_t {
  binocle_slot_t slot;
  char *name;
  float total_seconds;
  float remaining_seconds;
  void *func;
  bool active;
} cooldown_t;

typedef struct entity_handle_t {
  int id;
} entity_handle_t;

typedef struct entity_t {
  binocle_slot_t slot;
  struct entity_t *parent;
  int32_t cx;
  int32_t cy;
  float xr;
  float yr;

  float dx;
  float dy;
  float bdx;
  float bdy;

  float gravity;

  float frict;
  float bump_frict;

  float hei;
  float wid;
  float radius;

  float depth;

  float pivot_x;
  float pivot_y;

  binocle_sprite *sprite;
  binocle_subtexture *frames;

  int32_t sprite_x;
  int32_t sprite_y;
  float sprite_scale_x;
  float sprite_scale_y;
  float sprite_scale_set_x;
  float sprite_scale_set_y;
  bool visible;

  float time_mul;
  int32_t dir;

  bool destroyed;

  bool has_collisions;
  char *name;

  // Game specific data
  float speed_x;
  float speed_y;
} entity_t;

/// Container for all the arrays (pools) of managed objects
typedef struct pools_t {
  binocle_pool_t cooldown_pool;
  cooldown_t *cooldowns;

  binocle_pool_t entity_pool;
  entity_t *entities;
} pools_t;

typedef struct cached_image_t {
  sg_image img;
  const char *filename;
} cached_image_t;

typedef struct cache_t {
  cached_image_t images[MAX_CACHED_IMAGES];
  size_t images_num;
} cache_t;

/// The main game state
typedef struct game_t {
  pools_t pools;
  sg_shader default_shader;
  float dt;
  cache_t cache;
} game_t;

#endif //GAME_TEMPLATE_TYPES_H
