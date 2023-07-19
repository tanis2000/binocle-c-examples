//
// Created by Valerio Santinelli on 19/07/23.
//

#ifndef GAME_TEMPLATE_ENTITY_H
#define GAME_TEMPLATE_ENTITY_H

#include "types.h"

void entity_system_init(pools_t *pools, size_t pool_size);
void entity_system_shutdown(pools_t *pools);

entity_handle_t entity_make(pools_t *pools);

#endif //GAME_TEMPLATE_ENTITY_H
