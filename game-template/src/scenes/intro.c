//
// Created by Valerio Santinelli on 29/08/23.
//

#include "intro.h"
#include "scene.h"
#include "cache.h"
#include "backend/binocle_material.h"
#include "game.h"

extern struct game_t game;

void intro_init(scene_t *s) {
  s->name = "intro";
  intro_scene_t *intro = s->data;
  intro->tex_width = 1682;
  intro->tex_height = 479;
  intro->tanis_tex_width = 512;
  intro->tanis_tex_height = 250;
  intro->img = cache_load_image("img/binocle-logo-full.png");
  intro->mat = binocle_material_new();
  intro->mat->albedo_texture = intro->img;
  intro->mat->shader = game.gfx.default_shader;
  intro->logo = binocle_sprite_from_material(intro->mat);

  s->on_update = intro_on_update;
}

scene_t *intro_new() {
  scene_t *s = scene_new(NULL);
  intro_scene_t *intro = SDL_calloc(1, sizeof(intro_scene_t));
  s->data = intro;
  intro_init(s);
  return s;
}

void intro_free(scene_t *s) {
  SDL_free(s->data);
  s->data = NULL;
  scene_free(s);
}

void intro_on_update(scene_t *s, float dt) {
  intro_scene_t *intro = s->data;

  if (binocle_input_is_key_pressed(&game.input, KEY_RETURN) || binocle_input_is_mouse_down(game.input, MOUSE_LEFT)) {
    scene_t *new_scene = game_new();
    game.scene = new_scene;
    return;
  }

  kmVec2 scale = {
    .x = (float)DESIGN_WIDTH / (float)intro->tex_width,
    .y = (float)DESIGN_HEIGHT / (float)intro->tex_width,
  };

  float x = (DESIGN_WIDTH - ((float)intro->tex_width * scale.x)) / 2.0f;
  float y = (DESIGN_HEIGHT - ((float)intro->tex_height * scale.y)) / 2.0f;

  binocle_gd_set_offscreen_clear_color(&game.gfx.gd, binocle_color_white());
  kmAABB2 viewport;
  kmAABB2Initialize(&viewport, &(kmVec2){.x = (float)DESIGN_WIDTH/2.0f, .y = (float)DESIGN_HEIGHT/2.0f}, DESIGN_WIDTH, DESIGN_HEIGHT, 0);
  binocle_sprite_draw(intro->logo, &game.gfx.gd, x, y, &viewport, 0, &scale, &game.gfx.camera, 0, NULL);

  const char *line1 = "Press ENTER or LEFT MOUSE CLICK to START";
  float width = binocle_ttfont_get_string_width(game.gfx.default_font, line1);
  binocle_ttfont_draw_string(game.gfx.default_font, line1, &game.gfx.gd, ((float)DESIGN_WIDTH - width)/2.0f, 50, viewport, binocle_color_black(), &game.gfx.camera, 0);

  const char *line2 = "A game by Valerio Santinelli";
  width = binocle_ttfont_get_string_width(game.gfx.default_font, line2);
  binocle_ttfont_draw_string(game.gfx.default_font, line2, &game.gfx.gd, ((float)DESIGN_WIDTH - width)/2.0f, 170, viewport, binocle_color_black(), &game.gfx.camera, 0);

}