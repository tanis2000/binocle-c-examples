//
// Created by Valerio Santinelli on 20/07/23.
//

#include "hero.h"
#include "entity.h"
#include "cooldown.h"

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
  g->anim.idle1 = ANIMATION_ID_HERO_IDLE1;
  g->anim.idle2 = ANIMATION_ID_HERO_IDLE2;
  g->anim.run = ANIMATION_ID_HERO_RUN;
  g->anim.jump_up = ANIMATION_ID_HERO_JUMP_UP;
  g->anim.jump_down = ANIMATION_ID_HERO_JUMP_DOWN;
  g->anim.shoot = ANIMATION_ID_HERO_SHOOT;
  g->anim.death = ANIMATION_ID_HERO_DEATH;

  ecs_set(game.ecs, e, state_component_t, {0});
  ecs_set(game.ecs, e, cooldowns_component_t, {0});
  cooldowns_component_t *cds = ecs_get_mut(game.ecs, e, cooldowns_component_t);
  cooldown_system_init(&cds->pools, 16);

  return e;
}

bool hero_is_shooting(cooldowns_component_t *cds) {
  return cooldown_has(&cds->pools, "shoot");
}

void hero_on_damage_taken(cooldowns_component_t *cds, health_component_t *health, int32_t amount, int32_t direction) {
  if (cooldown_has(&cds->pools, "hurt")) {
    return;
  }

  health->health -= amount;
  if (health->health <= 0) {
    //game_camera_shake(&game.game_camera, 2, 0.3f);
    //entity_bump(e, -e->dir * 0.4f, -0.15f);
  } else {
    //binocle_audio_play_sound(G.sounds["hurt"]);
    cooldown_set(&cds->pools, "hurt", 0.2f, NULL);
  }
}