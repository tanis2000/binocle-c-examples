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
binocle_camera_3d camera;
binocle_gd gd;
binocle_shader *shader;
binocle_shader *lamp_shader;
char *binocle_data_dir = NULL;
binocle_app app;
binocle_image *diffuse_image;
binocle_image *specular_image;
binocle_texture *diffuse_texture;
binocle_texture *specular_texture;
binocle_sprite sprite;
binocle_model model;
static kmVec3 pointLightPositions[] = {
  /*
  {.x = 0.7f, .y = 0.2f,  .z = 2.0f},
  {.x = 2.3f, .y = -3.3f, .z = -4.0f},
  {.x = -4.0f, .y = 2.0f, .z = -12.0f},
  {.x = 0.0f, .y= 0.0f, .z = -3.0f}
   */
  {.x = -5.0f, .y = 0.0f, .z = 0.0f}, // left
  {.x = 5.0f, .y = 0.0f, .z = 0.0f}, // right
  {.x = 0.0f, .y = 0.0f, .z = 5.0f}, // front
  {.x = 0.0f, .y = 0.0f, .z = -5.0f} // back
};

void setup_lights() {
  binocle_gd_apply_shader(&gd, *shader);
  GLint tex_id;
  glCheck(tex_id = glGetUniformLocation(shader->program_id, "material.diffuse"));
  glCheck(glUniform1i(tex_id, 0));
  glCheck(tex_id = glGetUniformLocation(shader->program_id, "material.specular"));
  glCheck(glUniform1i(tex_id, 1));
  binocle_gd_set_uniform_vec3(*shader, "viewPos", camera.position);
  binocle_gd_set_uniform_float(*shader, "material.shininess", 32.0f);
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
    binocle_gd_set_uniform_vec3(*shader, "dirLight.direction", direction);
    binocle_gd_set_uniform_vec3(*shader, "dirLight.ambient", ambient);
    binocle_gd_set_uniform_vec3(*shader, "dirLight.diffuse", diffuse);
    binocle_gd_set_uniform_vec3(*shader, "dirLight.specular", specular);
  }
  {
    // point light 1
    kmVec3 ambient = {.x = 0.05f, .y = 0.05f, .z = 0.05f};
    kmVec3 diffuse = {.x = 0.8f, .y = 0.8f, .z = 0.8f};
    kmVec3 specular = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
    binocle_gd_set_uniform_vec3(*shader, "pointLights[0].position", pointLightPositions[0]);
    binocle_gd_set_uniform_vec3(*shader, "pointLights[0].ambient", ambient);
    binocle_gd_set_uniform_vec3(*shader, "pointLights[0].diffuse", diffuse);
    binocle_gd_set_uniform_vec3(*shader, "pointLights[0].specular", specular);
    binocle_gd_set_uniform_float(*shader, "pointLights[0].constant", 1.0f);
    binocle_gd_set_uniform_float(*shader, "pointLights[0].linear", 0.09);
    binocle_gd_set_uniform_float(*shader, "pointLights[0].quadratic", 0.032);
  }
  {
    // point light 2
    kmVec3 ambient = {.x = 0.05f, .y = 0.05f, .z = 0.05f};
    kmVec3 diffuse = {.x = 0.8f, .y = 0.8f, .z = 0.8f};
    kmVec3 specular = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
    binocle_gd_set_uniform_vec3(*shader, "pointLights[1].position", pointLightPositions[1]);
    binocle_gd_set_uniform_vec3(*shader, "pointLights[1].ambient", ambient);
    binocle_gd_set_uniform_vec3(*shader, "pointLights[1].diffuse", diffuse);
    binocle_gd_set_uniform_vec3(*shader, "pointLights[1].specular", specular);
    binocle_gd_set_uniform_float(*shader, "pointLights[1].constant", 1.0f);
    binocle_gd_set_uniform_float(*shader, "pointLights[1].linear", 0.09);
    binocle_gd_set_uniform_float(*shader, "pointLights[1].quadratic", 0.032);
  }
  {
    // point light 3
    kmVec3 ambient = {.x = 0.05f, .y = 0.05f, .z = 0.05f};
    kmVec3 diffuse = {.x = 0.8f, .y = 0.8f, .z = 0.8f};
    kmVec3 specular = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
    binocle_gd_set_uniform_vec3(*shader, "pointLights[2].position", pointLightPositions[2]);
    binocle_gd_set_uniform_vec3(*shader, "pointLights[2].ambient", ambient);
    binocle_gd_set_uniform_vec3(*shader, "pointLights[2].diffuse", diffuse);
    binocle_gd_set_uniform_vec3(*shader, "pointLights[2].specular", specular);
    binocle_gd_set_uniform_float(*shader, "pointLights[2].constant", 1.0f);
    binocle_gd_set_uniform_float(*shader, "pointLights[2].linear", 0.09f);
    binocle_gd_set_uniform_float(*shader, "pointLights[2].quadratic", 0.032);
  }
  {
    // point light 4
    kmVec3 ambient = {.x = 0.05f, .y = 0.05f, .z = 0.05f};
    kmVec3 diffuse = {.x = 0.8f, .y = 0.8f, .z = 0.8f};
    kmVec3 specular = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
    binocle_gd_set_uniform_vec3(*shader, "pointLights[3].position", pointLightPositions[3]);
    binocle_gd_set_uniform_vec3(*shader, "pointLights[3].ambient", ambient);
    binocle_gd_set_uniform_vec3(*shader, "pointLights[3].diffuse", diffuse);
    binocle_gd_set_uniform_vec3(*shader, "pointLights[3].specular", specular);
    binocle_gd_set_uniform_float(*shader, "pointLights[3].constant", 1.0f);
    binocle_gd_set_uniform_float(*shader, "pointLights[3].linear", 0.09);
    binocle_gd_set_uniform_float(*shader, "pointLights[3].quadratic", 0.032);
  }
  // spotLight
  {
    binocle_gd_set_uniform_vec3(*shader, "spotLight.position", camera.position);
    binocle_gd_set_uniform_vec3(*shader, "spotLight.direction", camera.front);
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
    binocle_gd_set_uniform_vec3(*shader, "spotLight.ambient", ambient);
    binocle_gd_set_uniform_vec3(*shader, "spotLight.diffuse", diffuse);
    binocle_gd_set_uniform_vec3(*shader, "spotLight.specular", specular);
    binocle_gd_set_uniform_float(*shader, "spotLight.constant", 1.0f);
    binocle_gd_set_uniform_float(*shader, "spotLight.linear", 0.09f);
    binocle_gd_set_uniform_float(*shader, "spotLight.quadratic", 0.032f);
    binocle_gd_set_uniform_float(*shader, "spotLight.cutOff", cosf(kmDegreesToRadians(12.5f)));
    binocle_gd_set_uniform_float(*shader, "spotLight.outerCutOff", cosf(kmDegreesToRadians(15.0f)));
  }
}

void draw_light(kmVec3 position, kmAABB2 viewport) {
  static GLfloat g_quad_vertex_buffer_data[] = {
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, 0.5f,
    -0.5f, -0.5f, 0.5f,
    -0.5f, -0.5f, -0.5f,
    0.5f, 0.5f, -0.5f,
    0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, -0.5f
  };

  static const GLuint index_buffer_data[] = {
    0, 1, 2,
    0, 2, 3,
    4, 7, 6,
    4, 6, 5,
    0, 4, 5,
    0, 5, 1,
    1, 5, 6,
    1, 6, 2,
    2, 6, 7,
    2, 7, 3,
    4, 0, 3,
    4, 3, 7
  };

  kmMat4 projectionMatrix;
  kmMat4Identity(&projectionMatrix);
  kmMat4PerspectiveProjection(&projectionMatrix, camera.fov_y, viewport.max.x / viewport.max.y, camera.near /*camera.near + camera.position.z*/, camera.far /*camera.position.z + camera.far*/);

  kmMat4 viewMatrix;
  kmMat4Identity(&viewMatrix);
  kmMat4Multiply(&viewMatrix, &viewMatrix, binocle_camera_3d_get_transform_matrix(&camera));

  kmMat4 modelMatrix;
  kmMat4Identity(&modelMatrix);

  kmMat4 scale;
  kmMat4Scaling(&scale, 0.2f, 0.2f, 0.2f);
  kmMat4Multiply(&modelMatrix, &modelMatrix, &scale);

  kmMat4 trans;
  kmMat4Translation(&trans, position.x, position.y, position.z);
  kmMat4Multiply(&modelMatrix, &modelMatrix, &trans);


/*
  kmMat4 rot_x;
  kmMat4 rot_y;
  kmMat4 rot_z;
  kmMat4RotationX(&rot_x, 1);
  kmMat4RotationY(&rot_y, 1);
  kmMat4RotationZ(&rot_z, 1);
  kmMat4Multiply(&modelMatrix, &modelMatrix, &rot_x);
  kmMat4Multiply(&modelMatrix, &modelMatrix, &rot_y);
  kmMat4Multiply(&modelMatrix, &modelMatrix, &rot_z);
*/



  GLuint quad_vertexbuffer;
  glCheck(glGenBuffers(1, &quad_vertexbuffer));
  glCheck(glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer));
  glCheck(glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW));

  GLint pos_id;
  glCheck(pos_id = glGetAttribLocation(lamp_shader->program_id, "vertexPosition"));
  glCheck(glVertexAttribPointer(pos_id, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0));
  glCheck(glEnableVertexAttribArray(pos_id));

  binocle_gd_set_uniform_mat4(*lamp_shader, "projectionMatrix", projectionMatrix);
  binocle_gd_set_uniform_mat4(*lamp_shader, "viewMatrix", viewMatrix);
  binocle_gd_set_uniform_mat4(*lamp_shader, "modelMatrix", modelMatrix);

  GLuint quad_indexbuffer;
  glCheck(glGenBuffers(1, &quad_indexbuffer));
  glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_indexbuffer));
  glCheck(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW));
  glCheck(glDrawElements(GL_TRIANGLES, 12 * 3, GL_UNSIGNED_INT, 0));

  glCheck(glDisableVertexAttribArray(pos_id));
  glCheck(glBindBuffer(GL_ARRAY_BUFFER, 0));
  glCheck(glDeleteBuffers(1, &quad_vertexbuffer));
  glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  glCheck(glDeleteBuffers(1, &quad_indexbuffer));
}

void main_loop() {
  binocle_window_begin_frame(&window);
  float dt = binocle_window_get_frame_time(&window) / 1000.0f;

  binocle_input_update(&input);

  if (input.resized) {
    kmVec2 oldWindowSize = {.x = window.width, .y = window.height};
    window.width = input.newWindowSize.x;
    window.height = input.newWindowSize.y;
    input.resized = false;
  }

  if (binocle_input_is_key_pressed(&input, KEY_D)) {
    binocle_camera_3d_translate(&camera, 30.0f * dt, 0.0f, 0.0f);
  } else if (binocle_input_is_key_pressed(&input, KEY_A)) {
    binocle_camera_3d_translate(&camera, -30.0f * dt, 0.0f, 0.0f);
  }

  if (binocle_input_is_key_pressed(&input, KEY_W)) {
    binocle_camera_3d_translate(&camera, 0.0f, 0.0f, -30.0f * dt);
  } else if (binocle_input_is_key_pressed(&input, KEY_S)) {
    binocle_camera_3d_translate(&camera, 0.0f, 0.0f, 30.0f * dt);
  }

  if (binocle_input_is_key_pressed(&input, KEY_R)) {
    binocle_camera_3d_translate(&camera, 0.0f, 30.0f * dt, 0.0f);
  } else if (binocle_input_is_key_pressed(&input, KEY_F)) {
    binocle_camera_3d_translate(&camera, 0.0f, -30.0f * dt, 0.0f);
  }

  if (binocle_input_is_key_pressed(&input, KEY_E)) {
    binocle_camera_3d_rotate(&camera, 0.0f, 30.0 * dt, 0.0f);
  } else if (binocle_input_is_key_pressed(&input, KEY_Q)) {
    binocle_camera_3d_rotate(&camera, 0.0f, -30.0 * dt, 0.0f);
  }

  if (binocle_input_is_key_pressed(&input, KEY_T)) {
    binocle_camera_3d_rotate(&camera, 30.0 * dt, 0.0f, 0.0f);
  } else if (binocle_input_is_key_pressed(&input, KEY_G)) {
    binocle_camera_3d_rotate(&camera, -30.0 * dt, 0.0f, 0.0f);
  }

  binocle_gd_clear(binocle_color_black());

  kmAABB2 viewport;
  viewport.min.x = 0;
  viewport.min.y = 0;
  viewport.max.x = window.width;
  viewport.max.y = window.height;

  binocle_gd_apply_viewport(viewport);
  binocle_gd_apply_shader(&gd, *shader);
  //binocle_gd_draw_test_triangle(shader);
  //binocle_gd_draw_test_cube(shader);
  kmMat4 rot_x;
  kmMat4 rot_y;
  kmMat4 rot_z;
  kmMat4RotationX(&rot_x, 0.3f * dt);
  kmMat4RotationY(&rot_y, 0.3f * dt);
  kmMat4RotationZ(&rot_z, 0.3f * dt);
  //kmMat4Multiply(&model.meshes[0].transform, &model.meshes[0].transform, &rot_x);
  //kmMat4Multiply(&model.meshes[0].transform, &model.meshes[0].transform, &rot_y);
  //kmMat4Multiply(&model.meshes[0].transform, &model.meshes[0].transform, &rot_z);
  kmMat4 trans;
  kmMat4Translation(&trans, 0.1f * dt, 0.0f, 0.0f);
  //kmMat4Multiply(&model.meshes[0].transform, &model.meshes[0].transform, &trans);
  setup_lights();
  kmMat4Identity(&model.meshes[0].transform);
  binocle_gd_draw_mesh(&gd, &model.meshes[0], viewport, &camera);

  binocle_gd_apply_shader(&gd, *lamp_shader);
  for (int i = 0 ; i < 4 ; i++) {
    draw_light(pointLightPositions[i], viewport);
  }

  binocle_window_refresh(&window);
  binocle_window_end_frame(&window);
}

int main(int argc, char *argv[])
{
  app = binocle_app_new();
  binocle_app_init(&app);

  binocle_data_dir = binocle_sdl_assets_dir();

  window = binocle_window_new(DESIGN_WIDTH, DESIGN_HEIGHT, "Binocle 3D Model Example");
  binocle_window_set_background_color(&window, binocle_color_white());
  kmVec3 camera_pos;
  camera_pos.x = 0;
  camera_pos.y = 0;
  camera_pos.z = 3.0f;
  camera = binocle_camera_3d_new(camera_pos, 0.3f, 1000.0f, 60);
  //binocle_camera_3d_set_rotation(&camera, 0, -90, 0);
  input = binocle_input_new();
  binocle_shader_init_defaults();

  char vert[1024];
  sprintf(vert, "%s%s", binocle_data_dir, "default.vert");
  char frag[1024];
  sprintf(frag, "%s%s", binocle_data_dir, "default.frag");
  shader = binocle_shader_load_from_file(vert, frag);

  sprintf(vert, "%s%s", binocle_data_dir, "lamp.vert");
  sprintf(frag, "%s%s", binocle_data_dir, "lamp.frag");
  lamp_shader = binocle_shader_load_from_file(vert, frag);

  char filename[1024];
  sprintf(filename, "%s%s", binocle_data_dir, "cube2.model");
  char mtl_filename[1024];
  sprintf(mtl_filename, "%s%s", binocle_data_dir, "cube2.mtl");
  model = binocle_model_load_obj(filename, mtl_filename);
  /*
  kmMat4 scale;
  kmMat4Scaling(&scale, 0.000005f, 0.000005f, 0.000005f);
  kmMat4Multiply(&model.meshes[0].transform, &model.meshes[0].transform, &scale);
   */


  sprintf(filename, "%s%s", binocle_data_dir, "container2.png");
  diffuse_image = binocle_image_load(filename);
  diffuse_texture = binocle_texture_from_image(diffuse_image);
  model.meshes[0].material->texture = diffuse_texture;
  model.meshes[0].material->shader = shader;

  sprintf(filename, "%s%s", binocle_data_dir, "container2_specular.png");
  specular_image = binocle_image_load(filename);
  specular_texture = binocle_texture_from_image(specular_image);
  model.meshes[0].material->specular_texture = specular_texture;

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
  binocle_texture_destroy(diffuse_texture);
  binocle_texture_destroy(specular_texture);
  binocle_image_destroy(diffuse_image);
  binocle_image_destroy(specular_image);
  binocle_shader_destroy(lamp_shader);
  binocle_shader_destroy(shader);
  free(binocle_data_dir);
  binocle_app_destroy(&app);
}


