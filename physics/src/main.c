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
#include <dgNewton/Newton.h>

#define DESIGN_WIDTH 320
#define DESIGN_HEIGHT 240

binocle_window *window;
binocle_input input;
binocle_viewport_adapter *adapter;
binocle_camera camera;
binocle_gd gd;
binocle_shader *shader;
char *binocle_data_dir = NULL;
binocle_app app;
binocle_image *image;
binocle_texture *texture;
binocle_sprite *sprite;
binocle_material *material;
binocle_render_target *render_target;
binocle_shader *screen_shader;

binocle_bitmapfont *font;
binocle_image *font_image;
binocle_texture *font_texture;
binocle_material *font_material;
binocle_sprite *font_sprite;
kmVec2 font_sprite_pos;

NewtonWorld *world;
NewtonBody *background_body;
NewtonBody *ball_body;
binocle_sprite *ball_sprite;
binocle_texture *ball_texture;
bool dragging_ball;
kmVec2 mouse_prev_pos;
kmVec2 mouse_world_pos;

void create_barriers() {
  kmMat4 identity;
  kmMat4Identity(&identity);

  kmMat4 bottom_offset;
  kmMat4Translation(&bottom_offset, DESIGN_WIDTH/2, -50, 0);
  NewtonCollision *coll_bottom = NewtonCreateBox(world, DESIGN_WIDTH, 100, 100, 0, &identity.mat[0]);

  kmMat4 top_offset;
  kmMat4Translation(&top_offset, DESIGN_WIDTH/2, DESIGN_HEIGHT + 50, 0);
  NewtonCollision *coll_top = NewtonCreateBox(world, DESIGN_WIDTH, 100, 100, 0, &identity.mat[0]);

  kmMat4 left_offset;
  kmMat4Translation(&left_offset, -50, DESIGN_HEIGHT/2, 0);
  NewtonCollision *coll_left = NewtonCreateBox(world, 100, DESIGN_HEIGHT, 100, 0, &identity.mat[0]);

  kmMat4 right_offset;
  kmMat4Translation(&right_offset, DESIGN_WIDTH + 50, DESIGN_HEIGHT/2, 0);
  NewtonCollision *coll_right = NewtonCreateBox(world, 100, DESIGN_HEIGHT, 100, 0, &identity.mat[0]);

  NewtonCreateDynamicBody(world, coll_bottom, &bottom_offset.mat[0]);
  NewtonCreateDynamicBody(world, coll_top, &top_offset.mat[0]);
  NewtonCreateDynamicBody(world, coll_left, &left_offset.mat[0]);
  NewtonCreateDynamicBody(world, coll_right, &right_offset.mat[0]);

  NewtonDestroyCollision(coll_bottom);
  NewtonDestroyCollision(coll_top);
  NewtonDestroyCollision(coll_left);
  NewtonDestroyCollision(coll_right);
}

NewtonBody *create_background() {
  dFloat points[4][3] =
    {
      {-100.0f, 0.0f,  100.0f},
      { 420.0f, 0.0f,  100.0f},
      { 420.0f, 0.0f, -100.0f},
      {-100.0f, 0.0f, -100.0f},
    };

  // crate a collision tree
  NewtonCollision* const collision = NewtonCreateTreeCollision (world, 0);

  // start building the collision mesh
  NewtonTreeCollisionBeginBuild (collision);

  // add the face one at a time
  NewtonTreeCollisionAddFace (collision, 4, &points[0][0], 3 * sizeof (dFloat), 0);

  // finish building the collision
  NewtonTreeCollisionEndBuild (collision, 1);

  // create a body with a collision and locate at the identity matrix position
  kmMat4 matrix;
  kmMat4Identity(&matrix);
  NewtonBody *body = NewtonCreateDynamicBody(world, collision, &matrix.mat[0]);

  // do no forget to destroy the collision after you not longer need it
  NewtonDestroyCollision(collision);
  return body;
}

static void apply_gravity (const NewtonBody* const body, dFloat timestep, int threadIndex)
{
  // apply gravity force to the body
  dFloat mass;
  dFloat Ixx;
  dFloat Iyy;
  dFloat Izz;

  NewtonBodyGetMass(body, &mass, &Ixx, &Iyy, &Izz);
  float gravityForce[4] = {0, -9.8f * mass, 0.0f, 0.0f};
  NewtonBodySetForce(body, &gravityForce[0]);
  //float torque[4] = {0, -9.8f * mass, 0.0f, 0.0f};
  //NewtonBodySetTorque(body, &torque[0]);
}

NewtonBody* create_ball()
{
  kmMat4 offset;
  kmMat4Translation(&offset, 8, 8, 0);
  // crate a collision sphere
  NewtonCollision* const collision = NewtonCreateBox(world, 16.0f, 16.0f, 16.0f, 0, &offset.mat[0]);

  // create a dynamic body with a sphere shape, and
  kmMat4 matrix;
  kmMat4Identity(&matrix);
  kmMat4 trans;
  kmMat4Translation(&trans, 160, 50, 0);
  kmMat4Multiply(&matrix, &trans, &matrix);
  //matrix.m_posit.m_y = 50.0f;
  NewtonBody* const body = NewtonCreateDynamicBody(world, collision, &matrix.mat[0]);

  // set the force callback for applying the force and torque
  NewtonBodySetForceAndTorqueCallback(body, apply_gravity);

  // set the mass for this body
  dFloat mass = 1.0f;
  NewtonBodySetMassProperties(body, mass, collision);

  // set the linear damping to zero
  NewtonBodySetLinearDamping (body, 0.1f);

  // do no forget to destroy the collision after you not longer need it
  NewtonDestroyCollision(collision);
  return body;
}

void setup_world() {
  world = NewtonCreate();
  //background_body = create_background();
  create_barriers();
  ball_body = create_ball();
  NewtonInvalidateCache(world);
}

void destroy_world() {
  NewtonDestroyAllBodies(world);
  NewtonDestroy(world);
}

void advance_simulation(float dt) {
  if (dt <= 0) return;

  NewtonUpdate(world, dt);
  kmMat4 matrix;
  NewtonBodyGetMatrix(ball_body, matrix.mat);
  kmVec3 pos;
  kmMat4ExtractTranslationVec3(&matrix, &pos);
  binocle_log_info("ball x:%f y:%f z:%f", pos.x, pos.y, pos.z);
}

void main_loop() {
  binocle_window_begin_frame(window);
  float dt = binocle_window_get_frame_time(window) / 1000.0f;

  binocle_input_update(&input);

  if (input.resized) {
    kmVec2 oldWindowSize;
    oldWindowSize.x = window->width;
    oldWindowSize.y = window->height;
    window->width = input.newWindowSize.x;
    window->height = input.newWindowSize.y;
    // Update the pixel-perfect rescaling viewport adapter
    binocle_viewport_adapter_reset(camera.viewport_adapter, oldWindowSize, input.newWindowSize);
    input.resized = false;
  }

  kmMat4 matrix;
  NewtonBodyGetMatrix(ball_body, matrix.mat);
  kmVec3 pos;
  kmMat4ExtractTranslationVec3(&matrix, &pos);

  kmAABB2 ball_bounds;
  ball_bounds.min.x = pos.x;
  ball_bounds.min.y = pos.y;
  ball_bounds.max.x = pos.x + 32;
  ball_bounds.max.y = pos.y + 32;
  if (kmAABB2ContainsPoint(&ball_bounds, &mouse_world_pos) && binocle_input_is_mouse_down(input, MOUSE_LEFT)) {
    dragging_ball = true;
  }

  if (dragging_ball && binocle_input_is_mouse_pressed(input, MOUSE_LEFT)) {
    // set position
    kmMat4 identity;
    kmMat4Identity(&identity);
    kmMat4 trans;
    kmMat4Translation(&trans, mouse_world_pos.x, mouse_world_pos.y, pos.z);
    kmMat4Multiply(&identity, &identity, &trans);
    NewtonBodySetMatrix(ball_body, &identity.mat[0]);

    // apply force
    dFloat mass;
    dFloat Ixx;
    dFloat Iyy;
    dFloat Izz;

    NewtonBodyGetMass(ball_body, &mass, &Ixx, &Iyy, &Izz);
    float gravityForce[4] = {10 * (mouse_world_pos.x - mouse_prev_pos.x), 0.0f, 0.0f, 0.0f};
    NewtonBodySetVelocity(ball_body, &gravityForce[0]);
  }

  if (binocle_input_is_mouse_up(input, MOUSE_LEFT)) {
    dragging_ball = false;
  }

  // Set the render target we will draw to
  binocle_gd_set_render_target(render_target);

  // Clear the render target with red color
  binocle_window_clear(window);

  advance_simulation(dt);

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
  uint64_t x = (uint64_t)((DESIGN_WIDTH - (sprite->material->albedo_texture->width * scale.x)) / 2.0f);
  uint64_t y = (uint64_t)((DESIGN_HEIGHT - (sprite->material->albedo_texture->height * scale.x)) / 2.0f);

  // Draw the logo to the render target
  binocle_sprite_draw(sprite, &gd, x, y, &viewport, 0, &scale, &camera);

  binocle_sprite_draw(ball_sprite, &gd, (uint64_t)pos.x, (uint64_t)pos.y, &viewport, 0, &scale, &camera);
  char mouse_str[256];
  sprintf(mouse_str, "x: %.0f y:%.0f %d", mouse_world_pos.x, mouse_world_pos.y, dragging_ball);
  binocle_bitmapfont_draw_string(font, mouse_str, 32, &gd, 0, DESIGN_HEIGHT - 70, viewport, binocle_color_white(), identity_matrix);

  // Gets the viewport calculated by the adapter
  kmAABB2 vp = binocle_viewport_adapter_get_viewport(*adapter);
  float vp_x = vp.min.x;
  float vp_y = vp.min.y;
  // Reset the render target to the screen
  binocle_gd_set_render_target(NULL);
  // Clear the screen with an azure
  binocle_gd_clear(binocle_color_azure());
  binocle_gd_apply_viewport(vp);
  binocle_gd_apply_shader(&gd, screen_shader);
  binocle_gd_set_uniform_float2(screen_shader, "resolution", DESIGN_WIDTH,
                                DESIGN_HEIGHT);
  binocle_gd_set_uniform_mat4(screen_shader, "transform", identity_matrix);
  binocle_gd_set_uniform_float2(screen_shader, "scale", adapter->inverse_multiplier, adapter->inverse_multiplier);
  binocle_gd_set_uniform_float2(screen_shader, "viewport", vp_x, vp_y);
  binocle_gd_draw_quad_to_screen(screen_shader, *render_target);

  binocle_window_refresh(window);
  binocle_window_end_frame(window);

  mouse_prev_pos = mouse_world_pos;
}

int main(int argc, char *argv[])
{
  app = binocle_app_new();
  binocle_app_init(&app);

  binocle_data_dir = binocle_sdl_assets_dir();

  window = binocle_window_new(DESIGN_WIDTH, DESIGN_HEIGHT, "Binocle Render Target Example");
  binocle_window_set_background_color(window, binocle_color_red());
  binocle_window_set_minimum_size(window, DESIGN_WIDTH, DESIGN_HEIGHT);
  adapter = binocle_viewport_adapter_new(window, BINOCLE_VIEWPORT_ADAPTER_KIND_SCALING, BINOCLE_VIEWPORT_ADAPTER_SCALING_TYPE_PIXEL_PERFECT, window->original_width, window->original_height, window->original_width, window->original_height);
  camera = binocle_camera_new(adapter);
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
  material->albedo_texture = texture;
  material->shader = shader;
  sprite = binocle_sprite_from_material(material);

  char font_filename[1024];
  sprintf(font_filename, "%s%s", binocle_data_dir, "font.fnt");
  font = binocle_bitmapfont_from_file(font_filename, true);

  char font_image_filename[1024];
  sprintf(font_image_filename, "%s%s", binocle_data_dir, "font.png");
  font_image = binocle_image_load(font_image_filename);
  font_texture = binocle_texture_from_image(font_image);
  font_material = binocle_material_new();
  font_material->albedo_texture = font_texture;
  font_material->shader = shader;
  font->material = font_material;
  font_sprite = binocle_sprite_from_material(font_material);
  font_sprite_pos.x = 0;
  font_sprite_pos.y = -256;

  sprintf(filename, "%s%s", binocle_data_dir, "player.png");
  binocle_image *ball_image = binocle_image_load(filename);
  binocle_texture *ball_texture = binocle_texture_from_image(ball_image);
  binocle_material *ball_material = binocle_material_new();
  ball_material->albedo_texture = ball_texture;
  ball_material->shader = shader;
  ball_sprite = binocle_sprite_from_material(ball_material);

  setup_world();

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
  binocle_sprite_destroy(sprite);
  binocle_material_destroy(material);
  binocle_texture_destroy(texture);
  binocle_image_destroy(image);
  binocle_shader_destroy(shader);
  binocle_shader_destroy(screen_shader);
  binocle_viewport_adapter_destroy(adapter);
  free(binocle_data_dir);
  destroy_world();
  binocle_app_destroy(&app);
}


