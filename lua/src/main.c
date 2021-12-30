//
//  Binocle
//  Copyright(C)2015-2019 Valerio Santinelli
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
#include <binocle_input_wrap.h>
#include <binocle_camera_wrap.h>
#include <binocle_viewport_adapter_wrap.h>

#define BINOCLE_MATH_IMPL
#include "binocle_math.h"
#include "binocle_gd.h"
#include "binocle_log.h"
#include "binocle_bitmapfont.h"
#include "binocle_ecs.h"
#include "binocle_app.h"
#include "sys_config.h"

#if defined(__IPHONEOS__)
#define DESIGN_WIDTH 812
#define DESIGN_HEIGHT 345
#else // defined(__IPHONEOS__)
#define DESIGN_WIDTH 320
#define DESIGN_HEIGHT 240
#endif // defined(__IPHONEOS__)

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

binocle_window *window;
binocle_input input;
binocle_viewport_adapter *adapter;
binocle_camera camera;
binocle_gd gd;
binocle_bitmapfont *font;
binocle_image font_texture;
binocle_material *font_material;
binocle_sprite *font_sprite;
kmVec2 font_sprite_pos;
binocle_sprite_batch sprite_batch;
binocle_shader default_shader;
binocle_shader screen_shader;
uint64_t last_fps;
float frame_counter;
char *binocle_data_dir = NULL;
binocle_app app;
binocle_lua lua;
binocle_shader *shader;

int lua_set_globals() {
  lua_pushlightuserdata(lua.L, (void *)&gd);
  lua_setglobal(lua.L, "gd");

  lua_pushlightuserdata(lua.L, (void *)&sprite_batch);
  lua_setglobal(lua.L, "sprite_batch");

  lua_pushlightuserdata(lua.L, (void *)&adapter->viewport);
  lua_setglobal(lua.L, "viewport");

//  lua_pushlightuserdata(lua.L, (void *)&camera);
//  lua_setglobal(lua.L, "camera");

  l_binocle_camera_t *l_camera = lua_newuserdata(lua.L, sizeof(l_binocle_camera_t));
  lua_getfield(lua.L, LUA_REGISTRYINDEX, "binocle_camera");
  lua_setmetatable(lua.L, -2);
  SDL_memset(l_camera, 0, sizeof(*l_camera));
  l_camera->camera = SDL_malloc(sizeof(binocle_camera));
  SDL_memcpy(l_camera->camera, &camera, sizeof(binocle_camera));
//  lua_pushlightuserdata(lua.L, (void *)l_camera);
  lua_setglobal(lua.L, "camera_mgr");

//  lua_pushlightuserdata(lua.L, (void *)&input);
//  lua_setglobal(lua.L, "input_mgr");

  l_binocle_input_t *l_input = lua_newuserdata(lua.L, sizeof(l_binocle_input_t));
  lua_getfield(lua.L, LUA_REGISTRYINDEX, "binocle_input");
  lua_setmetatable(lua.L, -2);
  SDL_memset(l_input, 0, sizeof(*l_input));
  l_input->input = SDL_malloc(sizeof(binocle_input));
  SDL_memcpy(l_input->input, &input, sizeof(binocle_input));
//  lua_pushlightuserdata(lua.L, (void *)l_input);
  lua_setglobal(lua.L, "input_mgr");

  return 0;
}

int lua_on_update(float dt) {
  lua_getglobal(lua.L, "on_update");
  lua_pushnumber(lua.L, dt);
  int result = lua_pcall(lua.L, 1, 0, 0);
  if (result) {
    binocle_log_error("Failed to run function: %s\n", lua_tostring(lua.L, -1));
    return 1;
  }
  return 0;
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

//  binocle_window_clear(&window);

  lua_on_update(dt);

  kmMat4 view_matrix;
  kmMat4Identity(&view_matrix);
  binocle_sprite_batch_end(&sprite_batch, binocle_camera_get_viewport(camera));

  char fps_string[256];
  sprintf(fps_string, "FPS: %lld", last_fps);
  binocle_bitmapfont_draw_string(font, fps_string, 16, &gd, 10, window->original_height - 32, adapter->viewport, binocle_color_black(), view_matrix);

  binocle_lua_check_scripts_modification_time(&lua, "assets");

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

  binocle_data_dir = binocle_sdl_assets_dir();

  window = binocle_window_new(DESIGN_WIDTH, DESIGN_HEIGHT, "Binocle Lua");
  binocle_window_set_background_color(window, binocle_color_azure());
  binocle_window_set_minimum_size(window, DESIGN_WIDTH, DESIGN_HEIGHT);
  adapter = binocle_viewport_adapter_new(window, BINOCLE_VIEWPORT_ADAPTER_KIND_SCALING, BINOCLE_VIEWPORT_ADAPTER_SCALING_TYPE_PIXEL_PERFECT, window->original_width, window->original_height, window->original_width, window->original_height);
  camera = binocle_camera_new(adapter);
  input = binocle_input_new();
  gd = binocle_gd_new();
  binocle_gd_init(&gd, window);

  binocle_data_dir = binocle_sdl_assets_dir();
  binocle_log_info("Current base path: %s", binocle_data_dir);

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

  lua = binocle_lua_new();
  binocle_lua_init(&lua);

  lua_set_globals();

  char main_lua[1024];
  sprintf(main_lua, "%s%s", binocle_data_dir, "main.lua");
  binocle_lua_run_script(&lua, main_lua);

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  while (!input.quit_requested) {
    main_loop();
  }
#endif
  binocle_log_info("Quit requested");
  binocle_image_destroy(font_texture);
  binocle_material_destroy(font_material);
  binocle_sprite_destroy(font_sprite);
  binocle_gd_destroy(&gd);
  binocle_app_destroy(&app);
}


