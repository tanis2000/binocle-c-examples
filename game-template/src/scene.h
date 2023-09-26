//
// Created by Valerio Santinelli on 29/08/23.
//

#ifndef GAME_TEMPLATE_SCENE_H
#define GAME_TEMPLATE_SCENE_H

#include "types.h"

void scene_system_init();
scene_t *scene_new(scene_t *parent);
void scene_free(scene_t *e);
void scene_pre_update(scene_t *s, float dt);
void scene_update(scene_t *s, float dt);
void scene_post_update(scene_t *s, float dt);
void scene_pause(scene_t *s);
void scene_resume(scene_t *s);
#endif //GAME_TEMPLATE_SCENE_H
