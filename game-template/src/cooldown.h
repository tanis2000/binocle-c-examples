//
// Created by Valerio Santinelli on 18/07/23.
//

#ifndef GAME_TEMPLATE_COOLDOWN_H
#define GAME_TEMPLATE_COOLDOWN_H

#include "types.h"

void cooldown_system_init(pools_t *pools, size_t pool_size);
void cooldown_system_shutdown(pools_t *pools);
void cooldown_system_update(pools_t *pools, float dt);

cooldown_t *cooldown_get_with_name(pools_t *pools, const char *name);
void cooldown_set(pools_t *pools, const char *name, float duration_seconds, void *func);
void cooldown_unset(pools_t *pools, const char *name);
bool cooldown_has(pools_t *pools, const char *name);
float cooldown_get(pools_t *pools, const char *name);

#endif //GAME_TEMPLATE_COOLDOWN_H
