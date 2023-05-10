//
//  Binocle
//  Copyright(C)2015-2021 Valerio Santinelli
//

#include <stdio.h>
#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#endif
#include "binocle_sdl.h"
#include "backend/binocle_color.h"
#include "binocle_window.h"
#include "binocle_game.h"
#include "binocle_viewport_adapter.h"
#include "binocle_camera.h"
#include <binocle_input.h>
#include <binocle_image.h>
#include <binocle_sprite.h>
#include <backend/binocle_material.h>
#include <binocle_lua.h>

#define BINOCLE_MATH_IMPL
#include "binocle_math.h"
#include "binocle_gd.h"
#include "binocle_log.h"
#include "binocle_bitmapfont.h"
#include "binocle_ecs.h"
#include "binocle_app.h"

#define DESIGN_WIDTH 800
#define DESIGN_HEIGHT 600

#if defined(BINOCLE_MACOS) && defined(BINOCLE_METAL)
#include "../assets/metal/default-metal-macosx.h"
#include "../assets/metal/screen-metal-macosx.h"
#endif


#if defined(__IPHONEOS__) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
#define SHADER_PATH "gles"
#else
#define SHADER_PATH "gl33"
#endif

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
binocle_gd gd;
sg_shader *shader;
char *binocle_data_dir = NULL;
binocle_app app;
sg_image image;
binocle_sprite *sprite;
binocle_material *material;
sg_shader default_shader;
sg_shader screen_shader;

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

  kmAABB2 viewport = binocle_camera_get_viewport(camera);
  sg_image_desc info = sg_query_image_desc(sprite->material->albedo_texture);
  kmVec2 scale;
  scale.x = (float)DESIGN_WIDTH / (float)info.width;
  scale.y = (float)DESIGN_HEIGHT / (float)info.width;
  int64_t x = (int64_t)((DESIGN_WIDTH - ((float)info.width * scale.x)) / 2.0f);
  int64_t y = (int64_t)((DESIGN_HEIGHT - ((float)info.height * scale.y)) / 2.0f);

  sg_color white = binocle_color_white();
  binocle_sprite_draw(sprite, &gd, x, y, &viewport, 0, &scale, &camera, 0, &white);

  // Gets the viewport calculated by the adapter
  kmAABB2 vp = binocle_viewport_adapter_get_viewport(*adapter);

  kmMat4 view_matrix;
  kmMat4Identity(&view_matrix);

  binocle_gd_render(&gd, window, DESIGN_WIDTH, DESIGN_HEIGHT, vp, view_matrix, 1.0f);

  binocle_window_refresh(window);
  binocle_window_end_frame(window);
}

int main(int argc, char *argv[])
{
  binocle_app_desc_t app_desc = {0};
  app = binocle_app_new();
  binocle_app_init(&app, &app_desc);

  binocle_data_dir = binocle_sdl_assets_dir();
  binocle_log_info("Current base path: %s", binocle_data_dir);

  window = binocle_window_new(DESIGN_WIDTH, DESIGN_HEIGHT, "Binocle Basic Example");
  binocle_window_set_background_color(window, binocle_color_white());
  binocle_window_set_minimum_size(window, DESIGN_WIDTH, DESIGN_HEIGHT);
  adapter = binocle_viewport_adapter_new(window, BINOCLE_VIEWPORT_ADAPTER_KIND_SCALING, BINOCLE_VIEWPORT_ADAPTER_SCALING_TYPE_PIXEL_PERFECT, window->original_width, window->original_height, window->original_width, window->original_height);
  camera = binocle_camera_new(adapter);
  input = binocle_input_new();
  gd = binocle_gd_new();
  binocle_gd_init(&gd, window);

#ifdef BINOCLE_GL
  // Default shader
  char vert[1024];
  sprintf(vert, "%sshaders/%s/%s", binocle_data_dir, SHADER_PATH, "default.vert");
  char frag[1024];
  sprintf(frag, "%sshaders/%s/%s", binocle_data_dir, SHADER_PATH, "default.frag");

  char *shader_vs_src;
  size_t shader_vs_src_size;
  binocle_sdl_load_text_file(vert, &shader_vs_src, &shader_vs_src_size);

  char *shader_fs_src;
  size_t shader_fs_src_size;
  binocle_sdl_load_text_file(frag, &shader_fs_src, &shader_fs_src_size);
#endif

  sg_shader_desc default_shader_desc = {
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
        [0] = { .name = "projectionMatrix", .type = SG_UNIFORMTYPE_MAT4},
        [1] = { .name = "viewMatrix", .type = SG_UNIFORMTYPE_MAT4},
        [2] = { .name = "modelMatrix", .type = SG_UNIFORMTYPE_MAT4},
      }
    },
#ifdef BINOCLE_GL
    .fs.source = shader_fs_src,
#else
    .fs.byte_code = default_fs_bytecode,
    .fs.byte_code_size = sizeof(default_fs_bytecode),
#endif
    .fs.images[0] = { .name = "tex0", .image_type = SG_IMAGETYPE_2D},
  };
  default_shader = sg_make_shader(&default_shader_desc);

#ifdef BINOCLE_GL
  // Screen shader
  sprintf(vert, "%sshaders/%s/%s", binocle_data_dir, SHADER_PATH, "screen.vert");
  sprintf(frag, "%sshaders/%s/%s", binocle_data_dir, SHADER_PATH, "screen.frag");

  char *screen_shader_vs_src;
  size_t screen_shader_vs_src_size;
  binocle_sdl_load_text_file(vert, &screen_shader_vs_src, &screen_shader_vs_src_size);

  char *screen_shader_fs_src;
  size_t screen_shader_fs_src_size;
  binocle_sdl_load_text_file(frag, &screen_shader_fs_src, &screen_shader_fs_src_size);
#endif

  sg_shader_desc screen_shader_desc = {
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
        [0] = { .name = "transform", .type = SG_UNIFORMTYPE_MAT4},
      },
    },
#ifdef BINOCLE_GL
    .fs.source = screen_shader_fs_src,
#else
    .fs.byte_code = screen_fs_bytecode,
    .fs.byte_code_size = sizeof(screen_fs_bytecode),
#endif
    .fs.images[0] = { .name = "tex0", .image_type = SG_IMAGETYPE_2D},
    .fs.uniform_blocks[0] = {
      .size = sizeof(screen_shader_fs_params_t),
      .uniforms = {
        [0] = { .name = "resolution", .type = SG_UNIFORMTYPE_FLOAT2 },
        [1] = { .name = "scale", .type = SG_UNIFORMTYPE_FLOAT2 },
        [2] = { .name = "viewport", .type = SG_UNIFORMTYPE_FLOAT2 },
      },
    },
  };
  screen_shader = sg_make_shader(&screen_shader_desc);

  char filename[1024];
  sprintf(filename, "%s%s", binocle_data_dir, "binocle-logo-full.png");
  image = binocle_image_load(filename);
  material = binocle_material_new();
  material->albedo_texture = image;
  material->shader = default_shader;
  sprite = binocle_sprite_from_material(material);

  binocle_gd_setup_default_pipeline(&gd, DESIGN_WIDTH, DESIGN_HEIGHT, default_shader, screen_shader);

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  while (!input.quit_requested) {
    main_loop();
  }
#endif
  binocle_log_info("Quit requested");
  binocle_sprite_destroy(sprite);
  binocle_material_destroy(material);
  binocle_image_destroy(image);
  free(binocle_data_dir);
  binocle_app_destroy(&app);
}


