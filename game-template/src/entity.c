//
// Created by Valerio Santinelli on 19/07/23.
//

#include "entity.h"
#include "binocle_image.h"
#include "backend/binocle_material.h"
#include "binocle_sdl.h"
#include "binocle_log.h"
#include "binocle_ttfont.h"
#include "cache.h"
#include "level.h"
#include "stb_ds.h"

extern struct game_t game;


void draw_entities(ecs_iter_t *it) {
  graphics_t *g = ecs_field(it, graphics_t, 1);
  for (int i = 0 ; i < it->count; i++) {
    if (g->visible && g->sprite != NULL) {
      kmVec2 scale;
      scale.x = g->sprite_scale_x;
      scale.y = g->sprite_scale_y;
      kmAABB2 viewport = binocle_camera_get_viewport(game.gfx.camera);
      binocle_sprite_draw_with_sprite_batch(&game.gfx.sprite_batch, g->sprite, &game.gfx.gd, g->sprite_x, g->sprite_y, &viewport, 0, &scale, &game.gfx.camera, g->depth);
    }
  }
}

void entity_system_init(pools_t *pools, size_t pool_size) {
  assert(pools);
  assert(pool_size > 0 && pool_size < BINOCLE_MAX_POOL_SIZE);
  binocle_pool_init(&pools->entity_pool, pool_size);
  size_t pool_byte_size = sizeof(entity_t) * (size_t)pools->entity_pool.size;
  pools->entities = (entity_t*) malloc(pool_byte_size);
  memset(pools->entities, 0, pool_byte_size);
}

void entity_system_shutdown(pools_t *pools) {
  free(pools->entities);
  pools->entities = NULL;
  binocle_pool_discard(&pools->entity_pool);
}

void entity_system_update(ecs_iter_t *it) {
  physics_t *physics = ecs_field(it, physics_t, 1);
  collider_t *collider = ecs_field(it, collider_t, 2);
  ecs_query_t *q_level = it->ctx;

  ecs_iter_t qit = ecs_query_iter(it->world, q_level);
  level_t *level = NULL;
  if (ecs_query_next(&qit)) {
    level = ecs_field(&qit, level_t, 1);
  }

  for (int i = 0; i < it->count; i++) {
    // X
    float steps = ceilf(fabsf((physics->dx + physics->bdx) * physics->time_mul));
    float step = ((physics->dx + physics->bdx) * physics->time_mul) / steps;
    while (steps > 0) {
      physics->xr += step;
      // Add X collision checks
      entity_on_pre_step_x(it->entities[i], level, physics, collider);
      while (physics->xr > 1) {
        physics->xr -= 1;
        physics->cx += 1;
      }
      while (physics->xr < 0) {
        physics->xr += 1;
        physics->cx -= 1;
      }
      steps -= 1;
    }
    physics->dx *= powf(physics->frict, physics->time_mul);
    physics->bdx *= powf(physics->bump_frict, physics->time_mul);
    if (fabsf(physics->dx) <= 0.0005f * physics->time_mul) {
      physics->dx = 0;
    }
    if (fabsf(physics->bdx) <= 0.0005f * physics->time_mul) {
      physics->bdx = 0;
    }

    // Y
    steps = ceilf(fabsf((physics->dy + physics->bdy) * physics->time_mul));
    step = ((physics->dy + physics->bdy) * physics->time_mul) / steps;
    while (steps > 0) {
      physics->yr += step;
      // Add Y collision checks
      entity_on_pre_step_y(it->entities[i], level, physics, collider);
      while (physics->yr > 1) {
        physics->yr -= 1;
        physics->cy += 1;
      }
      while (physics->yr < 0) {
        physics->yr += 1;
        physics->cy -= 1;
      }
      steps -= 1;
    }
    physics->dy *= powf(physics->frict, physics->time_mul);
    physics->bdy *= powf(physics->bump_frict, physics->time_mul);
    if (fabsf(physics->dy) <= 0.0005f * physics->time_mul) {
      physics->dy = 0;
    }
    if (fabsf(physics->bdy) <= 0.0005f * physics->time_mul) {
      physics->bdy = 0;
    }

//    entity_update_animation(handle, dt);
  }
}

void entity_system_post_update(ecs_iter_t *it) {
  physics_t *physics = ecs_field(it, physics_t, 1);
  graphics_t *graphics = ecs_field(it, graphics_t, 2);

  for (int i = 0; i < it->count; i++) {
    if (graphics->sprite == NULL) {
      continue;
    }

    graphics->sprite_x = (physics->cx + physics->xr) * GRID;
    graphics->sprite_y = (physics->cy + physics->yr) * GRID;
    graphics->sprite_scale_x = physics->dir * graphics->sprite_scale_set_x;
    graphics->sprite_scale_y = graphics->sprite_scale_set_y;
  }
}

static entity_handle_t entity_alloc(pools_t *pools) {
  entity_handle_t res;
  int slot_index = binocle_pool_alloc_index(&pools->entity_pool);
  if (BINOCLE_POOL_INVALID_SLOT_INDEX != slot_index) {
    res.id = binocle_pool_slot_alloc(&pools->entity_pool, &pools->entities[slot_index].slot, slot_index);
  } else {
    res.id = BINOCLE_INVALID_ID;
    binocle_log_error("Entities pool exhausted");
  }
  return res;
}

static binocle_resource_state entity_init(entity_t *en) {
  assert(en && (en->slot.state == BINOCLE_RESOURCESTATE_ALLOC));
  en->cx = 0;
  en->cy = 0;
  en->xr = 0.5f;
  en->yr = 0;

  en->dx = 0;
  en->dy = 0;
  en->bdx = 0;
  en->bdy = 0;

  en->gravity = 0.025f;

  en->frict = 0.82f;
  en->bump_frict = 0.93f;

  en->hei = GRID;
  en->wid = GRID;
  en->radius = GRID * 0.5f;

  en->depth = 0;

  en->pivot_x = 0.5f;
  en->pivot_y = 0;

  en->sprite = NULL;
  en->frames = NULL;

  en->sprite_x = 0;
  en->sprite_y = 0;
  en->sprite_scale_x = 1;
  en->sprite_scale_y = 1;
  en->sprite_scale_set_x = 1;
  en->sprite_scale_set_y = 1;
  en->visible = true;

  en->time_mul = 1;
  en->dir = 1;

  en->destroyed = false;

  en->has_collisions = true;

  en->name = SDL_strdup("entity");

  en->parent = NULL;

  en->slot.state = BINOCLE_RESOURCESTATE_VALID;
  return en->slot.state;
}

static entity_t *entity_at(const pools_t *pools, uint32_t en_id) {
  assert(pools && (BINOCLE_INVALID_ID != en_id));
  int slot_index = binocle_pool_slot_index(en_id);
  assert((slot_index > BINOCLE_POOL_INVALID_SLOT_INDEX) && (slot_index < pools->entity_pool.size));
  return &pools->entities[slot_index];
}

entity_handle_t entity_make(pools_t *pools) {
  entity_handle_t handle = entity_alloc(pools);
  if (handle.id != BINOCLE_INVALID_ID) {
    entity_t *en = entity_at(pools, handle.id);
    assert(en && en->slot.state == BINOCLE_RESOURCESTATE_ALLOC);
    entity_init(en);
    assert((en->slot.state == BINOCLE_RESOURCESTATE_VALID) || (en->slot.state == BINOCLE_RESOURCESTATE_FAILED));
  }
  return handle;
}

void entity_load_image(graphics_t *g, const char *filename, uint32_t width, uint32_t height) {
  sg_image img = cache_load_image(filename);
  binocle_material *mat = binocle_material_new();
  mat->albedo_texture = img;
  mat->shader = game.gfx.default_shader;
  g->sprite = binocle_sprite_from_material(mat);
  sg_image_desc img_info = sg_query_image_desc(img);
  size_t num_frames = (img_info.width / width - 1) * (img_info.height / height - 1);
  arrsetlen(g->frames, num_frames);
  for (size_t x = 0 ; x < img_info.width / width - 1 ; x++) {
    for (size_t y = 0 ; x < img_info.height / height - 1 ; y++) {
      binocle_subtexture frame = binocle_subtexture_with_texture(&img, x * width, y * width, width, height);
      SDL_memcpy(&g->sprite->subtexture, &frame, sizeof(binocle_subtexture));
      arrput(g->frames, frame);
    }
  }
  g->sprite->origin.x = width * g->pivot_x;
  g->sprite->origin.y = width * g->pivot_y;
}

void entity_set_pos_grid(ecs_entity_t en, int32_t x, int32_t y) {
  physics_t *p = ecs_get(game.ecs, en, physics_t);
  p->cx = floor(x);
  p->cy = floor(y);
  p->xr = 0.5f;
  p->yr = 0;
}

void entity_set_pos_pixel(pools_t *pools, entity_handle_t handle, int32_t x, int32_t y) {
  entity_t *entity = entity_at(pools, handle.id);
  entity->cx = x / GRID;
  entity->cy = y / GRID;
  entity->xr = (float)(x - entity->cx * GRID) / (float)GRID;
  entity->yr = (float)(y - entity->cy * GRID) / (float)GRID;
}

void entity_bump(entity_handle_t handle, float x, float y) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  entity->bdx += x;
  entity->bdy += y;
}

void entity_set_speed(pools_t *pools, entity_handle_t handle, float x, float y) {
  entity_t *entity = entity_at(pools, handle.id);
  entity->speed_x = x;
  entity->speed_y = y;
}

void entity_cancel_velocities(entity_handle_t handle) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  entity->dx = 0;
  entity->dy = 0;
  entity->bdx = 0;
  entity->bdy = 0;
}

bool entity_on_ground(entity_handle_t handle) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  // TODO: implement the level and collisions
//  return level_has_wall_collisions(entity->cx, entity->cy-1) && entity->yr == 0 && entity->dy <= 0;
  return false;
}

void entity_on_touch_wall(ecs_entity_t en, int32_t direction) {
}

void entity_on_land(ecs_entity_t en) {
}

void entity_on_pre_step_x(ecs_entity_t en, level_t *level, physics_t *physics, collider_t *collider) {
  // Right collisions
  if (collider->has_collisions && physics->xr > 0.8f && level_has_wall_collision(level, physics->cx+1, physics->cy)) {
    entity_on_touch_wall(en, 1);
    physics->xr = 0.8f;
  }

  // Left collisions
  if (collider->has_collisions && physics->xr < 0.2f && level_has_wall_collision(level, physics->cx-1, physics->cy)) {
    entity_on_touch_wall(en, -1);
    physics->xr = 0.2f;
  }
}

void entity_on_pre_step_y(ecs_entity_t en, level_t *level, physics_t *physics, collider_t *collider) {
  // Down collisions
  if (collider->has_collisions && physics->yr < 0.0f && level_has_wall_collision(level, physics->cx, physics->cy-1)) {
    physics->dy = 0;
    physics->yr = 0;
    physics->bdx *= 0.5f;
    physics->bdx = 0;
    entity_on_land(en);
  }

  // Up collisions
  if (collider->has_collisions && physics->yr > 0.5f && level_has_wall_collision(level, physics->cx, physics->cy+1)) {
    physics->yr = 0.5f;
  }
}

float entity_get_attach_x(physics_t *physics) {
  return (physics->cx + physics->xr) * GRID;
}

float entity_get_attach_y(physics_t *physics) {
  return (physics->cy + physics->yr) * GRID;
}

float entity_get_left(physics_t *physics, graphics_t *graphics, collider_t *collider) {
  return entity_get_attach_x(physics) + (0.0f - graphics->pivot_x) * collider->wid;
}

float entity_get_right(physics_t *physics, graphics_t *graphics, collider_t *collider) {
  return entity_get_attach_x(physics) + (1.0f - graphics->pivot_x) * collider->wid;
}

float entity_get_top(physics_t *physics, graphics_t *graphics, collider_t *collider) {
  return entity_get_attach_y(physics) + (1.0f - graphics->pivot_y) * collider->hei;
}

float entity_get_bottom(physics_t *physics, graphics_t *graphics, collider_t *collider) {
  return entity_get_attach_y(physics) + (0.0f - graphics->pivot_y) * collider->hei;
}

float entity_get_center_x(ecs_entity_t en) {
  physics_t *physics = ecs_get(game.ecs, en, physics_t);
  graphics_t *graphics = ecs_get(game.ecs, en, graphics_t);
  collider_t *collider = ecs_get(game.ecs, en, collider_t);
  return entity_get_attach_x(physics) + (0.5f - graphics->pivot_x) * collider->wid;
}

float entity_get_center_y(ecs_entity_t en) {
  physics_t *physics = ecs_get(game.ecs, en, physics_t);
  graphics_t *graphics = ecs_get(game.ecs, en, graphics_t);
  collider_t *collider = ecs_get(game.ecs, en, collider_t);
  return entity_get_attach_y(physics) + (0.5f - graphics->pivot_y) * collider->hei;
}

bool entity_is_inside(physics_t *physics, graphics_t *graphics, collider_t *collider, float px, float py) {
  return (px >= entity_get_left(physics, graphics, collider)
  && px <= entity_get_right(physics, graphics, collider)
  && py >= entity_get_bottom(physics, graphics, collider)
  && py <= entity_get_top(physics, graphics, collider));
}

/*
void entity_draw_debug(physics_t *physics, graphics_t *graphics, collider_t *collider) {
  if (game.debug_enabled) {
    kmAABB2 viewport = binocle_camera_get_viewport(game.gfx.camera);
    char s[1024];
    sprintf(s, "(%d,%d) (%.0f, %.0f)", physics->cx, physics->cy, entity_get_center_x(physics, graphics, collider), entity_get_center_y(physics, graphics, collider));
    binocle_ttfont_draw_string(&game.gfx.default_font, s, &game.gfx.gd, entity_get_center_x(physics, graphics, collider), entity_get_top(physics, graphics, collider), viewport, binocle_color_white(), &game.gfx.camera, LAYER_TEXT);
    kmAABB2 rect;
    kmVec2 center;
    center.x = entity_get_center_x(physics, graphics, collider);
    center.y = entity_get_center_y(physics, graphics, collider);
    kmAABB2Initialize(&rect, &center, collider->wid, collider->hei, 0);
    binocle_gd_draw_rect(&game.gfx.gd, rect, binocle_color_green_translucent(), viewport, &game.gfx.camera, NULL, LAYER_TEXT + 1);
  }
}
*/
