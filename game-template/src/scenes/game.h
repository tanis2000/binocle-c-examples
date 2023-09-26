//
// Created by Valerio Santinelli on 29/08/23.
//

#ifndef GAME_TEMPLATE_GAME_H
#define GAME_TEMPLATE_GAME_H

#include "types.h"

scene_t *game_new();
void game_free(scene_t *s);
void game_on_pre_update(scene_t *s, float dt);
void game_on_update(scene_t *s, float dt);
void game_on_post_update(scene_t *s, float dt);
#endif //GAME_TEMPLATE_GAME_H
