//
// Created by Valerio Santinelli on 01/03/24.
//

#include "bullet.h"
#include "entity.h"
#include "binocle_log.h"

extern game_t game;

physics_component_t bullet_physics_new() {
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

node_component_t bullet_node_new() {
  return (node_component_t){
    .parent = 0,
  };
}

graphics_component_t bullet_graphics_new() {
  return (graphics_component_t){
    .depth = 0,
    .pivot_x = 0.5f,
    .pivot_y = 0.5f,
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

ecs_entity_t bullet_new(ecs_entity_t owner_entity) {
  physics_component_t *owner_physics = ecs_get(game.ecs, owner_entity, physics_component_t);
  ecs_entity_t e = ecs_entity(game.ecs, {.name = "bullet"});

  ecs_set(game.ecs, e, collider_component_t, { 0 });
  collider_component_t *collider = ecs_get_mut(game.ecs, e, collider_component_t);
  collider_component_t coll = {
    .hei = 2,
    .wid = 4,
    .has_collisions = false,
    .radius = 2,
  };
  memcpy(collider, &coll, sizeof(collider_component_t));
  ecs_set(game.ecs, e, projectile_component_t, {
    .speed_x = 1.0f,
    .speed_y = 0.0f,
    .ang = entity_dir_to_ang(owner_physics),
  });
  ecs_set(game.ecs, e, profile_component_t, {
    .name = "bullet"
  });
  ecs_set(game.ecs, e, physics_component_t, {0});
  physics_component_t *p = ecs_get_mut(game.ecs, e, physics_component_t);
  physics_component_t physics = bullet_physics_new();
  memcpy(p, &physics, sizeof(physics_component_t));

  ecs_set(game.ecs, e, graphics_component_t, {0});
  graphics_component_t *g = ecs_get_mut(game.ecs, e, graphics_component_t);
  graphics_component_t graphics = bullet_graphics_new();
  graphics.depth = LAYER_BULLETS;
  memcpy(g, &graphics, sizeof(graphics_component_t));

  entity_load_image(g, "img/bullet.png", 16, 16);

  ecs_set(game.ecs, e, owned_component_t, {
    .owner = owner_entity,
  });

  const ecs_type_t *type = ecs_get_type(game.ecs, e);
  char *str = ecs_type_str(game.ecs, type);
  binocle_log_info(str);

  physics_component_t *owner_p = ecs_get(game.ecs, owner_entity, physics_component_t);
  graphics_component_t *owner_g = ecs_get(game.ecs, owner_entity, graphics_component_t);
  collider_component_t *owner_c = ecs_get(game.ecs, owner_entity, collider_component_t);
  entity_set_pos_pixel(p, entity_get_center_x(owner_p, owner_g, owner_c), entity_get_center_y(owner_p, owner_g, owner_c));

  return e;
}