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
#include "binocle_input.h"
#include "binocle_audio.h"
#include "gui/gui.h"

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
#define MAX_CACHED_MUSIC (16)
#define MAX_CACHED_SOUNDS (256)

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

typedef struct cached_music_t {
  binocle_audio_music music;
  const char *filename;
} cached_music_t;

typedef struct cached_sound_t {
  binocle_audio_sound sound;
  const char *filename;
} cached_sound_t;

typedef struct cache_t {
  cached_image_t images[MAX_CACHED_IMAGES];
  size_t images_num;

  cached_music_t music[MAX_CACHED_MUSIC];
  size_t music_num;

  cached_sound_t sounds[MAX_CACHED_SOUNDS];
  size_t sound_num;
} cache_t;

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

typedef struct health_component_t {
  int32_t max_health;
  int32_t health;
  bool destroyed;
} health_component_t;

typedef struct animation_frame_t {
  int *frames;
  int frames_count;
  float period;
  bool loop;
} animation_frame_t;

typedef struct graphics_component_t {
  binocle_sprite *sprite;
  binocle_subtexture *frames;
  animation_frame_t *animations;
  animation_frame_t *animation;
  float animation_timer;
  int animation_frame;
  int frame;

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
} graphics_component_t;

typedef struct physics_component_t {
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
} physics_component_t;

typedef struct collider_component_t {
  float hei;
  float wid;
  float radius;
  bool has_collisions;
} collider_component_t;

typedef struct profile_component_t {
  char *name;
} profile_component_t;

typedef struct node_component_t {
  ecs_entity_t parent;
} node_component_t;

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

typedef struct level_component_t {
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
} level_component_t;

typedef struct l_point_t {
  float cx;
  float cy;
  float xr;
  float yr;
} l_point_t;

typedef struct game_camera_component_t {
  l_point_t raw_focus;
  l_point_t clamped_focus;
  bool clamp_to_level_bounds;
  float dx;
  float dy;

  ecs_entity_t target;
  float target_off_x;
  float target_off_y;
  float dead_zone_pct_x;
  float dead_zone_pct_y;
  float base_frict;
  float bump_off_x;
  float bump_off_y;
  float zoom;
  float tracking_speed;
  float brake_dist_near_bounds;
  float shake_power;
} game_camera_component_t;

typedef struct input_component_t {
  int dummy;
} input_component_t;

/// The main game state
typedef struct game_t {
  bool debug;
  pools_t pools;
  float dt;
  float elapsed_time;
  cache_t cache;
  bool debug_enabled;
  binocle_input input;
  ecs_world_t *ecs;
  /// The level entity
  ecs_entity_t level;
  /// The hero entity
  ecs_entity_t hero;
  /// The game camera entity
  ecs_entity_t game_camera;
  struct {
    binocle_gd gd;
    binocle_camera camera;
    binocle_ttfont *default_font;
    binocle_sprite_batch sprite_batch;
    sg_shader default_shader;
    struct binocle_window *window;
  } gfx;
  binocle_audio audio;
  struct {
    ecs_entity_t update_entities;
    ecs_entity_t post_update_entities;
    ecs_entity_t update_game_camera;
    ecs_entity_t post_update_game_camera;
    ecs_entity_t draw;
    ecs_entity_t draw_level;
    ecs_entity_t input_update;
  } systems;
  struct {
    gui_handle_t debug_gui_handle;
    struct gui_t *debug_gui;
    gui_handle_t game_gui_handle;
    struct gui_t *game_gui;
  } gui;
} game_t;

typedef enum ANIMATION_ID {
  ANIMATION_ID_FX_MAIN = 0,

  ANIMATION_ID_HERO_IDLE1 = 0,
  ANIMATION_ID_HERO_IDLE2 = 1,
  ANIMATION_ID_HERO_RUN = 2,
  ANIMATION_ID_HERO_JUMP_UP = 3,
  ANIMATION_ID_HERO_JUMP_DOWN = 4,
  ANIMATION_ID_HERO_SHOOT = 5,
  ANIMATION_ID_HERO_DEATH = 6,
} ANIMATION_ID;

extern ECS_COMPONENT_DECLARE(health_component_t);
extern ECS_COMPONENT_DECLARE(collider_component_t);
extern ECS_COMPONENT_DECLARE(physics_component_t);
extern ECS_COMPONENT_DECLARE(graphics_component_t);
extern ECS_COMPONENT_DECLARE(profile_component_t);
extern ECS_COMPONENT_DECLARE(node_component_t);
extern ECS_COMPONENT_DECLARE(level_component_t);
extern ECS_COMPONENT_DECLARE(game_camera_component_t);
extern ECS_COMPONENT_DECLARE(input_component_t);

extern ECS_TAG_DECLARE(player_component_t);

#endif //GAME_TEMPLATE_TYPES_H
