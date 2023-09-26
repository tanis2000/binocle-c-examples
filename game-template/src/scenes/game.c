//
// Created by Valerio Santinelli on 29/08/23.
//

#include "game.h"
#include "scene.h"
#include "cache.h"
#include "backend/binocle_material.h"
#include "en/hero.h"
#include "level.h"
#include "entity.h"
#include "game_camera.h"

extern struct game_t game;

void game_create_game_camera(game_scene_t *data) {
  game.game_camera = game_camera_new();
  game_camera_track_entity(&game.game_camera, game.hero, false, 1.0f);
  game_camera_center_on_target(&game.game_camera);
}

void game_init(scene_t *s) {
  s->name = "game";
  game_scene_t *data = s->data;
  game.hero = hero_new();

  game.level = (level_t) {0};
  level_load_tilemap(&game.level, "maps/map01.json");

  spawner_t *hs = level_get_hero_spawner(&game.level);
  entity_set_pos_grid(game.hero, hs->cx + 10, hs->cy);

  binocle_gd_set_offscreen_clear_color(&game.gfx.gd, binocle_color_new(73.0f/256.0f, 77.0f/256.0f, 126.0f/256.0f, 1.0f));

  game_create_game_camera(data);

  // TODO: add in-game HUD

  s->on_pre_update = game_on_pre_update;
  s->on_update = game_on_update;
  s->on_post_update = game_on_post_update;
}

scene_t *game_new() {
  scene_t *s = scene_new(NULL);
  game_scene_t *data = SDL_calloc(1, sizeof(game_scene_t));
  s->data = data;
  game_init(s);
  return s;
}

void game_free(scene_t *s) {
  SDL_free(s->data);
  s->data = NULL;
  scene_free(s);
}

void game_on_pre_update(scene_t *s, float dt) {
//  for (int i = 0; i < game.num_entities; i++) {
//    entity_pre_update(&game.entities[i], dt);
//  }
}

void game_on_update(scene_t *s, float dt) {
  game_scene_t *data = s->data;

  if (binocle_input_is_key_down(game.input, KEY_2)) {
    scene_pause(s);
  }

  if (binocle_input_is_key_down(game.input, KEY_3)) {
    scene_resume(s);
  }

  hero_input_update(game.hero, dt);

  for (int i = 0; i < game.num_entities; i++) {
    entity_update(&game.entities[i], dt);
  }

  game_camera_update(&game.game_camera, dt);
  level_render(&game.level);
}

void game_on_post_update(scene_t *s, float dt) {
  game_scene_t *data = s->data;
  for (int i = 0; i < game.num_entities; i++) {
    entity_post_update(&game.entities[i], dt);
    entity_draw(&game.entities[i]);
//    entity_draw_debug(&game.entities[i]);
  }
  entity_system_update(); // garbage collection
  game_camera_post_update(&game.game_camera);
}