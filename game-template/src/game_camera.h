//
// Created by Valerio Santinelli on 27/07/23.
//

#ifndef GAME_TEMPLATE_GAME_CAMERA_H
#define GAME_TEMPLATE_GAME_CAMERA_H

#include "types.h"

game_camera_component_t game_camera_new();
void update_game_camera(ecs_iter_t *it);
void post_update_game_camera(ecs_iter_t *it);
void game_camera_track_entity(game_camera_component_t *gc, ecs_entity_t e, bool immediate, float speed);
void game_camera_center_on_target(game_camera_component_t *gc);

#endif //GAME_TEMPLATE_GAME_CAMERA_H
