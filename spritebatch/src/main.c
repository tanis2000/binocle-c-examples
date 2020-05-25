//
//  Binocle
//  Copyright(C)2015-2018 Valerio Santinelli
//

#include <stdio.h>
#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#endif
#include "binocle_sdl.h"
#include "binocle_color.h"
#include "binocle_window.h"
#include "binocle_game.h"
#include "binocle_viewport_adapter.h"
#include "binocle_camera.h"
#include <binocle_input.h>
#include <binocle_image.h>
#include <binocle_texture.h>
#include <binocle_sprite.h>
#include <binocle_shader.h>
#include <binocle_material.h>
#include <binocle_array.h>
#define BINOCLE_MATH_IMPL
#include "binocle_math.h"
#include "binocle_gd.h"
#include "binocle_log.h"
#include "binocle_bitmapfont.h"
#include "sys_config.h"

//#define GAMELOOP 1
#define START_SPRITES 1024

typedef struct entity_t {
  binocle_sprite *sprite;
  kmVec2 pos;
  kmVec2 sub_pos;
  kmVec2 speed;
} entity_t;

binocle_window *window;
binocle_input input;
binocle_viewport_adapter adapter;
binocle_camera camera;
entity_t *entities;
binocle_gd gd;
binocle_bitmapfont *font;
binocle_image *font_image;
binocle_texture *font_texture;
binocle_material *font_material;
binocle_sprite *font_sprite;
kmVec2 font_sprite_pos;
binocle_sprite_batch sprite_batch;
binocle_shader *shader;
binocle_image *image;
binocle_texture *texture;
binocle_material *material;
float gravity;
kmAABB2 bounding_box;
uint32_t number_of_sprites = START_SPRITES;

void create_entity(entity_t *entity) {
  entity->sprite = binocle_sprite_from_material(material);
  entity->pos.x = (float)lrand48()/RAND_MAX * bounding_box.max.x;
  entity->pos.y = (float)lrand48()/RAND_MAX * bounding_box.max.y;
  entity->sub_pos.x = 0;
  entity->sub_pos.y = 0;
  entity->speed.x = (float)lrand48()/RAND_MAX * 500.0f;
  entity->speed.y = ((float)lrand48()/RAND_MAX * 500.0f) - 250.0f;
}

void update_entity(entity_t *entity, float dt) {
  entity->pos.x += entity->speed.x * dt;
  entity->pos.y += entity->speed.y * dt;
  entity->speed.y += gravity;

  if (entity->pos.x > bounding_box.max.x) {
    entity->speed.x *= -1.0f;
    entity->pos.x = bounding_box.max.x;
  } else if (entity->pos.x < bounding_box.min.x) {
    entity->speed.x *= -1.0f;
    entity->pos.x = bounding_box.min.x;
  }

  if (entity->pos.y < bounding_box.min.y) {
    entity->speed.y *= -0.8f;
    entity->pos.y = bounding_box.min.y;

    if ((float)lrand48()/RAND_MAX > 0.5f) {
      entity->speed.y -= 3.0f + (float)lrand48()/RAND_MAX * 4.0f;
    }
  } else if (entity->pos.y > bounding_box.max.y) {
    entity->speed.y = 0.0f;
    entity->pos.y = bounding_box.max.y;
  }

}

void main_loop() {
  binocle_window_begin_frame(window);
  binocle_input_update(&input);

  if (input.resized) {
    kmVec2 oldWindowSize = {.x = window->width, .y = window->height};
    window->width = input.newWindowSize.x;
    window->height = input.newWindowSize.y;
    binocle_viewport_adapter_reset(&adapter, oldWindowSize, input.newWindowSize);
    input.resized = false;
  }

  if (binocle_input_is_key_pressed(&input, KEY_SPACE)) {
    uint32_t old_number_of_sprites = number_of_sprites;
    number_of_sprites += 256;
    binocle_array_grow(entities, number_of_sprites);
    for (int i = old_number_of_sprites ; i < number_of_sprites ; i++) {
      create_entity(&entities[i]);
    }
  }

  kmMat4 matrix;
  kmMat4Identity(&matrix);
  binocle_sprite_batch_begin(&sprite_batch, binocle_camera_get_viewport(camera), BINOCLE_SPRITE_SORT_MODE_DEFERRED, shader, &matrix);

  binocle_window_clear(window);
  kmVec2 scale;
  scale.x = 1.0f;
  scale.y = 1.0f;
  for (int i = 0 ; i < number_of_sprites ; i++) {
    update_entity(&entities[i], (binocle_window_get_frame_time(window) / 1000.0f));
    //binocle_sprite_draw(entities[i].sprite, &gd, (uint64_t)entities[i].pos.x, (uint64_t)entities[i].pos.y, adapter.viewport, 0, scale, &camera);
    //binocle_sprite_batch_draw_position(&sprite_batch, entities[i].sprite.material->texture, entities[i].pos);
    binocle_sprite_batch_draw(&sprite_batch, entities[i].sprite->material->albedo_texture, &entities[i].pos, NULL, NULL, NULL, 0.0f, NULL, binocle_color_white(), 0.0f);
  }
  kmMat4 view_matrix;
  kmMat4Identity(&view_matrix);
  //binocle_sprite_draw(font_sprite, &gd, (uint64_t)font_sprite_pos.x, (uint64_t)font_sprite_pos.y, adapter.viewport);
  binocle_sprite_batch_end(&sprite_batch, binocle_camera_get_viewport(camera));

  char fps[256];
  sprintf(fps, "FPS:%d COUNT: %d PRESS SPACE TO ADD", binocle_window_get_fps(window), number_of_sprites);
  binocle_bitmapfont_draw_string(font, fps, 16, &gd, 20, 20, adapter.viewport, binocle_color_white(), view_matrix);
  binocle_window_refresh(window);
  binocle_window_end_frame(window);
  //binocle_log_info("FPS: %d", binocle_window_get_fps(&window));
}

int main(int argc, char *argv[])
{
  binocle_sdl_init();
  window = binocle_window_new(320, 240, "Binocle Sprite Batch");
  binocle_window_set_background_color(window, binocle_color_azure());
  adapter = binocle_viewport_adapter_new(*window, BINOCLE_VIEWPORT_ADAPTER_KIND_SCALING, BINOCLE_VIEWPORT_ADAPTER_SCALING_TYPE_PIXEL_PERFECT, window->original_width, window->original_height, window->original_width, window->original_height);
  camera = binocle_camera_new(&adapter);
  input = binocle_input_new();
  char filename[1024];
  sprintf(filename, "%s%s", BINOCLE_DATA_DIR, "wabbit_alpha.png");
  image = binocle_image_load(filename);
  texture = binocle_texture_from_image(image);
  char vert[1024];
  sprintf(vert, "%s%s", BINOCLE_DATA_DIR, "default.vert");
  char frag[1024];
  sprintf(frag, "%s%s", BINOCLE_DATA_DIR, "default.frag");
  shader = binocle_shader_load_from_file(vert, frag);
  material = binocle_material_new();
  material->albedo_texture = texture;
  material->shader = shader;
  srand48(42);
  gravity = -0.5f * 100.0f;
  bounding_box.min.x = 0;
  bounding_box.min.y = 0;
  bounding_box.max.x = 320;
  bounding_box.max.y = 240;

  binocle_array_set_capacity(entities, number_of_sprites);
  for (int i = 0 ; i < number_of_sprites ; i++) {
    create_entity(&entities[i]);
  }

  char font_filename[1024];
  sprintf(font_filename, "%s%s", BINOCLE_DATA_DIR, "font.fnt");
  font = binocle_bitmapfont_from_file(font_filename, true);

  char font_image_filename[1024];
  sprintf(font_image_filename, "%s%s", BINOCLE_DATA_DIR, "font.png");
  font_image = binocle_image_load(font_image_filename);
  font_texture = binocle_texture_from_image(font_image);
  font_material = binocle_material_new();
  font_material->albedo_texture = font_texture;
  font_material->shader = shader;
  font->material = font_material;
  font_sprite = binocle_sprite_from_material(font_material);
  font_sprite_pos.x = 0;
  font_sprite_pos.y = -256;

  gd = binocle_gd_new();
  binocle_gd_init(&gd);
  sprite_batch = binocle_sprite_batch_new();
  sprite_batch.gd = &gd;
#ifdef GAMELOOP
  binocle_game_run(window, input);
#else
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  while (!input.quit_requested) {
    main_loop();
  }
#endif
  binocle_log_info("Quit requested");
#endif
  binocle_sdl_exit();
}


