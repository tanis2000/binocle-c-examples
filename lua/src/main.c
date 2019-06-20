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
#include "sys_config.h"

#if defined(__IPHONEOS__)
#define DESIGN_WIDTH 812
#define DESIGN_HEIGHT 345
#else // defined(__IPHONEOS__)
#define DESIGN_WIDTH 320
#define DESIGN_HEIGHT 240
#endif // defined(__IPHONEOS__)

binocle_window window;
binocle_input input;
binocle_viewport_adapter adapter;
binocle_camera camera;
binocle_gd gd;
binocle_bitmapfont *font;
binocle_image font_image;
binocle_texture font_texture;
binocle_material font_material;
binocle_sprite font_sprite;
kmVec2 font_sprite_pos;
binocle_sprite_batch sprite_batch;
binocle_shader shader;
uint64_t last_fps;
float frame_counter;
char *binocle_data_dir = NULL;
binocle_app app;
binocle_lua lua;

int lua_set_globals() {
  lua_pushlightuserdata(lua.L, (void *)&gd);
  lua_setglobal(lua.L, "gd");

  lua_pushlightuserdata(lua.L, (void *)&sprite_batch);
  lua_setglobal(lua.L, "sprite_batch");

  lua_pushlightuserdata(lua.L, (void *)&adapter.viewport);
  lua_setglobal(lua.L, "viewport");

  lua_pushlightuserdata(lua.L, (void *)&camera);
  lua_setglobal(lua.L, "camera");

  lua_pushlightuserdata(lua.L, (void *)&input);
  lua_setglobal(lua.L, "input");

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
  binocle_window_begin_frame(&window);
  float dt = binocle_window_get_frame_time(&window) / 1000.0f;

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

  lua_on_update(dt);

  kmMat4 view_matrix;
  kmMat4Identity(&view_matrix);
  binocle_sprite_batch_end(&sprite_batch, binocle_camera_get_viewport(camera));

  char fps_string[256];
  sprintf(fps_string, "FPS: %lld", last_fps);
  binocle_bitmapfont_draw_string(font, fps_string, 16, &gd, 10, window.original_height - 32, adapter.viewport, binocle_color_black(), view_matrix);

  binocle_lua_check_scripts_modification_time(&lua, "assets");

  binocle_window_refresh(&window);
  binocle_window_end_frame(&window);

  frame_counter += dt;
  if (frame_counter >= 1.0f) {
    frame_counter = 0;
    last_fps = binocle_window_get_fps(&window);
  }
  //binocle_log_info("FPS: %d", binocle_window_get_fps(&window));
}

int main(int argc, char *argv[])
{
  app = binocle_app_new();
  binocle_app_init(&app);

  binocle_data_dir = binocle_sdl_assets_dir();

  window = binocle_window_new(DESIGN_WIDTH, DESIGN_HEIGHT, "Binocle Lua");
  binocle_window_set_background_color(&window, binocle_color_azure());
  adapter = binocle_viewport_adapter_new(window, BINOCLE_VIEWPORT_ADAPTER_KIND_SCALING, BINOCLE_VIEWPORT_ADAPTER_SCALING_TYPE_PIXEL_PERFECT, window.original_width, window.original_height, window.original_width, window.original_height);
  camera = binocle_camera_new(&adapter);
  input = binocle_input_new();

  char vert[1024];
  sprintf(vert, "%s%s", binocle_data_dir, "default.vert");
  char frag[1024];
  sprintf(frag, "%s%s", binocle_data_dir, "default.frag");
  shader = binocle_shader_load_from_file(vert, frag);

  char font_filename[1024];
  sprintf(font_filename, "%s%s", binocle_data_dir, "font.fnt");
  font = binocle_bitmapfont_from_file(font_filename, true);

  char font_image_filename[1024];
  sprintf(font_image_filename, "%s%s", binocle_data_dir, "font.png");
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
  free(binocle_data_dir);
  binocle_app_destroy(&app);
}


