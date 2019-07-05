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
#include <binocle_model.h>

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
binocle_camera_3d camera;
binocle_gd gd;
binocle_shader shader;
char *binocle_data_dir = NULL;
binocle_app app;
binocle_image image;
binocle_texture texture;
binocle_sprite sprite;
binocle_model model;

void setup_lights() {
  binocle_gd_apply_shader(&gd, shader);
  GLint tex_id;
  glCheck(tex_id = glGetUniformLocation(shader.program_id, "material.diffuse"));
  glCheck(glUniform1i(tex_id, 0));
  glCheck(tex_id = glGetUniformLocation(shader.program_id, "material.specular"));
  glCheck(glUniform1i(tex_id, 1));
  binocle_gd_set_uniform_vec3(shader, "viewPos", camera.position);
  binocle_gd_set_uniform_float(shader, "material.shininess", 32.0f);
  {
    // directional light
    kmVec3 direction;
    direction.x = -0.2f;
    direction.y = -1.0f;
    direction.z = -0.3f;
    kmVec3 ambient;
    ambient.x = 0.1f;
    ambient.y = 0.1f;
    ambient.z = 0.1f;
    kmVec3 diffuse;
    diffuse.x = 0.4f;
    diffuse.y = 0.4f;
    diffuse.z = 0.4f;
    kmVec3 specular;
    specular.x = 0.5f;
    specular.y = 0.5f;
    specular.z = 0.5f;
    binocle_gd_set_uniform_vec3(shader, "dirLight.direction", direction);
    binocle_gd_set_uniform_vec3(shader, "dirLight.ambient", ambient);
    binocle_gd_set_uniform_vec3(shader, "dirLight.diffuse", diffuse);
    binocle_gd_set_uniform_vec3(shader, "dirLight.specular", specular);
  }
  /*
  // point light 1
  binocle_gd_set_uniform_vec3(shader, "pointLights[0].position", pointLightPositions[0]);
  binocle_gd_set_uniform_vec3(shader, "pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
  binocle_gd_set_uniform_vec3(shader, "pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
  binocle_gd_set_uniform_vec3(shader, "pointLights[0].specular", 1.0f, 1.0f, 1.0f);
  binocle_gd_set_uniform_float(shader, "pointLights[0].constant", 1.0f);
  binocle_gd_set_uniform_float(shader, "pointLights[0].linear", 0.09);
  binocle_gd_set_uniform_float(shader, "pointLights[0].quadratic", 0.032);
  // point light 2
  binocle_gd_set_uniform_vec3(shader, "pointLights[1].position", pointLightPositions[1]);
  binocle_gd_set_uniform_vec3(shader, "pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
  binocle_gd_set_uniform_vec3(shader, "pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
  binocle_gd_set_uniform_vec3(shader, "pointLights[1].specular", 1.0f, 1.0f, 1.0f);
  binocle_gd_set_uniform_float(shader, "pointLights[1].constant", 1.0f);
  binocle_gd_set_uniform_float(shader, "pointLights[1].linear", 0.09);
  binocle_gd_set_uniform_float(shader, "pointLights[1].quadratic", 0.032);
  // point light 3
  binocle_gd_set_uniform_vec3(shader, "pointLights[2].position", pointLightPositions[2]);
  binocle_gd_set_uniform_vec3(shader, "pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
  binocle_gd_set_uniform_vec3(shader, "pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
  binocle_gd_set_uniform_vec3(shader, "pointLights[2].specular", 1.0f, 1.0f, 1.0f);
  binocle_gd_set_uniform_float(shader, "pointLights[2].constant", 1.0f);
  binocle_gd_set_uniform_float(shader, "pointLights[2].linear", 0.09);
  binocle_gd_set_uniform_float(shader, "pointLights[2].quadratic", 0.032);
  // point light 4
  binocle_gd_set_uniform_vec3(shader, "pointLights[3].position", pointLightPositions[3]);
  binocle_gd_set_uniform_vec3(shader, "pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
  binocle_gd_set_uniform_vec3(shader, "pointLights[3].diffuse", 0.8f, 0.8f, 0.8f);
  binocle_gd_set_uniform_vec3(shader, "pointLights[3].specular", 1.0f, 1.0f, 1.0f);
  binocle_gd_set_uniform_float(shader, "pointLights[3].constant", 1.0f);
  binocle_gd_set_uniform_float(shader, "pointLights[3].linear", 0.09);
  binocle_gd_set_uniform_float(shader, "pointLights[3].quadratic", 0.032);
   */
  // spotLight
  {
    kmVec3 forward;
    kmMat4GetForwardVec3RH(&forward, binocle_camera_3d_get_transform_matrix(&camera));
    binocle_gd_set_uniform_vec3(shader, "spotLight.position", camera.position);
    binocle_gd_set_uniform_vec3(shader, "spotLight.direction", forward);
    kmVec3 ambient;
    ambient.x = 0.0f;
    ambient.y = 0.0f;
    ambient.z = 0.0f;
    kmVec3 diffuse;
    diffuse.x = 1.0f;
    diffuse.y = 1.0f;
    diffuse.z = 1.0f;
    kmVec3 specular;
    specular.x = 1.0f;
    specular.y = 1.0f;
    specular.z = 1.0f;
    binocle_gd_set_uniform_vec3(shader, "spotLight.ambient", ambient);
    binocle_gd_set_uniform_vec3(shader, "spotLight.diffuse", diffuse);
    binocle_gd_set_uniform_vec3(shader, "spotLight.specular", specular);
    binocle_gd_set_uniform_float(shader, "spotLight.constant", 10.0f);
    binocle_gd_set_uniform_float(shader, "spotLight.linear", 0.09f);
    binocle_gd_set_uniform_float(shader, "spotLight.quadratic", 0.032f);
    binocle_gd_set_uniform_float(shader, "spotLight.cutOff", cosf(kmDegreesToRadians(12.5f)));
    binocle_gd_set_uniform_float(shader, "spotLight.outerCutOff", cosf(kmDegreesToRadians(15.0f)));
  }
  /*
  binocle_gd_set_uniform_float(dof_shader, "time", running_time);
  binocle_gd_set_uniform_float2(dof_shader, "resolution", window.width, window.height);
  binocle_gd_set_uniform_float2(dof_shader, "uv", 1.0, 1.0);
  binocle_gd_set_uniform_render_target_as_texture(dof_shader, "tInput", g_buffer);
  binocle_gd_set_uniform_render_target_as_texture(dof_shader, "tBias", g_buffer);
  binocle_gd_set_uniform_float(dof_shader, "radius", 0.5);
  binocle_gd_set_uniform_float(dof_shader, "amount", 1.0);
  binocle_gd_set_uniform_float(dof_shader, "focalDistance", 2.0);
  binocle_gd_set_uniform_float(dof_shader, "aperture", 2.0);
  binocle_gd_draw_quad(dof_shader);
   */
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

  if (binocle_input_is_key_pressed(&input, KEY_D)) {
    binocle_camera_3d_translate(&camera, 30.0f * dt, 0.0f, 0.0f);
  } else if (binocle_input_is_key_pressed(&input, KEY_A)) {
    binocle_camera_3d_translate(&camera, -30.0f * dt, 0.0f, 0.0f);
  }

  if (binocle_input_is_key_pressed(&input, KEY_W)) {
    binocle_camera_3d_translate(&camera, 0.0f, 0.0f, 30.0f * dt);
  } else if (binocle_input_is_key_pressed(&input, KEY_S)) {
    binocle_camera_3d_translate(&camera, 0.0f, 0.0f, -30.0f * dt);
  }

  if (binocle_input_is_key_pressed(&input, KEY_R)) {
    binocle_camera_3d_translate(&camera, 0.0f, 30.0f * dt, 0.0f);
  } else if (binocle_input_is_key_pressed(&input, KEY_F)) {
    binocle_camera_3d_translate(&camera, 0.0f, -30.0f * dt, 0.0f);
  }

  if (binocle_input_is_key_pressed(&input, KEY_E)) {
    binocle_camera_3d_rotate(&camera, 0.0f, 1.0 * dt, 0.0f);
  } else if (binocle_input_is_key_pressed(&input, KEY_Q)) {
    binocle_camera_3d_rotate(&camera, 0.0f, -1.0 * dt, 0.0f);
  }

  binocle_window_set_background_color(&window, binocle_color_azure());
  binocle_gd_clear(binocle_color_black());

  kmVec2 scale;
  scale.x = 0.3f;
  scale.y = 0.3f;
  kmAABB2 viewport;
  viewport.min.x = 0;
  viewport.min.y = 0;
  viewport.max.x = window.width;
  viewport.max.y = window.height;
  binocle_gd_apply_viewport(viewport);
  binocle_gd_apply_shader(&gd, shader);
  //binocle_gd_draw_test_triangle(shader);
  //binocle_gd_draw_test_cube(shader);
  kmMat4 rot_x;
  kmMat4 rot_y;
  kmMat4 rot_z;
  kmMat4RotationX(&rot_x, 0.3f * dt);
  kmMat4RotationY(&rot_y, 0.3f * dt);
  kmMat4RotationZ(&rot_z, 0.3f * dt);
  kmMat4Multiply(&model.meshes[0].transform, &model.meshes[0].transform, &rot_x);
  kmMat4Multiply(&model.meshes[0].transform, &model.meshes[0].transform, &rot_y);
  kmMat4Multiply(&model.meshes[0].transform, &model.meshes[0].transform, &rot_z);
  setup_lights();
  binocle_gd_draw_mesh(&gd, &model.meshes[0], viewport, &camera);

  binocle_window_refresh(&window);
  binocle_window_end_frame(&window);
}

int main(int argc, char *argv[])
{
  app = binocle_app_new();
  binocle_app_init(&app);

  binocle_data_dir = binocle_sdl_assets_dir();

  window = binocle_window_new(DESIGN_WIDTH, DESIGN_HEIGHT, "Binocle Basic Example");
  binocle_window_set_background_color(&window, binocle_color_white());
  adapter = binocle_viewport_adapter_new(window, BINOCLE_VIEWPORT_ADAPTER_KIND_SCALING, BINOCLE_VIEWPORT_ADAPTER_SCALING_TYPE_PIXEL_PERFECT, window.original_width, window.original_height, window.original_width, window.original_height);
  kmVec3 camera_pos;
  camera_pos.x = 0;
  camera_pos.y = 0;
  camera_pos.z = -500.0f;
  kmVec3 camera_rot;
  camera_rot.x = 0;
  camera_rot.y = 0;
  camera_rot.z = 0;
  camera = binocle_camera_3d_new(camera_pos, camera_rot, 100.0f, 50000.0f, kmDegreesToRadians(45));
  input = binocle_input_new();
  binocle_shader_init_defaults();

  char vert[1024];
  sprintf(vert, "%s%s", binocle_data_dir, "default.vert");
  char frag[1024];
  sprintf(frag, "%s%s", binocle_data_dir, "default.frag");
  shader = binocle_shader_load_from_file(vert, frag);

  char filename[1024];
  sprintf(filename, "%s%s", binocle_data_dir, "chr_knight.model");
  model = binocle_model_load_obj(filename);

  sprintf(filename, "%s%s", binocle_data_dir, "chr_knight.png");
  image = binocle_image_load(filename);
  texture = binocle_texture_from_image(image);
  model.meshes[0].material->texture = &texture;
  model.meshes[0].material->shader = &shader;

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


