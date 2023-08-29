//
// Created by Valerio Santinelli on 24/08/23.
//

#ifndef GAME_TEMPLATE_FX_H
#define GAME_TEMPLATE_FX_H

#include "types.h"

entity_t *fx_new(const char *filename, int32_t frames, float duration);
void fx_on_finish(entity_t *e);

#endif //GAME_TEMPLATE_FX_H
