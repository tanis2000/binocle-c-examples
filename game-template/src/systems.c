//
// Created by Valerio Santinelli on 28/02/24.
//

#include "systems.h"
#include "types.h"
#include "entity.h"

extern struct game_t game;

void system_input_update(ecs_iter_t *it) {
  physics_component_t *physics = ecs_field(it, physics_component_t, 1);
  health_component_t *health = ecs_field(it, health_component_t, 2);
  for (int i = 0; i < it->count; i++) {
    float spd = 2.0f;
    if (entity_is_alive(&health[i]) && (binocle_input_is_key_pressed(&game.input, KEY_LEFT) || binocle_input_is_key_pressed(&game.input, KEY_A))) {
      physics[i].dx += -spd * it->delta_time * physics[i].time_mul;
      physics[i].dir = -1;
    } else if (entity_is_alive(&health[i]) && (binocle_input_is_key_pressed(&game.input, KEY_RIGHT) || binocle_input_is_key_pressed(&game.input, KEY_D))) {
      physics[i].dx += spd * it->delta_time * physics[i].time_mul;
      physics[i].dir = 1;
    }

    if (entity_is_alive(&health[i]) && (binocle_input_is_key_pressed(&game.input, KEY_UP) || binocle_input_is_key_pressed(&game.input, KEY_W))) {
      if (entity_on_ground(it->entities[i])) {
        physics[i].dy = 0.9f;
//        binocle_audio_play_sound(game.sounds.jump);
        // TODO: spawn effect
      }
    }

    if (entity_is_alive(&health[i]) && (binocle_input_is_key_pressed(&game.input, KEY_E) || binocle_input_is_key_pressed(&game.input, KEY_RETURN) || binocle_input_is_key_pressed(&game.input, KEY_SPACE) ||
                                        binocle_input_is_mouse_down(game.input, MOUSE_LEFT))) {
//      if (!cd.has("shoot")) {
//        // TODO: shoot bullet
//      }
    }
  }
}