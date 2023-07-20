//
// Created by Valerio Santinelli on 19/07/23.
//

#include "entity.h"
#include "binocle_image.h"
#include "backend/binocle_material.h"
#include "binocle_array.h"
#include "binocle_sdl.h"
#include "binocle_log.h"
#include "binocle_ttfont.h"
#include "cache.h"
#include "level.h"

extern struct game_t game;

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

void entity_system_pre_update(pools_t *pools, void func(entity_handle_t handle, entity_t *)) {
  for (size_t i = 0 ; i < pools->entity_pool.size ; i++) {
    entity_handle_t handle = {.id = pools->entities[i].slot.id};
    entity_t *entity = &pools->entities[i];
    if (entity->slot.state != BINOCLE_RESOURCESTATE_VALID) {
      continue;
    }
    if (func != NULL) {
      func(handle, entity);
    }
  }
}

void entity_system_update(pools_t *pools, void func(entity_handle_t handle, entity_t *), float dt) {
  for (size_t i = 0 ; i < pools->entity_pool.size ; i++) {
    entity_handle_t handle = {.id = pools->entities[i].slot.id};
    entity_t *entity = &pools->entities[i];
    if (entity->slot.state != BINOCLE_RESOURCESTATE_VALID) {
      continue;
    }

    // X
    float steps = ceilf(fabsf((entity->dx + entity->bdx) * entity->time_mul));
    float step = ((entity->dx + entity->bdx) * entity->time_mul) / steps;
    while (steps > 0) {
      entity->xr += step;
      // Add X collision checks
      entity_on_pre_step_x(handle);
      while (entity->xr > 1) {
        entity->xr -= 1;
        entity->cx += 1;
      }
      while (entity->xr < 0) {
        entity->xr += 1;
        entity->cx -= 1;
      }
      steps -= 1;
    }
    entity->dx *= powf(entity->frict, entity->time_mul);
    entity->bdx *= powf(entity->bump_frict, entity->time_mul);
    if (fabsf(entity->dx) <= 0.0005f * entity->time_mul) {
      entity->dx = 0;
    }
    if (fabsf(entity->bdx) <= 0.0005f * entity->time_mul) {
      entity->bdx = 0;
    }

    // Y
    steps = ceilf(fabsf((entity->dy + entity->bdy) * entity->time_mul));
    step = ((entity->dy + entity->bdy) * entity->time_mul) / steps;
    while (steps > 0) {
      entity->yr += step;
      // Add Y collision checks
      entity_on_pre_step_y(handle);
      while (entity->yr > 1) {
        entity->yr -= 1;
        entity->cy += 1;
      }
      while (entity->yr < 0) {
        entity->yr += 1;
        entity->cy -= 1;
      }
      steps -= 1;
    }
    entity->dy *= powf(entity->frict, entity->time_mul);
    entity->bdy *= powf(entity->bump_frict, entity->time_mul);
    if (fabsf(entity->dy) <= 0.0005f * entity->time_mul) {
      entity->dy = 0;
    }
    if (fabsf(entity->bdy) <= 0.0005f * entity->time_mul) {
      entity->bdy = 0;
    }

//    entity_update_animation(handle, dt);

    if (func != NULL) {
      func(handle, entity);
    }
  }
}

void entity_system_post_update(pools_t *pools, void func(entity_handle_t handle, entity_t *)) {
  for (size_t i = 0 ; i < pools->entity_pool.size ; i++) {
    entity_handle_t handle = {.id = pools->entities[i].slot.id};
    entity_t *entity = &pools->entities[i];
    if (entity->slot.state != BINOCLE_RESOURCESTATE_VALID) {
      continue;
    }

    if (entity->sprite == NULL) {
      continue;
    }

    entity->sprite_x = (entity->cx + entity->xr) * GRID;
    entity->sprite_y = (entity->cy + entity->yr) * GRID;
    entity->sprite_scale_x = entity->dir * entity->sprite_scale_set_x;
    entity->sprite_scale_y = entity->sprite_scale_set_y;

    if (func != NULL) {
      func(handle, entity);
    }
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

void entity_load_image(pools_t *pools, entity_handle_t en, const char *filename, uint32_t width, uint32_t height) {
  entity_t *entity = entity_at(pools, en.id);
  sg_image img = cache_load_image(filename);
  binocle_material *mat = binocle_material_new();
  mat->albedo_texture = img;
  mat->shader = game.gfx.default_shader;
  entity->sprite = binocle_sprite_from_material(mat);
  sg_image_desc img_info = sg_query_image_desc(img);
  size_t num_frames = (img_info.width / width - 1) * (img_info.height / height - 1);
  binocle_array_set_capacity(entity->frames, num_frames);
  for (size_t x = 0 ; x < img_info.width / width - 1 ; x++) {
    for (size_t y = 0 ; x < img_info.height / height - 1 ; y++) {
      binocle_subtexture frame = binocle_subtexture_with_texture(&img, x * width, y * width, width, height);
      SDL_memcpy(&entity->sprite->subtexture, &frame, sizeof(binocle_subtexture));
      binocle_array_push(entity->frames, frame);
    }
  }
  entity->sprite->origin.x = width * entity->pivot_x;
  entity->sprite->origin.y = width * entity->pivot_y;
}

void entity_set_pos_grid(pools_t *pools, entity_handle_t handle, int32_t x, int32_t y) {
  entity_t *entity = entity_at(pools, handle.id);
  entity->cx = floor(x);
  entity->cy = floor(y);
  entity->xr = 0.5f;
  entity->yr = 0;
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

void entity_on_touch_wall(entity_handle_t handle, int32_t direction) {
}

void entity_on_land(entity_handle_t handle) {
}

void entity_on_pre_step_x(entity_handle_t handle) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  // Right collisions
  if (entity->has_collisions && entity->xr > 0.8f && level_has_wall_collision(entity->cx+1, entity->cy)) {
    entity_on_touch_wall(handle, 1);
    entity->xr = 0.8f;
  }

  // Left collisions
  if (entity->has_collisions && entity->xr < 0.2f && level_has_wall_collision(entity->cx-1, entity->cy)) {
    entity_on_touch_wall(handle, -1);
    entity->xr = 0.2f;
  }
}

void entity_on_pre_step_y(entity_handle_t handle) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  // Down collisions
  if (entity->has_collisions && entity->yr < 0.0f && level_has_wall_collision(entity->cx, entity->cy-1)) {
    entity->dy = 0;
    entity->yr = 0;
    entity->bdx *= 0.5f;
    entity->bdx = 0;
    entity_on_land(handle);
  }

  // Up collisions
  if (entity->has_collisions && entity->yr > 0.5f && level_has_wall_collision(entity->cx, entity->cy+1)) {
    entity->yr = 0.5f;
  }
}

float entity_get_attach_x(entity_handle_t handle) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  return (entity->cx + entity->xr) * GRID;
}

float entity_get_attach_y(entity_handle_t handle) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  return (entity->cy + entity->yr) * GRID;
}

float entity_get_left(entity_handle_t handle) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  return entity_get_attach_x(handle) + (0.0f - entity->pivot_x) * entity->wid;
}

float entity_get_right(entity_handle_t handle) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  return entity_get_attach_x(handle) + (1.0f - entity->pivot_x) * entity->wid;
}

float entity_get_top(entity_handle_t handle) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  return entity_get_attach_y(handle) + (1.0f - entity->pivot_y) * entity->hei;
}

float entity_get_bottom(entity_handle_t handle) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  return entity_get_attach_y(handle) + (0.0f - entity->pivot_y) * entity->hei;
}

float entity_get_center_x(entity_handle_t handle) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  return entity_get_attach_x(handle) + (0.5f - entity->pivot_x) * entity->wid;
}

float entity_get_center_y(entity_handle_t handle) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  return entity_get_attach_y(handle) + (0.5f - entity->pivot_y) * entity->hei;
}

bool entity_is_inside(entity_handle_t handle, float px, float py) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  return (px >= entity_get_left(handle) && px <= entity_get_right(handle) && py >= entity_get_bottom(handle) && py <= entity_get_top(handle));
}

void entity_draw_debug(entity_handle_t handle) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  if (game.debug_enabled) {
    kmAABB2 viewport = binocle_camera_get_viewport(game.gfx.camera);
    char s[1024];
    sprintf(s, "(%d,%d) (%.0f, %.0f)", entity->cx, entity->cy, entity_get_center_x(handle), entity_get_center_y(handle));
    binocle_ttfont_draw_string(&game.gfx.default_font, s, &game.gfx.gd, entity_get_center_x(handle), entity_get_top(handle), viewport, binocle_color_white(), &game.gfx.camera, LAYER_TEXT);
    kmAABB2 rect;
    kmVec2 center;
    center.x = entity_get_center_x(handle);
    center.y = entity_get_center_y(handle);
    kmAABB2Initialize(&rect, &center, entity->wid, entity->hei, 0);
    binocle_gd_draw_rect(&game.gfx.gd, rect, binocle_color_green_translucent(), viewport, &game.gfx.camera, NULL, LAYER_TEXT + 1);
  }
}

void entity_draw(entity_handle_t handle) {
  entity_t *entity = entity_at(&game.pools, handle.id);
  if (entity->visible && entity->sprite != NULL) {
    kmVec2 scale;
    scale.x = entity->sprite_scale_x;
    scale.y = entity->sprite_scale_y;
    kmAABB2 viewport = binocle_camera_get_viewport(game.gfx.camera);
    binocle_sprite_draw_with_sprite_batch(&game.gfx.sprite_batch, entity->sprite, &game.gfx.gd, entity->sprite_x, entity->sprite_y, &viewport, 0, &scale, &game.gfx.camera, entity->depth);
  }
}