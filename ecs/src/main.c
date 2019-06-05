//
//  Binocle
//  Copyright(C)2015-2019 Valerio Santinelli
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
#define BINOCLE_MATH_IMPL
#include "binocle_math.h"
#include "binocle_gd.h"
#include "binocle_log.h"
#include "binocle_bitmapfont.h"
#include "binocle_ecs.h"
#include "sys_config.h"

//#define GAMELOOP 1
#define MAX_SPRITES 8

typedef struct physics_component_t {
  kmVec2 pos;
  kmVec2 sub_pos;
  kmVec2 speed;
} physics_component_t;

typedef struct render_component_t {
  binocle_sprite sprite;
} render_component_t;

typedef struct player_component_t {
  bool jump;
} player_component_t;

binocle_window window;
binocle_input input;
binocle_viewport_adapter adapter;
binocle_camera camera;
binocle_entity_id_t entities[MAX_SPRITES];
binocle_gd gd;
binocle_bitmapfont *font;
binocle_image font_image;
binocle_texture font_texture;
binocle_material font_material;
binocle_sprite font_sprite;
kmVec2 font_sprite_pos;
binocle_sprite_batch sprite_batch;
binocle_shader shader;
float gravity;
kmAABB2 bounding_box;
binocle_ecs_t ecs;
binocle_component_id_t physics_component_id;
binocle_component_id_t render_component_id;
binocle_component_id_t player_component_id;
binocle_system_id_t movement_system_id;
binocle_system_id_t rendering_system_id;
binocle_system_id_t player_system_id;

void update_entity(binocle_entity_id_t entity, float dt) {
  binocle_log_info("Processing movement entity %lld", entity);
  physics_component_t *physics;
  binocle_ecs_get_component(&ecs, entity, physics_component_id, (void **)&physics);
  render_component_t *render;
  binocle_ecs_get_component(&ecs, entity, render_component_id, (void **)&render);

  physics->pos.x += physics->speed.x * dt;
  physics->pos.y += physics->speed.y * dt;
  physics->speed.y += gravity;

  if (physics->pos.x > bounding_box.max.x) {
    physics->speed.x *= -1.0f;
    physics->pos.x = bounding_box.max.x;
  } else if (physics->pos.x < bounding_box.min.x) {
    physics->speed.x *= -1.0f;
    physics->pos.x = bounding_box.min.x;
  }

  if (physics->pos.y < bounding_box.min.y) {
    physics->speed.y *= -0.8f;
    physics->pos.y = bounding_box.min.y;

    if ((float)lrand48()/RAND_MAX > 0.5f) {
      physics->speed.y -= 3.0f + (float)lrand48()/RAND_MAX * 4.0f;
    }
  } else if (physics->pos.y > bounding_box.max.y) {
    physics->speed.y = 0.0f;
    physics->pos.y = bounding_box.max.y;
  }

}

void process_movement(binocle_ecs_t *ecs, void *user_data, binocle_entity_id_t entity, float delta) {
  update_entity(entity, delta);
}

void process_rendering(binocle_ecs_t *ecs, void *user_data, binocle_entity_id_t entity, float delta) {
  physics_component_t *physics = NULL;
  binocle_ecs_get_component(&ecs, entity, physics_component_id, (void **)&physics);
  render_component_t *render = NULL;
  binocle_ecs_get_component(&ecs, entity, render_component_id, (void **)&render);
  if (physics == NULL || render == NULL) {
    binocle_log_error("physics or render components are NULL for entity %lld", entity);
    return;
  }
  binocle_sprite_batch_draw(&sprite_batch, render->sprite.material->texture, &physics->pos, NULL, NULL, NULL, 0.0f, NULL, binocle_color_white(), 0.0f);
}

void process_player(binocle_ecs_t *ecs, void *user_data, binocle_entity_id_t entity, float delta) {
  binocle_log_info("Processing player entity %lld", entity);
}

void main_loop() {
  binocle_window_begin_frame(&window);
  binocle_input_update(&input);

  if (input.resized) {
    kmVec2 oldWindowSize = {.x = window.width, .y = window.height};
    window.width = input.newWindowSize.x;
    window.height = input.newWindowSize.y;
    binocle_viewport_adapter_reset(&adapter, oldWindowSize, input.newWindowSize);
    input.resized = false;
  }

  kmMat4 matrix;
  kmMat4Identity(&matrix);
  binocle_sprite_batch_begin(&sprite_batch, binocle_camera_get_viewport(camera), BINOCLE_SPRITE_SORT_MODE_DEFERRED, &shader, &matrix);

  binocle_window_clear(&window);
  kmVec2 scale;
  scale.x = 1.0f;
  scale.y = 1.0f;

  binocle_ecs_process(&ecs, binocle_window_get_frame_time(&window) / 1000.0f);

  /*
  for (int i = 0 ; i < MAX_SPRITES ; i++) {
    physics_component_t *physics;
    binocle_ecs_get_component(&ecs, entities[i], physics_component_id, (void **)&physics);
    render_component_t *render;
    binocle_ecs_get_component(&ecs, entities[i], render_component_id, (void **)&render);
    binocle_sprite_batch_draw(&sprite_batch, render->sprite.material->texture, &physics->pos, NULL, NULL, NULL, 0.0f, NULL, binocle_color_white(), 0.0f);
  }
   */
  kmMat4 view_matrix;
  kmMat4Identity(&view_matrix);
  binocle_bitmapfont_draw_string(font, "TEST", 12, &gd, 20, 20, adapter.viewport, binocle_color_white(), view_matrix);
  binocle_sprite_batch_end(&sprite_batch, binocle_camera_get_viewport(camera));
  char fps_string[256];
  sprintf(fps_string, "FPS: %lld SPRITES: %d", binocle_window_get_fps(&window), MAX_SPRITES);
  binocle_bitmapfont_draw_string(font, fps_string, 32, &gd, 10, window.original_height - 32, adapter.viewport, binocle_color_black(), view_matrix);
  binocle_window_refresh(&window);
  binocle_window_end_frame(&window);
  //binocle_log_info("FPS: %d", binocle_window_get_fps(&window));
}

int main(int argc, char *argv[])
{
  binocle_sdl_init();
  window = binocle_window_new(320, 240, "Binocle Sprite Batch");
  binocle_window_set_background_color(&window, binocle_color_azure());
  adapter = binocle_viewport_adapter_new(window, BINOCLE_VIEWPORT_ADAPTER_KIND_SCALING, BINOCLE_VIEWPORT_ADAPTER_SCALING_TYPE_PIXEL_PERFECT, window.original_width, window.original_height, window.original_width, window.original_height);
  camera = binocle_camera_new(&adapter);
  input = binocle_input_new();
  char filename[1024];
  sprintf(filename, "%s%s", BINOCLE_DATA_DIR, "wabbit_alpha.png");
  binocle_image image = binocle_image_load(filename);
  binocle_texture texture = binocle_texture_from_image(image);
  char vert[1024];
  sprintf(vert, "%s%s", BINOCLE_DATA_DIR, "default.vert");
  char frag[1024];
  sprintf(frag, "%s%s", BINOCLE_DATA_DIR, "default.frag");
  shader = binocle_shader_load_from_file(vert, frag);
  binocle_material material = binocle_material_new();
  material.texture = &texture;
  material.shader = &shader;

  ecs = binocle_ecs_new();
  if (!binocle_ecs_create_component(&ecs, "physics", sizeof(physics_component_t), &physics_component_id)) {
    binocle_log_error("Cannot create component physics");
  }
  if (!binocle_ecs_create_component(&ecs, "render", sizeof(render_component_t), &render_component_id)) {
    binocle_log_error("Cannot create component render");
  }
  if (!binocle_ecs_create_component(&ecs, "player", sizeof(player_component_t), &player_component_id)) {
    binocle_log_error("Cannot create component player");
  }
  if (!binocle_ecs_create_system(&ecs, "movement", NULL, process_movement, NULL, NULL, NULL, NULL, BINOCLE_SYSTEM_FLAG_NORMAL, &movement_system_id)) {
    binocle_log_error("Cannot create movement system");
  }
  /*
  if (!binocle_ecs_create_system(&ecs, "rendering", NULL, process_rendering, NULL, NULL, NULL, NULL, BINOCLE_SYSTEM_FLAG_NORMAL, &rendering_system_id)) {
    binocle_log_error("Cannot create rendering system");
  }
   */
  if (!binocle_ecs_create_system(&ecs, "playercontrol", NULL, process_player, NULL, NULL, NULL, NULL, BINOCLE_SYSTEM_FLAG_NORMAL, &player_system_id)) {
    binocle_log_error("Cannot create playercontrol system");
  } else {
    if (!binocle_ecs_watch(&ecs, player_system_id, player_component_id)) {
      binocle_log_error("Cannot watch player component");
    }
  }
  if (!binocle_ecs_initialize(&ecs)) {
    binocle_log_error("Cannot initialize ECS");
  }

  srand48(42);
  gravity = -0.5f * 100.0f;
  bounding_box.min.x = 0;
  bounding_box.min.y = 0;
  bounding_box.max.x = 320;
  bounding_box.max.y = 240;
  for (int i = 0 ; i < MAX_SPRITES ; i++) {
    if (!binocle_ecs_create_entity(&ecs, &entities[i])) {
      binocle_log_error("Cannot create entity");
    } else {
      physics_component_t p = {0};
      p.pos.x = (float)lrand48()/RAND_MAX * bounding_box.max.x;
      p.pos.y = (float)lrand48()/RAND_MAX * bounding_box.max.y;
      p.sub_pos.x = 0;
      p.sub_pos.y = 0;
      p.speed.x = (float)lrand48()/RAND_MAX * 500.0f;
      p.speed.y = ((float)lrand48()/RAND_MAX * 500.0f) - 250.0f;
      if (!binocle_ecs_set_component(&ecs, entities[i], physics_component_id, &p)) {
        binocle_log_error("Cannot set physics component for entity %lld", entities[i]);
      }
      render_component_t r = {0};
      r.sprite = binocle_sprite_from_material(&material);
      if (!binocle_ecs_set_component(&ecs, entities[i], render_component_id, &r)) {
        binocle_log_error("Cannot set render component for entity %lld", entities[i]);
      }
      if (i == 0) {
        player_component_t player = {0};
        if (!binocle_ecs_set_component(&ecs, entities[i], player_component_id, &player)) {
          binocle_log_error("Cannot set player component for entity %lld", entities[i]);
        }
      }
      binocle_ecs_signal(&ecs, entities[i], BINOCLE_ENTITY_ADDED);
    }
  }

  char font_filename[1024];
  sprintf(font_filename, "%s%s", BINOCLE_DATA_DIR, "font.fnt");
  font = binocle_bitmapfont_from_file(font_filename, true);

  char font_image_filename[1024];
  sprintf(font_image_filename, "%s%s", BINOCLE_DATA_DIR, "font.png");
  font_image = binocle_image_load(font_image_filename);
  font_texture = binocle_texture_from_image(font_image);
  font_material = binocle_material_new();
  font_material.texture = &font_texture;
  font_material.shader = &shader;
  font->material = &font_material;
  font_sprite = binocle_sprite_from_material(&font_material);
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


