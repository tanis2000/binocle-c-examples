//
// Created by Valerio Santinelli on 20/07/23.
//

#include "hero.h"
#include "binocle_log.h"
#include "entity.h"
#include "binocle_input.h"
#include "binocle_audio.h"

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
  ecs_add(game.ecs, e, player_t);
  ecs_set(game.ecs, e, physics_t, {0});
  const physics_t *p = ecs_get(game.ecs, e, physics_t);
  physics_t physics = physics_new();
  memcpy(p, &physics, sizeof(physics_t));

  ecs_set(game.ecs, e, graphics_t, {0});
  graphics_t *g = ecs_get_mut(game.ecs, e, graphics_t);
  graphics_t graphics = graphics_new();
  graphics.depth = LAYER_HERO;
  memcpy(g, &graphics, sizeof(graphics_t));

  entity_load_image(g, "img/hero.png", 32, 32);
  entity_add_animation(g, ANIMATION_ID_HERO_IDLE1, (int[]){0}, 1, 8, true);

  return e;
}

void hero_input_update(ecs_iter_t *it) {
  physics_t *physics = ecs_field(it, physics_t, 1);
  health_t *health = ecs_field(it, health_t, 2);
  for (int i = 0; i < it->count; i++) {
    float spd = 2.0f;
    if (entity_is_alive(&health[i]) && (binocle_input_is_key_pressed(&game.input, KEY_LEFT) || binocle_input_is_key_pressed(&game.input, KEY_A))) {
      physics[i].dx += -spd * it->delta_time * physics[i].time_mul;
      physics[i].dir = -1;
    } else if (entity_is_alive(&health[i]) && (binocle_input_is_key_pressed(&game.input, KEY_RIGHT) || binocle_input_is_key_pressed(&game.input, KEY_D))) {
      physics[i].dx += spd * it->delta_time * physics[i].time_mul;
      physics[i].dir = 1;
    }

    if (entity_is_alive(&health[i]) && (binocle_input_is_key_pressed(&game.input, KEY_UP) || binocle_input_is_key_pressed(&game.input, KEY_W))) {
      if (entity_on_ground(it->entities[i])) {
        physics[i].dy = 0.9f;
//        binocle_audio_play_sound(game.sounds.jump);
        // TODO: spawn effect
      }
    }

    if (entity_is_alive(&health[i]) && (binocle_input_is_key_pressed(&game.input, KEY_E) || binocle_input_is_key_pressed(&game.input, KEY_RETURN) || binocle_input_is_key_pressed(&game.input, KEY_SPACE) ||
      binocle_input_is_mouse_down(game.input, MOUSE_LEFT))) {
//      if (!cd.has("shoot")) {
//        // TODO: shoot bullet
//      }
    }
  }
}