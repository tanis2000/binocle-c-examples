//
// Created by Valerio Santinelli on 18/07/23.
//

#include <stddef.h>
#include <assert.h>
#include "cooldown.h"
#include "binocle_log.h"

void cooldown_system_init(pools_t *pools, size_t pool_size) {
  assert(pools);
  assert(pool_size > 0 && pool_size < BINOCLE_MAX_POOL_SIZE);
  binocle_pool_init(&pools->cooldown_pool, pool_size);
  size_t pool_byte_size = sizeof(cooldown_t) * (size_t)pools->cooldown_pool.size;
  pools->cooldowns = (cooldown_t*) malloc(pool_byte_size);
  memset(pools->cooldowns, 0, pool_byte_size);
}

void cooldown_system_shutdown(pools_t *pools) {
  free(pools->cooldowns);
  pools->cooldowns = NULL;
  binocle_pool_discard(&pools->cooldown_pool);
}

static cooldown_handle_t cooldown_alloc(pools_t *pools) {
  cooldown_handle_t res;
  int slot_index = binocle_pool_alloc_index(&pools->cooldown_pool);
  if (BINOCLE_POOL_INVALID_SLOT_INDEX != slot_index) {
    res.id = binocle_pool_slot_alloc(&pools->cooldown_pool, &pools->cooldowns[slot_index].slot, slot_index);
  } else {
    res.id = BINOCLE_INVALID_ID;
    binocle_log_error("Cooldowns pool exhausted");
  }
  return res;
}

static binocle_resource_state cooldown_init(cooldown_t *cd, const char *name, float duration_seconds) {
  assert(cd && (cd->slot.state == BINOCLE_RESOURCESTATE_ALLOC));
  cd->name = SDL_strdup(name);
  cd->remaining_seconds = duration_seconds;
  cd->total_seconds = duration_seconds;
  cd->func = NULL;
  cd->active = true;
  cd->slot.state = BINOCLE_RESOURCESTATE_VALID;
  return cd->slot.state;
}

static cooldown_t *cooldown_at(const pools_t *pools, uint32_t cd_id) {
  assert(pools && (BINOCLE_INVALID_ID != cd_id));
  int slot_index = binocle_pool_slot_index(cd_id);
  assert((slot_index > BINOCLE_POOL_INVALID_SLOT_INDEX) && (slot_index < pools->cooldown_pool.size));
  return &pools->cooldowns[slot_index];
}

cooldown_handle_t cooldown_make(pools_t *pools, const char *name, float duration_seconds) {
  cooldown_handle_t handle = cooldown_alloc(pools);
  if (handle.id != BINOCLE_INVALID_ID) {
    cooldown_t *cd = cooldown_at(pools, handle.id);
    assert(cd && cd->slot.state == BINOCLE_RESOURCESTATE_ALLOC);
    cooldown_init(cd, name, duration_seconds);
    assert((cd->slot.state == BINOCLE_RESOURCESTATE_VALID) || (cd->slot.state == BINOCLE_RESOURCESTATE_FAILED));
  }
  return handle;
}

cooldown_t *cooldown_get_with_name(pools_t *pools, const char *name) {
  for (int i = 0 ; i < pools->cooldown_pool.size ; i++) {
    cooldown_t *item = &pools->cooldowns[i];
    if (item->slot.state == BINOCLE_RESOURCESTATE_VALID && SDL_strcmp(item->name, name) == 0) {
      return item;
    }
  }
  return NULL;
}

void cooldown_set(pools_t *pools, const char *name, float duration_seconds, void *func) {
  cooldown_t *cd = cooldown_get_with_name(pools, name);
  if (cd == NULL) {
    cooldown_make(pools, name, duration_seconds);
    return;
  }

  cd->remaining_seconds = duration_seconds;
  cd->total_seconds = duration_seconds;
  cd->func = func;
  cd->active = true;
}

void cooldown_unset(pools_t *pools, const char *name) {
  cooldown_t *cd = cooldown_get_with_name(pools, name);
  if (cd != NULL) {
    cd->active = false;
  }
}

void cooldown_system_update(pools_t *pools, float dt) {
  for (size_t i = 0 ; i < pools->cooldown_pool.size ; i++) {
    cooldown_t *cd = &pools->cooldowns[i];
    cd->remaining_seconds -= dt;
    if (cd->remaining_seconds <= 0 && cd->active) {
      void *func = cd->func;
      cooldown_unset(pools, cd->name);
//      if (func != NULL) {
//        func();
//      }
    }
  }
}

bool cooldown_has(pools_t *pools, const char *name) {
  cooldown_t *cd = cooldown_get_with_name(pools, name);
  if (cd != NULL && cd->active && cd->remaining_seconds > 0) {
    return true;
  }
  return false;
}

float cooldown_get(pools_t *pools, const char *name) {
  cooldown_t *cd = cooldown_get_with_name(pools, name);
  if (cd != NULL && cd->active) {
    return cd->remaining_seconds;
  }
  return 0;
}

float cooldown_get_ratio(pools_t *pools, const char *name) {
  if (cooldown_has(pools, name)) {
    cooldown_t *cd = cooldown_get_with_name(pools, name);
    if (cd->remaining_seconds <= 0) {
      return 0;
    }
    return cd->remaining_seconds / cd->total_seconds;
  }
  return 0;
}
