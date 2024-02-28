//
// Created by Valerio Santinelli on 20/07/23.
//

#include "hero.h"
#include "binocle_log.h"
#include "entity.h"
#include "binocle_input.h"
#include "binocle_audio.h"

extern struct game_t game;

graphics_component_t graphics_new() {
  return (graphics_component_t){
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
    .anim = {
      .state = ANIMATION_ID_HERO_IDLE1
    }
  };
}

collider_component_t collider_new() {
  return (collider_component_t){
    .hei = GRID,
    .wid = GRID,
    .has_collisions = true,
    .radius = GRID * 0.5f,
  };
}

physics_component_t physics_new() {
  return (physics_component_t){
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

node_component_t node_new() {
  return (node_component_t){
    .parent = 0,
  };
}

ecs_entity_t hero_new() {
  ecs_entity_t e = ecs_set_name(game.ecs, 0, "hero");
  ecs_set(game.ecs, e, health_component_t, {
    .destroyed = false,
    .max_health = 100,
    .health = 100,
  });
  ecs_set(game.ecs, e, collider_component_t, {
    .hei = 16,
    .wid = 12,
    .has_collisions = true,
    .radius = 8,
  });
  ecs_add(game.ecs, e, player_component_t);
  ecs_set(game.ecs, e, input_component_t, {0});
  ecs_set(game.ecs, e, profile_component_t, {
    .name = "hero"
  });
  ecs_set(game.ecs, e, physics_component_t, {0});
  physics_component_t *p = ecs_get_mut(game.ecs, e, physics_component_t);
  physics_component_t physics = physics_new();
  memcpy(p, &physics, sizeof(physics_component_t));

  ecs_set(game.ecs, e, graphics_component_t, {0});
  graphics_component_t *g = ecs_get_mut(game.ecs, e, graphics_component_t);
  graphics_component_t graphics = graphics_new();
  graphics.depth = LAYER_HERO;
  memcpy(g, &graphics, sizeof(graphics_component_t));

  entity_load_image(g, "img/hero.png", 32, 32);
  entity_add_animation(g, ANIMATION_ID_HERO_IDLE1, (int[]){0}, 1, 8, true);
  entity_add_animation(g, ANIMATION_ID_HERO_IDLE2, (int[]){1, 2}, 2, 8, true);
  entity_add_animation(g, ANIMATION_ID_HERO_RUN, (int[]){3, 4, 5, 6, 7, 8}, 6, 14, true);
  entity_add_animation(g, ANIMATION_ID_HERO_JUMP_UP, (int[]){9}, 1, 14, true);
  entity_add_animation(g, ANIMATION_ID_HERO_JUMP_DOWN, (int[]){10}, 1, 14, true);
  entity_add_animation(g, ANIMATION_ID_HERO_SHOOT, (int[]){11, 12}, 2, 8, true);
  entity_add_animation(g, ANIMATION_ID_HERO_DEATH, (int[]){13, 14}, 2, 8, false);

  ecs_set(game.ecs, e, state_component_t, {0});

  return e;
}