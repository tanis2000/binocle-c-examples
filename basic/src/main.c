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

typedef struct state_t {
  struct {
    binocle_pipeline pip;
    binocle_bindings bind;
    binocle_pass_action action;
  } display;
} state_t;

binocle_app app;
binocle_window *window;
binocle_input input;
binocle_viewport_adapter *adapter;
binocle_camera camera;
binocle_gd gd;
binocle_shader *shader;
char *binocle_data_dir = NULL;
binocle_app app;
binocle_image image;
binocle_sprite *sprite;
binocle_material *material;
binocle_shader default_shader;
binocle_shader screen_shader;
state_t state;

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

  kmVec2 scale;
  scale.x = 0.3f;
  scale.y = 0.3f;
  kmAABB2 viewport = binocle_camera_get_viewport(camera);
  binocle_image_info info = binocle_backend_query_image_info(sprite->material->albedo_texture);
  int64_t x = (int64_t)((DESIGN_WIDTH - ((float)info.width * scale.x)) / 2.0f);
  int64_t y = (int64_t)((DESIGN_HEIGHT - ((float)info.height * scale.x)) / 2.0f);

  binocle_sprite_draw(sprite, &gd, x, y, &viewport, 0, &scale, &camera);

  screen_shader_vs_params_t screen_vs_params;
  screen_shader_fs_params_t screen_fs_params;
  kmMat4Identity(&screen_vs_params.transform);
  screen_fs_params.resolution[0] = DESIGN_WIDTH;
  screen_fs_params.resolution[1] = DESIGN_HEIGHT;
  screen_fs_params.scale[0] = 1;
  screen_fs_params.scale[1] = 1;

  // Gets the viewport calculated by the adapter
  kmAABB2 vp = binocle_viewport_adapter_get_viewport(*adapter);
  float vp_x = vp.min.x;
  float vp_y = vp.min.y;
  screen_fs_params.viewport[0] = vp_x;
  screen_fs_params.viewport[1] = vp_y;

  binocle_gd_render(&gd);

  state.display.bind.fs_images[0] = gd.offscreen.render_target;

  binocle_backend_begin_default_pass(&state.display.action, window->width, window->height);
  binocle_backend_apply_pipeline(state.display.pip);
  binocle_backend_apply_bindings(&state.display.bind);
  binocle_backend_apply_uniforms(BINOCLE_SHADERSTAGE_VS, 0, &BINOCLE_RANGE(screen_vs_params));
  binocle_backend_apply_uniforms(BINOCLE_SHADERSTAGE_FS, 0, &BINOCLE_RANGE(screen_fs_params));
  binocle_backend_draw(0, 6, 1);
  binocle_backend_end_pass();

  binocle_backend_commit();

  binocle_window_refresh(window);
  binocle_window_end_frame(window);
}

int main(int argc, char *argv[])
{
  app = binocle_app_new();
  binocle_app_init(&app);

  binocle_data_dir = binocle_sdl_assets_dir();

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

  char filename[1024];
  sprintf(filename, "%s%s", binocle_data_dir, "binocle-logo-full.png");
  image = binocle_image_load(filename);
  material = binocle_material_new();
  material->albedo_texture = image;
  material->shader = default_shader;
  sprite = binocle_sprite_from_material(material);

  binocle_gd_setup_default_pipeline(&gd, DESIGN_WIDTH, DESIGN_HEIGHT, default_shader);

  // Clear screen action for the actual screen
  binocle_color clear_color = binocle_color_green();
  binocle_pass_action default_action = {
    .colors[0] = {
      .action = BINOCLE_ACTION_CLEAR,
      .value = {
        .r = clear_color.r,
        .g = clear_color.g,
        .b = clear_color.b,
        .a = clear_color.a,
      }
    }
  };
  state.display.action = default_action;

  // Pipeline state object for the screen (default) pass
  state.display.pip = binocle_backend_make_pipeline(&(binocle_pipeline_desc){
    .layout = {
      .attrs = {
        [0] = { .format = BINOCLE_VERTEXFORMAT_FLOAT3 }, // position
        [1] = { .format = BINOCLE_VERTEXFORMAT_FLOAT4 }, // color
        [2] = { .format = BINOCLE_VERTEXFORMAT_FLOAT2 }, // texture uv
      }
    },
    .shader = screen_shader,
    .index_type = BINOCLE_INDEXTYPE_UINT16,
#if !defined(BINOCLE_GL)
    .depth = {
      .pixel_format = BINOCLE_PIXELFORMAT_NONE,
      .compare = BINOCLE_COMPAREFUNC_NEVER,
      .write_enabled = false,
    },
    .stencil = {
      .enabled = false,
    },
#endif
    .colors = {
#ifdef BINOCLE_GL
      [0] = { .pixel_format = BINOCLE_PIXELFORMAT_RGBA8 }
#else
      [0] = { .pixel_format = BINOCLE_PIXELFORMAT_BGRA8 }
#endif
    }
  });

  float vertices[] = {
    /* pos                  color                       uvs */
    -1.0f, -1.0f, 0.0f,    1.0f, 1.0f, 1.0f, 1.0f,     0.0f, 0.0f,
    1.0f, -1.0f, 0.0f,    1.0f, 1.0f, 1.0f, 1.0f,     1.0f, 0.0f,
    1.0f,  1.0f, 0.0f,    1.0f, 1.0f, 1.0f, 1.0f,     1.0f, 1.0f,
    -1.0f,  1.0f, 0.0f,    1.0f, 1.0f, 1.0f, 1.0f,     0.0f, 1.0f,
  };
  binocle_buffer_desc vbuf_desc = {
    .data = BINOCLE_RANGE(vertices)
  };
  binocle_buffer vbuf = binocle_backend_make_buffer(&vbuf_desc);

  uint16_t indices[] = {
    0, 1, 2,  0, 2, 3,
  };
  binocle_buffer_desc ibuf_desc = {
    .type = BINOCLE_BUFFERTYPE_INDEXBUFFER,
    .data = BINOCLE_RANGE(indices)
  };
  binocle_buffer ibuf = binocle_backend_make_buffer(&ibuf_desc);

  state.display.bind = (binocle_bindings){
    .vertex_buffers = {
      [0] = vbuf,
    },
    .index_buffer = ibuf,
    .fs_images = {
//      [0] = render_target,
    }
  };

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


