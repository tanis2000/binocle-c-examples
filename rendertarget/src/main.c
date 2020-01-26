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

binocle_window window;
binocle_input input;
binocle_viewport_adapter adapter;
binocle_camera camera;
binocle_gd gd;
binocle_shader shader;
char *binocle_data_dir = NULL;
binocle_app app;
binocle_image image;
binocle_texture texture;
binocle_sprite sprite;
binocle_material material;
binocle_render_target render_target;
binocle_shader screen_shader;

void main_loop() {
  binocle_window_begin_frame(&window);
  float dt = binocle_window_get_frame_time(&window) / 1000.0f;

  binocle_input_update(&input);

  if (input.resized) {
    window.width = input.newWindowSize.x;
    window.height = input.newWindowSize.y;
    input.resized = false;
  }

  // Set the render target we will draw to
  binocle_gd_set_render_target(render_target);

  // Clear the render target with red color
  binocle_window_clear(&window);

  // By default we scale our logo by 1/3
  kmVec2 scale;
  scale.x = 0.3f;
  scale.y = 0.3f;

  // Create a viewport that corresponds to the size of our render target
  kmAABB2 viewport;
  viewport.min.x = 0;
  viewport.min.y = 0;
  viewport.max.x = DESIGN_WIDTH;
  viewport.max.y = DESIGN_HEIGHT;

  // A simple identity matrix
  kmMat4 identity_matrix;
  kmMat4Identity(&identity_matrix);

  // Center the logo in the render target
  uint64_t x = (uint64_t)((DESIGN_WIDTH - (sprite.material->texture->width * scale.x)) / 2.0f);
  uint64_t y = (uint64_t)((DESIGN_HEIGHT - (sprite.material->texture->height * scale.x)) / 2.0f);

  // Draw the logo to the render target
  binocle_sprite_draw(&sprite, &gd, x, y, &viewport, 0, &scale, &camera);

  // Create a viewport that corresponds to the size of the window we want to blit to
  kmAABB2 window_viewport;
  window_viewport.min.x = 0;
  window_viewport.min.y = 0;
  window_viewport.max.x = window.width;
  window_viewport.max.y = window.height;

  float vp_x = (window.width - (float)DESIGN_WIDTH) / 2.0f;
  float vp_y = (window.height - (float)DESIGN_HEIGHT) / 2.0f;
  binocle_gd_clear_render_target();
  binocle_gd_clear(binocle_color_azure());
  binocle_gd_apply_viewport(window_viewport);
  binocle_gd_apply_shader(&gd, screen_shader);
  binocle_gd_set_uniform_float2(screen_shader, "resolution", DESIGN_WIDTH,
                                DESIGN_HEIGHT);
  binocle_gd_set_uniform_mat4(screen_shader, "transform", identity_matrix);
  binocle_gd_set_uniform_float2(screen_shader, "scale", 1.0f, 1.0f);
  binocle_gd_set_uniform_float2(screen_shader, "viewport", vp_x, vp_y);
  binocle_gd_draw_quad_to_screen(screen_shader, render_target);

  binocle_window_refresh(&window);
  binocle_window_end_frame(&window);
}

int main(int argc, char *argv[])
{
  app = binocle_app_new();
  binocle_app_init(&app);

  binocle_data_dir = binocle_sdl_assets_dir();

  window = binocle_window_new(DESIGN_WIDTH, DESIGN_HEIGHT, "Binocle Render Target Example");
  binocle_window_set_background_color(&window, binocle_color_red());
  adapter = binocle_viewport_adapter_new(window, BINOCLE_VIEWPORT_ADAPTER_KIND_SCALING, BINOCLE_VIEWPORT_ADAPTER_SCALING_TYPE_PIXEL_PERFECT, window.original_width, window.original_height, window.original_width, window.original_height);
  camera = binocle_camera_new(&adapter);
  input = binocle_input_new();

  char vert[1024];
  sprintf(vert, "%s%s", binocle_data_dir, "default.vert");
  char frag[1024];
  sprintf(frag, "%s%s", binocle_data_dir, "default.frag");
  shader = binocle_shader_load_from_file(vert, frag);

  sprintf(vert, "%s%s", binocle_data_dir, "screen.vert");
  sprintf(frag, "%s%s", binocle_data_dir, "screen.frag");
  screen_shader = binocle_shader_load_from_file(vert, frag);

  char filename[1024];
  sprintf(filename, "%s%s", binocle_data_dir, "binocle-logo-full.png");
  image = binocle_image_load(filename);
  texture = binocle_texture_from_image(image);
  material = binocle_material_new();
  material.texture = &texture;
  material.shader = &shader;
  sprite = binocle_sprite_from_material(&material);

  render_target = binocle_gd_create_render_target(DESIGN_WIDTH, DESIGN_HEIGHT, true, GL_RGBA8);

  gd = binocle_gd_new();
  binocle_gd_init(&gd);

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  while (!input.quit_requested) {
    main_loop();
  }
#endif
  binocle_log_info("Quit requested");
  free(binocle_data_dir);
  binocle_app_destroy(&app);
}


