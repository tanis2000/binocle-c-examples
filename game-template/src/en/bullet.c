//
// Created by Valerio Santinelli on 24/08/23.
//

#include "bullet.h"
#include "entity.h"
#include "cache.h"
#include "m.h"
#include "binocle_log.h"
#include "level.h"

extern struct game_t game;

entity_t *bullet_new(entity_t *owner) {
  entity_t *e = entity_spawn();
  e->name = "bullet";
  e->owner = owner;
  e->hei = 2;
  e->wid = 4;
  e->pivot_x = 0.5f;
  e->pivot_y = 0.5f;
  e->depth = LAYER_BULLETS;
  e->has_collisions = false;
  entity_load_image(e, "img/bullet.png", 16, 16);
  entity_set_pos_pixel(e, entity_get_center_x(owner), entity_get_center_y(owner));
  e->speed_x = 1.0f;
  e->speed_y = 0.0f;
  e->ang = entity_dir_to_ang(owner);

  e->on_update = bullet_on_update;
  e->on_pre_update = bullet_on_pre_update;

  return e;
}

void bullet_on_pre_update(entity_t *e, float dt) {
  e->dx = cosf(e->ang) * 0.55f * e->speed_x;
  e->dy = sinf(e->ang) * 0.55f * e->speed_y;
}

void bullet_on_update(entity_t *e, float dt) {
  float dist = m_rad_distance(e->ang, 0);
  if (dist <= M_PI_2) {
    e->dir = 1;
  } else {
    e->dir = -1;
  }

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
  }

  if (!level_is_valid(&game.level, e->cx, e->cy) || level_has_collision(&game.level, e->cx, e->cy)) {
    binocle_log_info("wall");
    entity_kill(e);
  }
}