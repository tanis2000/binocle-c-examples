//
// Created by Valerio Santinelli on 28/02/24.
//

#include "systems.h"
#include "types.h"
#include "entity.h"
#include "flecs.h"

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

void system_animations_update(ecs_iter_t *it) {
  graphics_component_t *graphics = ecs_field(it, graphics_component_t, 1);
  for (int i = 0; i < it->count; i++) {
    graphics_component_t *g = &graphics[i];

    if (g->anim.state == 6) {
      entity_play_animation(g, ANIMATION_ID_HERO_DEATH, false);
    } else if (g->anim.state == 3) {
      entity_play_animation(g, ANIMATION_ID_HERO_JUMP_UP, false);
    } else if (g->anim.state == 4) {
      entity_play_animation(g, ANIMATION_ID_HERO_JUMP_DOWN, false);
    } else if (g->anim.state == 2) {
      entity_play_animation(g, ANIMATION_ID_HERO_RUN, false);
    } else if (g->anim.state == 5) {
      entity_play_animation(g, ANIMATION_ID_HERO_SHOOT, false);
    } else {
      if (g->anim.state == 1) {
        entity_play_animation(g, ANIMATION_ID_HERO_IDLE2, false);
      } else if (g->anim.state == 0){
        entity_play_animation(g, ANIMATION_ID_HERO_IDLE1, false);
      }
    }

//    if (e->health < 0) {
//      entity_play_animation(e, ANIMATION_ID_HERO_DEATH, false);
//    } else if (e->dy > 0 && !entity_on_ground(e)) {
//      entity_play_animation(e, ANIMATION_ID_HERO_JUMP_UP, false);
//    } else if (!entity_on_ground(e)) {
//      entity_play_animation(e, ANIMATION_ID_HERO_JUMP_DOWN, false);
//    } else if (entity_on_ground(e) && e->dx != 0) {
//      entity_play_animation(e, ANIMATION_ID_HERO_RUN, false);
//    } else if (hero_is_shooting(e)) {
//      entity_play_animation(e, ANIMATION_ID_HERO_SHOOT, false);
//    } else {
//      if (m_rand_range_float(0, 1) < 0.4f) {
//        entity_play_animation(e, ANIMATION_ID_HERO_IDLE2, false);
//      } else {
//        entity_play_animation(e, ANIMATION_ID_HERO_IDLE1, false);
//      }
//    }

    entity_update_animation(g, it->delta_time);
  }
}