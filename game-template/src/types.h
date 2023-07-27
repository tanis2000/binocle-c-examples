//
// Created by Valerio Santinelli on 18/07/23.
//

#ifndef GAME_TEMPLATE_TYPES_H
#define GAME_TEMPLATE_TYPES_H

#include <stdbool.h>
#include "binocle_pool.h"
#include "binocle_sprite.h"
#include "binocle_ttfont.h"
#include "binocle_camera.h"
#include "binocle_gd.h"
#include "flecs.h"
#include "cute_tiled.h"

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
  float dt;
  cache_t cache;
  bool debug_enabled;
  ecs_world_t *ecs;
  /// The level entity
  ecs_entity_t level;
  /// The hero entity
  ecs_entity_t hero;
  struct {
    binocle_gd gd;
    binocle_camera camera;
    binocle_ttfont default_font;
    binocle_sprite_batch sprite_batch;
    sg_shader default_shader;
  } gfx;
  struct {
    ecs_entity_t update_entities;
    ecs_entity_t post_update_entities;
    ecs_entity_t draw;
    ecs_entity_t draw_level;
  } systems;
} game_t;

typedef enum LAYERS {
  LAYER_BG = 1,
  LAYER_MOBS = 2,
  LAYER_HERO = 3,
  LAYER_BULLETS = 4,
  LAYER_FX = 5,
  LAYER_FG = 6,
  LAYER_TEXT = 7
} LAYERS;

typedef struct hero_t {
  entity_handle_t handle;
  int32_t max_health;
  int32_t health;
} hero_t;

typedef struct health_t {
  int32_t max_health;
  int32_t health;
  bool destroyed;
} health_t;

typedef struct graphics_t {
  binocle_sprite *sprite;
  binocle_subtexture *frames;

  int32_t sprite_x;
  int32_t sprite_y;
  float sprite_scale_x;
  float sprite_scale_y;
  float sprite_scale_set_x;
  float sprite_scale_set_y;
  bool visible;
  float depth;

  float pivot_x;
  float pivot_y;
} graphics_t;

typedef struct physics_t {
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

  int32_t dir;
  float time_mul;
} physics_t;

typedef struct collider_t {
  float hei;
  float wid;
  float radius;
  bool has_collisions;
} collider_t;

typedef struct profile_t {
  char *name;
} profile_t;

typedef struct node_t {
  ecs_entity_t parent;
} node_t;

typedef struct tile_t {
  int gid;
  binocle_sprite *sprite;
} tile_t;

typedef enum LEVEL_MARK {
  LEVEL_MARK_NONE = 0,
  LEVEL_MARK_PLATFORM_END_LEFT = 1,
  LEVEL_MARK_PLATFORM_END_RIGHT = 2,
} LEVEL_MARK;

typedef struct spawner_t {
  int32_t cx;
  int32_t cy;
} spawner_t;

typedef struct level_t {
  cute_tiled_map_t *map;
  /// Collision map array
  bool *coll_map;
  binocle_sprite *sprite;
  /// Tiles array
  tile_t *tiles;
  /// Marks map array
  LEVEL_MARK *marks_map;
  /// Array of spawners
  spawner_t *hero_spawners;
} level_t;


extern ECS_COMPONENT_DECLARE(health_t);
extern ECS_COMPONENT_DECLARE(collider_t);
extern ECS_COMPONENT_DECLARE(physics_t);
extern ECS_COMPONENT_DECLARE(graphics_t);
extern ECS_COMPONENT_DECLARE(profile_t);
extern ECS_COMPONENT_DECLARE(node_t);
extern ECS_COMPONENT_DECLARE(level_t);

#endif //GAME_TEMPLATE_TYPES_H
