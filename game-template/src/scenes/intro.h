//
// Created by Valerio Santinelli on 29/08/23.
//

#ifndef GAME_TEMPLATE_INTRO_H
#define GAME_TEMPLATE_INTRO_H

#include "types.h"

scene_t *intro_new();
void intro_free(scene_t *s);
void intro_on_update(scene_t *s, float dt);

#endif //GAME_TEMPLATE_INTRO_H
