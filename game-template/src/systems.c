//
// Created by Valerio Santinelli on 28/02/24.
//

#include "systems.h"
#include "types.h"
#include "entity.h"
#include "flecs.h"
#include "en/hero.h"
#include "m.h"
#include "cooldown.h"
#include "binocle_log.h"
#include "level.h"

extern struct game_t game;

void system_input_update(ecs_iter_t *it) {
  physics_component_t *physics = ecs_field(it, physics_component_t, 1);
  health_component_t *health = ecs_field(it, health_component_t, 2);
  input_component_t *inputs = ecs_field(it, input_component_t, 3);
  cooldowns_component_t *cooldowns = ecs_field(it, cooldowns_component_t, 4);
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
      if (!hero_is_shooting(&cooldowns[i])) {
        entity_shoot(it->entities[i], &cooldowns[i]);
      }
    }
  }
}

void system_animation_controller(ecs_iter_t *it) {
  graphics_component_t *graphics = ecs_field(it, graphics_component_t, 1);
  health_component_t *healths = ecs_field(it, health_component_t, 2);
  physics_component_t *physics = ecs_field(it, physics_component_t, 3);
  cooldowns_component_t *cooldowns = ecs_field(it, cooldowns_component_t, 4);
  for (int i = 0; i < it->count; i++) {
    graphics_component_t *g = &graphics[i];
    health_component_t *h = &healths[i];
    physics_component_t *ph = &physics[i];
    cooldowns_component_t *cds = &cooldowns[i];
    ecs_entity_t e = it->entities[i];

    if (h->health < 0) {
      entity_play_animation(g, g->anim.death, false);
    } else if (ph->dy > 0 && !entity_on_ground(e)) {
      entity_play_animation(g, g->anim.jump_up, false);
    } else if (!entity_on_ground(e)) {
      entity_play_animation(g, g->anim.jump_down, false);
    } else if (entity_on_ground(e) && ph->dx != 0) {
      entity_play_animation(g, g->anim.run, false);
    } else if (hero_is_shooting(cds)) {
      entity_play_animation(g, g->anim.shoot, false);
    } else {
      if (m_rand_range_float(0, 1) < 0.4f) {
        entity_play_animation(g, g->anim.idle2, false);
      } else {
        entity_play_animation(g, g->anim.idle1, false);
      }
    }
  }
}

void system_animations_update(ecs_iter_t *it) {
  graphics_component_t *graphics = ecs_field(it, graphics_component_t, 1);
  for (int i = 0; i < it->count; i++) {
    graphics_component_t *g = &graphics[i];
    entity_update_animation(g, it->delta_time);
  }
}

void system_cooldowns_update(ecs_iter_t *it) {
  cooldowns_component_t *cooldowns = ecs_field(it, cooldowns_component_t, 1);
  for (int i = 0; i < it->count; i++) {
    cooldowns_component_t *cds = &cooldowns[i];
    cooldown_system_update(&cds->pools, it->delta_time);
  }
}

void system_projectile_movement_update(ecs_iter_t *it) {
  projectile_component_t *projectiles = ecs_field(it, projectile_component_t, 1);
  physics_component_t *physics = ecs_field(it, physics_component_t, 2);
  for (int i = 0; i < it->count; i++) {
    projectile_component_t *proj = &projectiles[i];
    physics_component_t *p = &physics[i];

    p->dx = cosf(proj->ang) * 0.55f * proj->speed_x;
    p->dy = sinf(proj->ang) * 0.55f * proj->speed_y;

    float dist = m_rad_distance(proj->ang, 0);
    if (dist <= M_PI_2) {
      p->dir = 1;
    } else {
      p->dir = -1;
    }
/*
    for (int i = 0 ; i < game.num_entities ; i++) {
      entity_t *other = &game.entities[i];
      if (e != other &&
          entity_is_alive(other) &&
          entity_get_center_x(e) >= entity_get_center_x(other) - other->radius &&
          entity_get_center_x(e) <= entity_get_center_x(other) + other->radius &&
          entity_get_bottom(e) >= entity_get_bottom(other) &&
          entity_get_bottom(e) <= entity_get_bottom(other) + other->hei) {
        binocle_log_info("hit");
        entity_kill(e);
        if (other->on_damage_taken != NULL) {
          other->on_damage_taken(other, 1, e->dir);
        }
      }
    }*/

    const level_component_t *level = ecs_get(game.ecs, game.level, level_component_t);
    if (!level_is_valid(level, p->cx, p->cy) || level_has_collision(level, p->cx, p->cy)) {
      binocle_log_info("wall");
      entity_kill(it->entities[i]);
    }

    ecs_modified(game.ecs, it->entities[i], physics_component_t);
  }
}