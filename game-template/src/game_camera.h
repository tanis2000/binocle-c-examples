//
// Created by Valerio Santinelli on 27/07/23.
//

#ifndef GAME_TEMPLATE_GAME_CAMERA_H
#define GAME_TEMPLATE_GAME_CAMERA_H

#include "types.h"

game_camera_t game_camera_new();

void game_camera_update(game_camera_t *gc);

void game_camera_post_update(game_camera_t *gc);

void game_camera_track_entity(game_camera_t *gc, entity_t *e, bool immediate, float speed);

void game_camera_center_on_target(game_camera_t *gc);

#endif //GAME_TEMPLATE_GAME_CAMERA_H
