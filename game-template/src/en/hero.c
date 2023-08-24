//
// Created by Valerio Santinelli on 20/07/23.
//

#include "hero.h"
#include "binocle_log.h"
#include "entity.h"
#include "binocle_input.h"
#include "binocle_audio.h"

extern struct game_t game;

collider_t collider_new() {
  return (collider_t){
    .hei = GRID,
    .wid = GRID,
    .has_collisions = true,
    .radius = GRID * 0.5f,
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

entity_t *hero_new() {
  entity_t *e = entity_spawn();
  e->name = "hero";
  e->destroyed = false;
  e->max_health = 100;
  e->health = 100;
  e->hei = 16;
  e->wid = 12;
  e->has_collisions = true;
  e->radius = 8;

  e->cx = 0;
  e->cy = 0;
  e->xr = 0.5f;
  e->yr = 0;

  e->dx = 0;
  e->dy = 0;
  e->bdx = 0;
  e->bdy = 0;

  e->gravity = 0.025f;

  e->frict = 0.82f;
  e->bump_frict = 0.93f;

  e->time_mul = 1;
  e->dir = 1;

  e->depth = 0;
  e->pivot_x = 0.5f;
  e->pivot_y = 0;
  e->sprite_x = 0;
  e->sprite_y = 0;
  e->sprite_scale_x = 1;
  e->sprite_scale_y = 1;
  e->sprite_scale_set_x = 1;
  e->sprite_scale_set_y = 1;
  e->visible = true;
  e->sprite = NULL;
  e->frames = NULL;
  e->depth = LAYER_HERO;

  entity_load_image(e, "img/hero.png", 32, 32);
  entity_add_animation(e, ANIMATION_ID_HERO_IDLE1, (int[]){0}, 1, 8, true);

  return e;
}

void hero_input_update(entity_t *e, float dt) {
    float spd = 2.0f;
    if (entity_is_alive(e) && (binocle_input_is_key_pressed(&game.input, KEY_LEFT) || binocle_input_is_key_pressed(&game.input, KEY_A))) {
      e->dx += -spd * dt * e->time_mul;
      e->dir = -1;
    } else if (entity_is_alive(e) && (binocle_input_is_key_pressed(&game.input, KEY_RIGHT) || binocle_input_is_key_pressed(&game.input, KEY_D))) {
      e->dx += spd * dt * e->time_mul;
      e->dir = 1;
    }

    if (entity_is_alive(e) && (binocle_input_is_key_pressed(&game.input, KEY_UP) || binocle_input_is_key_pressed(&game.input, KEY_W))) {
      if (entity_on_ground(e)) {
        e->dy = 0.9f;
//        binocle_audio_play_sound(game.sounds.jump);
        // TODO: spawn effect
      }
    }

    if (entity_is_alive(e) && (binocle_input_is_key_pressed(&game.input, KEY_E) || binocle_input_is_key_pressed(&game.input, KEY_RETURN) || binocle_input_is_key_pressed(&game.input, KEY_SPACE) ||
      binocle_input_is_mouse_down(game.input, MOUSE_LEFT))) {
//      if (!cd.has("shoot")) {
//        // TODO: shoot bullet
//      }
    }
}