//
// Created by Valerio Santinelli on 20/07/23.
//

#include "hero.h"
#include "binocle_log.h"
#include "entity.h"
#include "binocle_input.h"
#include "binocle_audio.h"
#include "fx.h"
#include "m.h"
#include "cooldown.h"
#include "game_camera.h"
#include "bullet.h"

extern struct game_t game;

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
  entity_add_animation(e, ANIMATION_ID_HERO_IDLE1, (int[]){0}, 1, 8, true, NULL);
  entity_add_animation(e, ANIMATION_ID_HERO_IDLE2, (int[]){1, 2}, 2, 8, true, NULL);
  entity_add_animation(e, ANIMATION_ID_HERO_RUN, (int[]){3, 4, 5, 6, 7, 8}, 6, 14, true, NULL);
  entity_add_animation(e, ANIMATION_ID_HERO_JUMP_UP, (int[]){9}, 1, 14, true, NULL);
  entity_add_animation(e, ANIMATION_ID_HERO_JUMP_DOWN, (int[]){10}, 1, 14, true, NULL);
  entity_add_animation(e, ANIMATION_ID_HERO_SHOOT, (int[]){11, 12}, 2, 8, true, NULL);
  entity_add_animation(e, ANIMATION_ID_HERO_DEATH, (int[]){13, 14}, 2, 8, false, NULL);

  e->on_update = hero_on_update;
  e->on_is_alive = hero_on_is_alive;
  e->on_land = hero_on_land;
  e->on_damage_taken = hero_on_damage_taken;

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
        entity_t *fx = fx_new("img/jump.png", 6, 0.3f);
        entity_set_pos_pixel(fx, entity_get_center_x(e), entity_get_bottom(e));
      }
    }

    if (entity_is_alive(e) && (binocle_input_is_key_pressed(&game.input, KEY_E) || binocle_input_is_key_pressed(&game.input, KEY_RETURN) || binocle_input_is_key_pressed(&game.input, KEY_SPACE) ||
      binocle_input_is_mouse_down(game.input, MOUSE_LEFT))) {
      if (!cooldown_has(&e->pools, "shoot")) {
        hero_shoot(e);
      }
    }
}

bool hero_is_shooting(entity_t *e) {
  return cooldown_has(&e->pools, "shoot");
}

void hero_on_update(entity_t *e, float dt) {
  if (e->health < 0) {
    entity_play_animation(e, ANIMATION_ID_HERO_DEATH, false);
  } else if (e->dy > 0 && !entity_on_ground(e)) {
    entity_play_animation(e, ANIMATION_ID_HERO_JUMP_UP, false);
  } else if (!entity_on_ground(e)) {
    entity_play_animation(e, ANIMATION_ID_HERO_JUMP_DOWN, false);
  } else if (entity_on_ground(e) && e->dx != 0) {
    entity_play_animation(e, ANIMATION_ID_HERO_RUN, false);
  } else if (hero_is_shooting(e)) {
    entity_play_animation(e, ANIMATION_ID_HERO_SHOOT, false);
  } else {
    if (m_rand_range_float(0, 1) < 0.4f) {
      entity_play_animation(e, ANIMATION_ID_HERO_IDLE2, false);
    } else {
      entity_play_animation(e, ANIMATION_ID_HERO_IDLE1, false);
    }
  }

}

bool hero_on_is_alive(entity_t *e) {
  return (!e->destroyed && e->health > 0);
}

void hero_on_land(entity_t *e) {
  entity_t *fx = fx_new("img/land.png", 6, 0.3f);
  entity_set_pos_pixel(fx, entity_get_center_x(e), entity_get_bottom(e));
}

void hero_shoot(entity_t *e) {
  entity_t *bullet = bullet_new(e);
  cooldown_set(&e->pools, "shoot", 0.15f, NULL);
//  binocle_audio_play_sound(G.sounds["shoot"]);
}

void hero_on_damage_taken(entity_t *e, int32_t amount, int32_t direction) {
  if (cooldown_has(&e->pools, "hurt")) {
    return;
  }

  e->health -= amount;
  if (e->health <= 0) {
    entity_play_animation(e, ANIMATION_ID_HERO_DEATH, false);
    game_camera_shake(&game.game_camera, 2, 0.3f);
    entity_bump(e, -e->dir * 0.4f, -0.15f);
  } else {
    //binocle_audio_play_sound(G.sounds["hurt"]);
    cooldown_set(&game.pools, "hurt", 0.2f, NULL);
  }
}