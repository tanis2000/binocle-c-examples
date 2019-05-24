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
#define BINOCLE_MATH_IMPL
#include "binocle_math.h"
#include "binocle_gd.h"
#include "binocle_log.h"
#include "binocle_bitmapfont.h"
#include "binocle_collision.h"
#include "sys_config.h"

//#define GAMELOOP 1

//#define HITBOX_PLAYER
#define CIRCLE_PLAYER
//#define HITBOX_ENEMY
#define CIRCLE_ENEMY

binocle_window window;
binocle_input input;
binocle_viewport_adapter adapter;
binocle_camera camera;
binocle_sprite player;
kmVec2 player_pos;
binocle_gd gd;
binocle_bitmapfont *font;
binocle_image font_image;
binocle_texture font_texture;
binocle_material font_material;
binocle_sprite font_sprite;
kmVec2 font_sprite_pos;
binocle_collider_hitbox player_hitbox;
binocle_collider_hitbox enemy_hitbox;
binocle_sprite enemy;
kmVec2 enemy_pos;
binocle_collider_circle player_hitbox_c;
binocle_collider_circle enemy_hitbox_c;

binocle_spatial_hash spatial_hash;
binocle_collider player_collider;
binocle_collider enemy_collider;
binocle_collider_ptr_array_t colliding_colliders;

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


  if (binocle_input_is_key_pressed(input, KEY_RIGHT)) {
    player_pos.x += 50 * (1.0/window.frame_time);
  } else if (binocle_input_is_key_pressed(input, KEY_LEFT)) {
    player_pos.x -= 50 * (1.0/window.frame_time);
  }

  if (binocle_input_is_key_pressed(input, KEY_UP)) {
    player_pos.y += 50 * (1.0/window.frame_time);
  } else if (binocle_input_is_key_pressed(input, KEY_DOWN)) {
    player_pos.y -= 50 * (1.0/window.frame_time);
  }

  player_hitbox.aabb.min.x = player_pos.x;
  player_hitbox.aabb.min.y = player_pos.y;
  player_hitbox.aabb.max.x = player_pos.x + player.material->texture->width;
  player_hitbox.aabb.max.y = player_pos.y + player.material->texture->height;

  player_hitbox_c.center.x = player_pos.x + (float)player.material->texture->width / 2.0f;
  player_hitbox_c.center.y = player_pos.y + (float)player.material->texture->height / 2.0f;

  binocle_window_clear(&window);
  kmVec2 scale;
  scale.x = 1.0f;
  scale.y = 1.0f;
  kmMat4 view_matrix;
  kmMat4Identity(&view_matrix);
  binocle_sprite_draw(player, &gd, (uint64_t)player_pos.x, (uint64_t)player_pos.y, adapter.viewport, 0, scale, &camera);
  binocle_sprite_draw(enemy, &gd, (uint64_t)enemy_pos.x, (uint64_t)enemy_pos.y, adapter.viewport, 0, scale, &camera);

#if defined(HITBOX_PLAYER) && defined(HITBOX_ENEMY)
  bool collides = binocle_collide_hitbox_to_hitbox(&player_hitbox, &enemy_hitbox);

  binocle_gd_draw_rect_outline(&gd, player_hitbox.aabb, collides ? binocle_color_red() : binocle_color_green(), adapter.viewport, view_matrix);
  binocle_gd_draw_rect_outline(&gd, enemy_hitbox.aabb, collides ? binocle_color_red() : binocle_color_green(), adapter.viewport, view_matrix);
#elif defined(CIRCLE_PLAYER) && defined(HITBOX_ENEMY)
  bool collides = binocle_collide_circle_to_hitbox(&player_hitbox_c, &enemy_hitbox);

  binocle_gd_draw_circle(&gd, player_hitbox_c.center, player_hitbox_c.radius, collides ? binocle_color_red_translucent() : binocle_color_green_translucent(), adapter.viewport, view_matrix);
  binocle_gd_draw_rect_outline(&gd, enemy_hitbox.aabb, collides ? binocle_color_red() : binocle_color_green(), adapter.viewport, view_matrix);
#elif defined(CIRCLE_PLAYER) && defined(CIRCLE_ENEMY)
  bool collides = false;

  binocle_spatial_hash_update_body(&spatial_hash, &player_collider);
  binocle_spatial_hash_update_body(&spatial_hash, &enemy_collider);

  binocle_spatial_hash_get_all_bodies_sharing_cells_with_body(&spatial_hash, &player_collider, &colliding_colliders, 0);
  if (da_count(colliding_colliders) > 0) {
    collides = binocle_collide_circle_to_circle(&player_hitbox_c, &enemy_hitbox_c);
  }

  binocle_gd_draw_circle(&gd, player_hitbox_c.center, player_hitbox_c.radius, collides ? binocle_color_red_translucent() : binocle_color_green_translucent(), adapter.viewport, view_matrix);
  binocle_gd_draw_circle(&gd, enemy_hitbox_c.center, enemy_hitbox_c.radius, collides ? binocle_color_red_translucent() : binocle_color_green_translucent(), adapter.viewport, view_matrix);
#endif

  /*
  kmVec2 start;
  start.x = 320.0f/2 - 20;
  start.y = 240.0f/2;
  kmVec2 end;
  end.x = 320.0f/2 + 20;
  end.y = 240.0f/2;
  binocle_gd_draw_line(&gd, start, end, binocle_color_red(), adapter.viewport, view_matrix);
   */
  binocle_bitmapfont_draw_string(font, "TEST", 12, &gd, 20, 20, adapter.viewport, binocle_color_white(), view_matrix);
  //binocle_sprite_draw(font_sprite, &gd, (uint64_t)font_sprite_pos.x, (uint64_t)font_sprite_pos.y, adapter.viewport);
  binocle_window_refresh(&window);
  binocle_window_end_frame(&window);
  //binocle_log_info("FPS: %d", binocle_window_get_fps(&window));
}

int main(int argc, char *argv[])
{
  binocle_sdl_init();
  window = binocle_window_new(320, 240, "Binocle Test Game");
  binocle_window_set_background_color(&window, binocle_color_azure());
  adapter = binocle_viewport_adapter_new(window, BINOCLE_VIEWPORT_ADAPTER_KIND_SCALING, BINOCLE_VIEWPORT_ADAPTER_SCALING_TYPE_PIXEL_PERFECT, window.original_width, window.original_height, window.original_width, window.original_height);
  camera = binocle_camera_new(&adapter);
  input = binocle_input_new();
  binocle_shader_init_defaults();
  char filename[1024];
  sprintf(filename, "%s%s", BINOCLE_DATA_DIR, "wabbit_alpha.png");
  binocle_image image = binocle_image_load(filename);
  binocle_texture texture = binocle_texture_from_image(image);
  char vert[1024];
  sprintf(vert, "%s%s", BINOCLE_DATA_DIR, "default.vert");
  char frag[1024];
  sprintf(frag, "%s%s", BINOCLE_DATA_DIR, "default.frag");
  binocle_shader shader = binocle_shader_load_from_file(vert, frag);
  binocle_material material = binocle_material_new();
  material.texture = &texture;
  material.shader = &shader;
  player = binocle_sprite_from_material(&material);
  player_pos.x = 50;
  player_pos.y = 50;
  kmAABB2 player_aabb2;
  player_aabb2.min.x = player_pos.x;
  player_aabb2.min.y = player_pos.y;
  player_aabb2.max.x = player_pos.x + texture.width;
  player_aabb2.max.y = player_pos.y + texture.height;
  player_hitbox = binocle_collider_hitbox_new(player_aabb2);
  kmVec2 player_center;
  player_center.x = player_pos.x + (float)texture.width / 2;
  player_center.y = player_pos.y + (float)texture.height / 2;
  player_hitbox_c = binocle_collider_circle_new(texture.width > texture.height ? (texture.width / 2) : (texture.height / 2), player_center);

  sprintf(filename, "%s%s", BINOCLE_DATA_DIR, "enemy.png");
  binocle_image enemy_image = binocle_image_load(filename);
  binocle_texture enemy_texture = binocle_texture_from_image(enemy_image);
  binocle_material enemy_material = binocle_material_new();
  enemy_material.texture = &enemy_texture;
  enemy_material.shader = &shader;
  enemy = binocle_sprite_from_material(&enemy_material);
  enemy_pos.x = 100;
  enemy_pos.y = 50;
  kmAABB2 enemy_aabb2;
  enemy_aabb2.min.x = enemy_pos.x;
  enemy_aabb2.min.y = enemy_pos.y;
  enemy_aabb2.max.x = enemy_pos.x + enemy_texture.width;
  enemy_aabb2.max.y = enemy_pos.y + enemy_texture.height;
  enemy_hitbox = binocle_collider_hitbox_new(enemy_aabb2);
  kmVec2 enemy_center;
  enemy_center.x = enemy_pos.x + (float)enemy_texture.width / 2;
  enemy_center.y = enemy_pos.y + (float)enemy_texture.height / 2;
  enemy_hitbox_c = binocle_collider_circle_new(enemy_texture.width > enemy_texture.height ? (enemy_texture.width / 2) : (enemy_texture.height / 2), enemy_center);

  spatial_hash = binocle_spatial_hash_new(1000, 1000, 32);
  player_collider = binocle_collider_new();
  player_collider.circle = &player_hitbox_c;

  enemy_collider = binocle_collider_new();
  enemy_collider.circle = &enemy_hitbox_c;

  binocle_spatial_hash_add_body(&spatial_hash, &player_collider);
  binocle_spatial_hash_add_body(&spatial_hash, &enemy_collider);

  da_init(colliding_colliders);

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


