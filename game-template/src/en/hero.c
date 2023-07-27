//
// Created by Valerio Santinelli on 20/07/23.
//

#include "hero.h"
#include "binocle_log.h"
#include "entity.h"

extern struct game_t game;

graphics_t graphics_new() {
  return (graphics_t){
    .depth = 0,
    .pivot_x = 0.5f,
    .pivot_y = 0,
    .sprite_x = 0,
    .sprite_y = 0,
    .sprite_scale_x = 1,
    .sprite_scale_y = 1,
    .sprite_scale_set_x = 1,
    .sprite_scale_set_y = 1,
    .visible = true,
    .sprite = NULL,
    .frames = NULL,
  };
}

collider_t collider_new() {
  return (collider_t){
    .hei = GRID,
    .wid = GRID,
    .has_collisions = true,
    .radius = GRID * 0.5f,
  };
}

physics_t physics_new() {
  return (physics_t){
    .cx = 0,
    .cy = 0,
    .xr = 0.5f,
    .yr = 0,

    .dx = 0,
    .dy = 0,
    .bdx = 0,
    .bdy = 0,

    .gravity = 0.025f,

    .frict = 0.82f,
    .bump_frict = 0.93f,

    .time_mul = 1,
    .dir = 1,
  };
}

node_t node_new() {
  return (node_t){
    .parent = 0,
  };
}

profile_t profile_new() {
  return (profile_t){
    .name = SDL_strdup("entity"),
  };
}

ecs_entity_t hero_new() {
  ecs_entity_t e = ecs_set_name(game.ecs, 0, "hero");
  ecs_set(game.ecs, e, health_t, {
    .destroyed = false,
    .max_health = 100,
    .health = 100,
  });
  ecs_set(game.ecs, e, collider_t, {
    .hei = 16,
    .wid = 12,
    .has_collisions = true,
    .radius = 8,
  });
  ecs_set(game.ecs, e, physics_t, {0});
  const physics_t *p = ecs_get(game.ecs, e, physics_t);
  physics_t physics = physics_new();
  memcpy(p, &physics, sizeof(physics_t));

  ecs_set(game.ecs, e, graphics_t, {0});
  const graphics_t *g = ecs_get(game.ecs, e, graphics_t);
  graphics_t graphics = graphics_new();
  graphics.depth = LAYER_HERO;
  memcpy(g, &graphics, sizeof(graphics_t));

  entity_load_image(g, "img/hero.png", 32, 32);

  return e;
}