//
// Created by Valerio Santinelli on 19/07/23.
//

#include "entity.h"
#include "binocle_image.h"
#include "backend/binocle_material.h"
#include "binocle_array.h"
#include "binocle_sdl.h"
#include "binocle_log.h"
#include "cache.h"

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

void entity_system_update(pools_t *pools, void func(entity_handle_t handle, entity_t *)) {
  for (size_t i = 0 ; i < pools->entity_pool.size ; i++) {
    entity_handle_t handle = {.id = pools->entities[i].slot.id};
    entity_t *entity = &pools->entities[i];
    if (entity->slot.state != BINOCLE_RESOURCESTATE_VALID) {
      continue;
    }
    entity->sprite_x = (entity->cx + entity->xr) * GRID;
    entity->sprite_y = (entity->cy + entity->yr) * GRID;
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
  mat->shader = game.default_shader;
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

void entity_set_speed(pools_t *pools, entity_handle_t handle, float x, float y) {
  entity_t *entity = entity_at(pools, handle.id);
  entity->speed_x = x;
  entity->speed_y = y;
}