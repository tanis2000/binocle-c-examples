//
//  Binocle
//  Copyright(C)2015-2021 Valerio Santinelli
//

#include <stdio.h>
#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#endif
#include "binocle_sdl.h"
#include "backend/binocle_backend.h"
#include "backend/binocle_color.h"
#include "binocle_window.h"
#include "binocle_game.h"
#include "binocle_viewport_adapter.h"
#include "binocle_camera.h"
#include <binocle_input.h>
#include <binocle_image.h>
#include <backend/binocle_texture.h>
#include <binocle_sprite.h>
#include <backend/binocle_material.h>
#include <binocle_app.h>

#define BINOCLE_MATH_IMPL
#include "binocle_math.h"
#include "binocle_gd.h"
#include "binocle_log.h"
#include "binocle_bitmapfont.h"
#include "binocle_ecs.h"
#include "sys_config.h"

//#define GAMELOOP 1
#if defined(__IPHONEOS__)
#define DESIGN_WIDTH 812
#define DESIGN_HEIGHT 345
#else // defined(__IPHONEOS__)
#define DESIGN_WIDTH 320
#define DESIGN_HEIGHT 240
#endif // defined(__IPHONEOS__)
#define MAX_SPRITES 100

#if defined(BINOCLE_MACOS) && defined(BINOCLE_METAL)
#include "../assets/metal/default-metal-macosx.h"
#include "../assets/metal/screen-metal-macosx.h"
#endif

typedef struct physics_component_t {
  kmVec2 pos;
  kmVec2 sub_pos;
  kmVec2 speed;
} physics_component_t;

typedef struct render_component_t {
  binocle_sprite *sprite;
} render_component_t;

typedef struct player_component_t {
  bool jump;
} player_component_t;

typedef struct default_shader_params_t {
  float projectionMatrix[16];
  float modelMatrix[16];
  float viewMatrix[16];
} default_shader_params_t;

typedef struct screen_shader_fs_params_t {
  float resolution[2];
  float scale[2];
  float viewport[2];
} screen_shader_fs_params_t;

typedef struct screen_shader_vs_params_t {
  kmMat4 transform;
} screen_shader_vs_params_t;

binocle_app app;
binocle_window *window;
binocle_input input;
binocle_viewport_adapter *adapter;
binocle_camera camera;
binocle_entity_id_t entities[MAX_SPRITES];
binocle_gd gd;
binocle_bitmapfont *font;
binocle_image font_texture;
binocle_material *font_material;
binocle_sprite *font_sprite;
kmVec2 font_sprite_pos;
binocle_sprite_batch sprite_batch;
binocle_shader *shader;
float gravity;
kmAABB2 bounding_box;
binocle_ecs_t ecs;
binocle_component_id_t physics_component_id;
binocle_component_id_t render_component_id;
binocle_component_id_t player_component_id;
binocle_system_id_t movement_system_id;
binocle_system_id_t rendering_system_id;
binocle_system_id_t player_system_id;
uint64_t last_fps;
float frame_counter;
char *binocle_data_dir = NULL;
binocle_sprite *shared_sprite;
binocle_image wabbit_image;
binocle_shader default_shader;
binocle_shader screen_shader;

void update_entity(binocle_entity_id_t entity, float dt) {
  //binocle_log_info("Processing movement entity %lld", entity);
  physics_component_t *physics = NULL;
  binocle_ecs_get_component(&ecs, entity, physics_component_id, (void **)&physics);
  render_component_t *render = NULL;
  binocle_ecs_get_component(&ecs, entity, render_component_id, (void **)&render);
    if (physics == NULL || render == NULL) {
        binocle_log_error("update_entity(): physics or render components are NULL for entity %lld", entity);
        return;
    }

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
  binocle_ecs_get_component(ecs, entity, physics_component_id, (void **)&physics);
  render_component_t *render = NULL;
  binocle_ecs_get_component(ecs, entity, render_component_id, (void **)&render);
  if (physics == NULL || render == NULL) {
    binocle_log_error("process_rendering(): physics or render components are NULL for entity %lld", entity);
    return;
  }
  binocle_sprite_batch_draw(&sprite_batch, &render->sprite->material->albedo_texture, &physics->pos, NULL, NULL, NULL, 0.0f, NULL, binocle_color_white(), 0.0f);
}

void process_player(binocle_ecs_t *ecs, void *user_data, binocle_entity_id_t entity, float delta) {
  //binocle_log_info("Processing player entity %lld", entity);
}

void player_subscribed(binocle_ecs_t *ecs, void *user_data, binocle_entity_id_t entity) {
  //binocle_log_info("Player subscribed entity %lld", entity);
}

void main_loop() {
  binocle_window_begin_frame(window);
  float dt = binocle_window_get_frame_time(window) / 1000.0f;

  binocle_input_update(&input);

  if (input.resized) {
    kmVec2 oldWindowSize = {.x = window->width, .y = window->height};
    window->width = input.newWindowSize.x;
    window->height = input.newWindowSize.y;
    binocle_viewport_adapter_reset(adapter, oldWindowSize, input.newWindowSize);
    input.resized = false;
  }

  kmMat4 matrix;
  kmMat4Identity(&matrix);
  binocle_sprite_batch_begin(&sprite_batch, binocle_camera_get_viewport(camera), BINOCLE_SPRITE_SORT_MODE_DEFERRED, shader, &matrix);

//  binocle_window_clear(window);
  kmVec2 scale;
  scale.x = 1.0f;
  scale.y = 1.0f;

  binocle_ecs_process(&ecs, dt);

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
  binocle_sprite_batch_end(&sprite_batch, binocle_camera_get_viewport(camera));

  char fps_string[256];
  sprintf(fps_string, "FPS: %lld", last_fps);
  binocle_bitmapfont_draw_string(font, fps_string, 16, &gd, 10, window->original_height - 32, adapter->viewport, binocle_color_black(), view_matrix);
  char sprites_string[256];
  sprintf(sprites_string, "SPRITES: %lld", MAX_SPRITES);
  binocle_bitmapfont_draw_string(font, sprites_string, 16, &gd, 10, window->original_height - 64, adapter->viewport, binocle_color_black(), view_matrix);

  // Gets the viewport calculated by the adapter
  kmAABB2 vp = binocle_viewport_adapter_get_viewport(*adapter);

  binocle_gd_render(&gd, window, DESIGN_WIDTH, DESIGN_HEIGHT, vp);

  binocle_window_refresh(window);
  binocle_window_end_frame(window);

  frame_counter += dt;
  if (frame_counter >= 1.0f) {
    frame_counter = 0;
    last_fps = binocle_window_get_fps(window);
  }
  //binocle_log_info("FPS: %d", binocle_window_get_fps(&window));
}

int main(int argc, char *argv[])
{
  app = binocle_app_new();
  binocle_app_init(&app);
  binocle_sdl_init();
  window = binocle_window_new(DESIGN_WIDTH, DESIGN_HEIGHT, "Binocle ECS");
  binocle_window_set_background_color(window, binocle_color_azure());
  binocle_window_set_minimum_size(window, DESIGN_WIDTH, DESIGN_HEIGHT);
  adapter = binocle_viewport_adapter_new(window, BINOCLE_VIEWPORT_ADAPTER_KIND_SCALING, BINOCLE_VIEWPORT_ADAPTER_SCALING_TYPE_PIXEL_PERFECT, window->original_width, window->original_height, window->original_width, window->original_height);
  camera = binocle_camera_new(adapter);
  input = binocle_input_new();
  gd = binocle_gd_new();
  binocle_gd_init(&gd, window);

  binocle_data_dir = binocle_sdl_assets_dir();
  binocle_log_info("Current base path: %s", binocle_data_dir);

  char filename[1024];
  sprintf(filename, "%s%s", binocle_data_dir, "wabbit_alpha.png");
  wabbit_image = binocle_image_load(filename);

#ifdef BINOCLE_GL
  // Default shader
  char vert[1024];
  sprintf(vert, "%s%s", binocle_data_dir, "default.vert");
  char frag[1024];
  sprintf(frag, "%s%s", binocle_data_dir, "default.frag");

  char *shader_vs_src;
  size_t shader_vs_src_size;
  binocle_sdl_load_text_file(vert, &shader_vs_src, &shader_vs_src_size);

  char *shader_fs_src;
  size_t shader_fs_src_size;
  binocle_sdl_load_text_file(frag, &shader_fs_src, &shader_fs_src_size);
#endif

  binocle_shader_desc default_shader_desc = {
#ifdef BINOCLE_GL
    .vs.source = shader_vs_src,
#else
    .vs.byte_code = default_vs_bytecode,
    .vs.byte_code_size = sizeof(default_vs_bytecode),
#endif
    .attrs = {
      [0].name = "vertexPosition",
      [1].name = "vertexColor",
      [2].name = "vertexTCoord",
    },
    .vs.uniform_blocks[0] = {
      .size = sizeof(default_shader_params_t),
      .uniforms = {
        [0] = { .name = "projectionMatrix", .type = BINOCLE_UNIFORMTYPE_MAT4},
        [1] = { .name = "viewMatrix", .type = BINOCLE_UNIFORMTYPE_MAT4},
        [2] = { .name = "modelMatrix", .type = BINOCLE_UNIFORMTYPE_MAT4},
      }
    },
#ifdef BINOCLE_GL
    .fs.source = shader_fs_src,
#else
    .fs.byte_code = default_fs_bytecode,
    .fs.byte_code_size = sizeof(default_fs_bytecode),
#endif
    .fs.images[0] = { .name = "tex0", .type = BINOCLE_IMAGETYPE_2D},
  };
  default_shader = binocle_backend_make_shader(&default_shader_desc);

#ifdef BINOCLE_GL
  // Screen shader
  sprintf(vert, "%s%s", binocle_data_dir, "screen.vert");
  sprintf(frag, "%s%s", binocle_data_dir, "screen.frag");

  char *screen_shader_vs_src;
  size_t screen_shader_vs_src_size;
  binocle_sdl_load_text_file(vert, &screen_shader_vs_src, &screen_shader_vs_src_size);

  char *screen_shader_fs_src;
  size_t screen_shader_fs_src_size;
  binocle_sdl_load_text_file(frag, &screen_shader_fs_src, &screen_shader_fs_src_size);
#endif

  binocle_shader_desc screen_shader_desc = {
#ifdef BINOCLE_GL
    .vs.source = screen_shader_vs_src,
#else
    .vs.byte_code = screen_vs_bytecode,
    .vs.byte_code_size = sizeof(screen_vs_bytecode),
#endif
    .attrs = {
      [0].name = "position"
    },
    .vs.uniform_blocks[0] = {
      .size = sizeof(screen_shader_vs_params_t),
      .uniforms = {
        [0] = { .name = "transform", .type = BINOCLE_UNIFORMTYPE_MAT4},
      },
    },
#ifdef BINOCLE_GL
    .fs.source = screen_shader_fs_src,
#else
    .fs.byte_code = screen_fs_bytecode,
    .fs.byte_code_size = sizeof(screen_fs_bytecode),
#endif
    .fs.images[0] = { .name = "texture", .type = BINOCLE_IMAGETYPE_2D},
    .fs.uniform_blocks[0] = {
      .size = sizeof(screen_shader_fs_params_t),
      .uniforms = {
        [0] = { .name = "resolution", .type = BINOCLE_UNIFORMTYPE_FLOAT2 },
        [1] = { .name = "scale", .type = BINOCLE_UNIFORMTYPE_FLOAT2 },
        [2] = { .name = "viewport", .type = BINOCLE_UNIFORMTYPE_FLOAT2 },
      },
    },
  };
  screen_shader = binocle_backend_make_shader(&screen_shader_desc);

  binocle_material *material = binocle_material_new();
  material->albedo_texture = wabbit_image;
  material->shader = default_shader;
  shared_sprite = binocle_sprite_from_material(material);

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
  if (!binocle_ecs_create_system(&ecs, "rendering", NULL, process_rendering, NULL, NULL, NULL, NULL, BINOCLE_SYSTEM_FLAG_NORMAL, &rendering_system_id)) {
    binocle_log_error("Cannot create rendering system");
  }
  if (!binocle_ecs_create_system(&ecs, "playercontrol", NULL, process_player, NULL, player_subscribed, NULL, NULL, BINOCLE_SYSTEM_FLAG_NORMAL, &player_system_id)) {
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
  bounding_box.max.x = DESIGN_WIDTH;
  bounding_box.max.y = DESIGN_HEIGHT;
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
      r.sprite = shared_sprite;
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
  sprintf(font_filename, "%s%s", binocle_data_dir, "font.fnt");
  font = binocle_bitmapfont_from_file(font_filename, true);

  char font_image_filename[1024];
  sprintf(font_image_filename, "%s%s", binocle_data_dir, "font.png");
  font_texture = binocle_image_load(font_image_filename);
  font_material = binocle_material_new();
  font_material->albedo_texture = font_texture;
  font_material->shader = default_shader;
  font->material = font_material;
  font_sprite = binocle_sprite_from_material(font_material);
  font_sprite_pos.x = 0;
  font_sprite_pos.y = -256;

  sprite_batch = binocle_sprite_batch_new();
  sprite_batch.gd = &gd;

  binocle_gd_setup_default_pipeline(&gd, DESIGN_WIDTH, DESIGN_HEIGHT, default_shader, screen_shader);

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
  binocle_image_destroy(wabbit_image);
//  binocle_shader_destroy(shader);
  binocle_material_destroy(material);
  binocle_sprite_destroy(shared_sprite);
  binocle_image_destroy(font_texture);
  binocle_material_destroy(font_material);
  binocle_sprite_destroy(font_sprite);
  binocle_gd_destroy(&gd);
  binocle_app_destroy(&app);
  binocle_sdl_exit();
}


