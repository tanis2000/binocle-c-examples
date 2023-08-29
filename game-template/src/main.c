//
//  Binocle Game Template
//  Copyright(C)2015-2023 Valerio Santinelli
//

#include <stdio.h>
#include <stdlib.h>

#if defined(__WIN32__)
#include <random>
#endif
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
#include <binocle_array.h>

#define BINOCLE_MATH_IMPL

#include "binocle_math.h"
#include "binocle_gd.h"
#include "binocle_log.h"
#include "binocle_bitmapfont.h"
#include "binocle_app.h"
#include "types.h"
#include "cooldown.h"
#include "entity.h"
#include "cache.h"
#include "en/hero.h"
#include "level.h"
#include "game_camera.h"

#define CUTE_TILED_IMPLEMENTATION

#include "cute_tiled.h"

#define STB_DS_IMPLEMENTATION

#include "stb_ds.h"
#include "gui/gui.h"
#include "gui/debug_gui.h"

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
char *binocle_data_dir = NULL;
binocle_bitmapfont *font;
sg_image font_image;
binocle_material *font_material;
binocle_sprite *font_sprite;
kmVec2 font_sprite_pos;
sg_shader screen_shader;

entity_t entities[MAX_ENTITIES];
game_t game;

void create_entity() {
  game.hero = hero_new();
}

void create_game_camera() {
  game.game_camera = game_camera_new();
  game_camera_track_entity(&game.game_camera, game.hero, false, 1.0f);
  game_camera_center_on_target(&game.game_camera);
}

void main_loop() {
  binocle_window_begin_frame(game.gfx.window);
  float dt = (float) binocle_window_get_frame_time(game.gfx.window) / 1000.0f;
  game.dt = dt;
  game.elapsed_time += dt;

  binocle_input_update(&game.input);
  gui_pass_input_to_imgui(game.gui.debug_gui_handle, &game.input);
  gui_pass_input_to_imgui(game.gui.game_gui_handle, &game.input);

  if (game.input.resized) {
    kmVec2 oldWindowSize = {.x = game.gfx.window->width, .y = game.gfx.window->height};
    game.gfx.window->width = game.input.newWindowSize.x;
    game.gfx.window->height = game.input.newWindowSize.y;
    binocle_viewport_adapter_reset(game.gfx.camera.viewport_adapter, oldWindowSize, game.input.newWindowSize);
    gui_recreate_imgui_render_target(game.gui.debug_gui_handle, game.gfx.window->width, game.gfx.window->height);
    gui_set_viewport(game.gui.debug_gui_handle, game.gfx.window->width, game.gfx.window->height);
    game.input.resized = false;
  }

  cooldown_system_update(&game.pools, dt);

  if (binocle_input_is_key_down(game.input, KEY_1)) {
    game.debug = !game.debug;
  }

  if (binocle_input_is_key_down(game.input, KEY_2)) {
    binocle_audio_music *music = &game.cache.music[0].music;
    //binocle_audio_stop_music_stream(music);
    binocle_audio_seek_music_stream(music, 0);
    //binocle_audio_play_music_stream(music);
  }


//  if (!cooldown_has(&game.pools, "new_entity")) {
//    // Add one more sprite every 3 seconds
//    uint32_t old_number_of_sprites = number_of_sprites;
//    number_of_sprites += 1;
//    binocle_array_grow(entities, number_of_sprites);
//    for (int i = old_number_of_sprites ; i < number_of_sprites ; i++) {
//      create_entity(&entities[i]);
//    }
//    cooldown_set(&game.pools, "new_entity", 3, NULL);
//  }


  kmAABB2 viewport;
  kmAABB2Initialize(&viewport, &(kmVec2){.x = DESIGN_WIDTH/2, .y = DESIGN_HEIGHT/2}, DESIGN_WIDTH, DESIGN_HEIGHT, 0);
  binocle_sprite_batch_begin(&game.gfx.sprite_batch, viewport,
                             BINOCLE_SPRITE_SORT_MODE_DEFERRED, &game.gfx.default_shader,
                             binocle_camera_get_transform_matrix(&game.gfx.camera));

  kmVec2 scale;
  scale.x = 1.0f;
  scale.y = 1.0f;

  for (int i = 0; i < game.num_entities; i++) {
    entity_update(&game.entities[i], dt);
  }
  hero_input_update(game.hero, dt);
  for (int i = 0; i < game.num_entities; i++) {
    entity_post_update(&game.entities[i], dt);
  }
  game_camera_update(&game.game_camera, dt);
  level_render(&game.level);
  for (int i = 0; i < game.num_entities; i++) {
    entity_draw(&game.entities[i]);
  }
  game_camera_post_update(&game.game_camera);
  debug_gui_draw(dt);
  entity_system_update();

  for (int i = 0 ; i < game.cache.music_num ; i++) {
    binocle_audio_music *music = &game.cache.music[i].music;
    binocle_audio_update_music_stream(&game.audio, music);
  }

  kmMat4 view_matrix;
  kmMat4Identity(&view_matrix);
  // Gets the viewport calculated by the adapter
  kmAABB2 screen_viewport = binocle_viewport_adapter_get_viewport(*game.gfx.camera.viewport_adapter);
  //binocle_sprite_draw(font_sprite, &gd, (uint64_t)font_sprite_pos.x, (uint64_t)font_sprite_pos.y, adapter.viewport);


  char fps[256] = {0};
  sprintf(fps, "FPS:%llu", binocle_window_get_fps(game.gfx.window));
  binocle_bitmapfont_draw_string(font, fps, 16, &game.gfx.gd, 20, 20, viewport, binocle_color_white(), view_matrix, 1);

  binocle_sprite_batch_end(&game.gfx.sprite_batch, viewport);

  binocle_gd_render_offscreen(&game.gfx.gd);
  binocle_gd_render_flat(&game.gfx.gd);
  binocle_gd_render_screen(&game.gfx.gd, game.gfx.window, DESIGN_WIDTH, DESIGN_HEIGHT, screen_viewport, game.gfx.camera.viewport_adapter->scale_matrix, game.gfx.camera.viewport_adapter->inverse_multiplier);

  if (game.debug) {
    struct gui_t *gui = gui_resources_get_gui("debug");
    gui_set_context(gui);
    gui_render_to_screen(gui, &game.gfx.gd, game.gfx.window, DESIGN_WIDTH, DESIGN_HEIGHT, screen_viewport,
                         game.gfx.camera.viewport_adapter->scale_matrix,
                         game.gfx.camera.viewport_adapter->inverse_multiplier);
  }
//  gui = gui_resources_get_gui("game");
//  gui_set_context(gui);
//  gui_render_to_screen(gui, &game.gfx.gd, game.gfx.window, DESIGN_WIDTH, DESIGN_HEIGHT, screen_viewport, game.gfx.camera.viewport_adapter->scale_matrix, game.gfx.camera.viewport_adapter->inverse_multiplier);

  binocle_window_refresh(game.gfx.window);
  binocle_window_end_frame(game.gfx.window);
  //binocle_log_info("FPS: %d", binocle_window_get_fps(&window));
}

int main(int argc, char *argv[]) {
  game = (game_t) {0};
  entity_system_init();

  binocle_app_desc_t app_desc = {0};
  app = binocle_app_new();
  binocle_app_init(&app, &app_desc);

  binocle_data_dir = binocle_sdl_assets_dir();
  binocle_log_info("Current base path: %s", binocle_data_dir);

  game.gfx.window = binocle_window_new(DESIGN_WIDTH * SCALE, DESIGN_HEIGHT * SCALE, "Binocle Game Template");
  binocle_window_set_background_color(game.gfx.window, binocle_color_azure());
  binocle_window_set_minimum_size(game.gfx.window, DESIGN_WIDTH, DESIGN_HEIGHT);
  binocle_viewport_adapter *adapter = binocle_viewport_adapter_new(game.gfx.window, BINOCLE_VIEWPORT_ADAPTER_KIND_SCALING,
                                         BINOCLE_VIEWPORT_ADAPTER_SCALING_TYPE_PIXEL_PERFECT, DESIGN_WIDTH,
                                         DESIGN_HEIGHT, DESIGN_WIDTH, DESIGN_HEIGHT);
  game.gfx.camera = binocle_camera_new(adapter);
  game.input = binocle_input_new();
  game.gfx.gd = binocle_gd_new();
  binocle_gd_init(&game.gfx.gd, game.gfx.window);


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
    .vs.bytecode.ptr = default_vs_bytecode,
    .vs.bytecode.size = sizeof(default_vs_bytecode),
#endif
    .attrs = {
      [0] = {.name = "vertexPosition"},
      [1] = {.name = "vertexColor"},
      [2] = {.name = "vertexTCoord"},
    },
    .vs.uniform_blocks[0] = {
      .size = sizeof(default_shader_params_t),
      .uniforms = {
        [0] = {.name = "projectionMatrix", .type = SG_UNIFORMTYPE_MAT4},
        [1] = {.name = "viewMatrix", .type = SG_UNIFORMTYPE_MAT4},
        [2] = {.name = "modelMatrix", .type = SG_UNIFORMTYPE_MAT4},
      }
    },
#ifdef BINOCLE_GL
    .fs.source = shader_fs_src,
#else
    .fs.bytecode.ptr = default_fs_bytecode,
    .fs.bytecode.size = sizeof(default_fs_bytecode),
#endif
    .fs.images[0] = {.name = "tex0", .image_type = SG_IMAGETYPE_2D},
  };
  game.gfx.default_shader = sg_make_shader(&default_shader_desc);

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
        [0] = {.name = "transform", .type = SG_UNIFORMTYPE_MAT4},
      },
    },
#ifdef BINOCLE_GL
    .fs.source = screen_shader_fs_src,
#else
    .fs.byte_code = screen_fs_bytecode,
    .fs.byte_code_size = sizeof(screen_fs_bytecode),
#endif
    .fs.images[0] = {.name = "tex0", .image_type = SG_IMAGETYPE_2D},
    .fs.uniform_blocks[0] = {
      .size = sizeof(screen_shader_fs_params_t),
      .uniforms = {
        [0] = {.name = "resolution", .type = SG_UNIFORMTYPE_FLOAT2},
        [1] = {.name = "scale", .type = SG_UNIFORMTYPE_FLOAT2},
        [2] = {.name = "viewport", .type = SG_UNIFORMTYPE_FLOAT2},
      },
    },
  };
  screen_shader = sg_make_shader(&screen_shader_desc);

  cache_system_init();

  char font_filename[1024];
  sprintf(font_filename, "%s%s", binocle_data_dir, "font.fnt");
  font = binocle_bitmapfont_from_file(font_filename, true);

  char font_image_filename[1024];
  sprintf(font_image_filename, "%s%s", binocle_data_dir, "font.png");
  font_image = binocle_image_load(font_image_filename);
  font_material = binocle_material_new();
  font_material->albedo_texture = font_image;
  font_material->shader = game.gfx.default_shader;
  font->material = font_material;
  font_sprite = binocle_sprite_from_material(font_material);
  font_sprite_pos.x = 0;
  font_sprite_pos.y = -256;

  game.gfx.sprite_batch = binocle_sprite_batch_new();
  game.gfx.sprite_batch.gd = &game.gfx.gd;

  binocle_gd_setup_default_pipeline(&game.gfx.gd, DESIGN_WIDTH, DESIGN_HEIGHT, game.gfx.default_shader, screen_shader);
  binocle_gd_setup_flat_pipeline(&game.gfx.gd);

  gui_resources_setup();
  game.gui.debug_gui_handle = gui_resources_create_gui("debug");
  gui_init_imgui(game.gui.debug_gui_handle, game.gfx.window->width, game.gfx.window->height, game.gfx.window->width, game.gfx.window->height);
  gui_setup_screen_pipeline(game.gui.debug_gui_handle, screen_shader, false);

  game.gui.game_gui_handle = gui_resources_create_gui("game");
  gui_set_apply_scissor(game.gui.game_gui_handle, true);
  gui_set_viewport_adapter(game.gui.game_gui_handle, binocle_camera_get_viewport_adapter(game.gfx.camera));
  gui_init_imgui(game.gui.game_gui_handle, DESIGN_WIDTH, DESIGN_HEIGHT, game.gfx.window->width, game.gfx.window->height);
  gui_setup_screen_pipeline(game.gui.game_gui_handle, screen_shader, true);

  game.audio = binocle_audio_new();
  binocle_audio_init(&game.audio);
  binocle_audio_music theme = cache_load_music("/assets/music/theme.mp3");
  binocle_audio_play_music_stream(&theme);
  binocle_audio_set_music_volume(&theme, 0);

  game.level = (level_t) {0};
  level_load_tilemap(&game.level, "maps/map01.json");

  cooldown_system_init(&game.pools, 16);

  spawner_t *hs = level_get_hero_spawner(&game.level);
  create_entity();
  entity_set_pos_grid(game.hero, hs->cx + 10, hs->cy);
  create_game_camera();

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  while (!game.input.quit_requested) {
    main_loop();
  }
#endif
  binocle_log_info("Quit requested");
  free(binocle_data_dir);
  binocle_app_destroy(&app);
  cooldown_system_shutdown(&game.pools);
}


