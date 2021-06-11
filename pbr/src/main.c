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
#include "binocle_camera.h"
#include <binocle_input.h>
#include <binocle_image.h>
#include <binocle_sprite.h>
#include <backend/binocle_material.h>
#include <binocle_model.h>

#define BINOCLE_MATH_IMPL
#include "binocle_math.h"
#include "binocle_gd.h"
#include "binocle_log.h"
#include "binocle_bitmapfont.h"
#include "binocle_ecs.h"
#include "binocle_app.h"
#include "binocle_collision.h"

#define DESIGN_WIDTH 800
#define DESIGN_HEIGHT 600
#define MAX_VERTICES (192000)

typedef struct default_shader_vs_uniforms_t {
  float projectionMatrix[16];
  float viewMatrix[16];
  float modelMatrix[16];
} default_shader_vs_uniforms_t;

typedef struct shader_material_t {
  float albedoMap;
  float normalMap;
  float metallicMap;
  float roughnessMap;
  float aoMap;
} shader_material_t;

typedef struct shader_dir_light_t {
  float direction[3];

  float ambient[3];
  float diffuse[3];
  float specular[3];
} shader_dir_light_t;

typedef struct shader_point_light_t {
  float position[3];

  float constant;
  float linear;
  float quadratic;

  float ambient[3];
  float diffuse[3];
  float specular[3];
} shader_point_light_t;

typedef struct shader_spot_light_t {
  float position[3];
  float direction[3];
  float cutOff;
  float outerCutOff;

  float constant;
  float linear;
  float quadratic;

  float ambient[3];
  float diffuse[3];
  float specular[3];
} shader_spot_light_t;

typedef struct default_shader_fs_uniforms_t {
  float viewPos[3];
  shader_dir_light_t dirLight;
  shader_point_light_t pointLight[4];
  shader_spot_light_t spotLight;
//  shader_material_t material;
} default_shader_fs_uniforms_t;

typedef struct lamp_shader_uniforms_t {
  float projectionMatrix[16];
  float viewMatrix[16];
  float modelMatrix[16];
} lamp_shader_uniforms_t;

typedef struct state_t {
  struct {
    binocle_pass_action action;
    binocle_pipeline pip;
    binocle_bindings bind;
    default_shader_vs_uniforms_t vs_uni;
    default_shader_fs_uniforms_t fs_uni;
    binocle_buffer vbuf;
  } main;
} state_t;

binocle_window *window;
binocle_input input;
binocle_camera_3d camera;
binocle_gd gd;
binocle_shader shader;
binocle_shader lamp_shader;
char *binocle_data_dir = NULL;
binocle_app app;
binocle_image albedo_image;
binocle_image normal_image;
binocle_image metallic_image;
binocle_image roughness_image;
binocle_image ao_image;
binocle_sprite sprite;
binocle_model model;
float elapsed_time = 0;
state_t state;

static kmVec3 pointLightPositions[] = {
  /*
  {.x = 0.7f, .y = 0.2f,  .z = 2.0f},
  {.x = 2.3f, .y = -3.3f, .z = -4.0f},
  {.x = -4.0f, .y = 2.0f, .z = -12.0f},
  {.x = 0.0f, .y= 0.0f, .z = -3.0f}
   */
  /*
  {.x = -7.0f, .y = 0.0f, .z = 0.0f}, // left
  {.x = 7.0f, .y = 0.0f, .z = 0.0f}, // right
  {.x = 0.0f, .y = 0.0f, .z = 7.0f}, // front
  {.x = 0.0f, .y = 0.0f, .z = -7.0f} // back
  */
  /*
  {.x = -1.0f, .y = 0.0f, .z = 0.0f}, // left
  {.x = 1.0f, .y = 0.0f, .z = 0.0f}, // right
  {.x = 0.0f, .y = 0.0f, .z = 1.0f}, // front
  {.x = 0.0f, .y = 0.0f, .z = -1.0f} // back
   */
  {.x = -2.0f, .y = 0.0f, .z = 0.0f}, // left
  {.x = 2.0f, .y = 0.0f, .z = 0.0f}, // right
  {.x = 0.0f, .y = 0.0f, .z = 2.0f}, // front
  {.x = 0.0f, .y = 0.0f, .z = -2.0f} // back
};

kmVec3 mouse_position;
kmVec3 ray_end_position;

void setup_default_pipeline() {
  // Clear screen action for the main scene
  binocle_color off_clear_color = binocle_color_azure();
  binocle_pass_action action = {
    .colors[0] = {
      .action = BINOCLE_ACTION_CLEAR,
      .value = {
        .r = off_clear_color.r,
        .g = off_clear_color.g,
        .b = off_clear_color.b,
        .a = off_clear_color.a,
      }
    }
  };
  state.main.action = action;

  // Pipeline state object for the screen (default) pass
  state.main.pip = binocle_backend_make_pipeline(&(binocle_pipeline_desc){
    .layout = {
      .attrs = {
        [0] = { .format = BINOCLE_VERTEXFORMAT_FLOAT3 }, // position
        [1] = { .format = BINOCLE_VERTEXFORMAT_FLOAT4 }, // color
        [2] = { .format = BINOCLE_VERTEXFORMAT_FLOAT2 }, // texture uv
        [3] = { .format = BINOCLE_VERTEXFORMAT_FLOAT3 }, // normal
      }
    },
    .shader = shader,
    .index_type = BINOCLE_INDEXTYPE_NONE,
    .colors = {
      [0] = { .pixel_format = BINOCLE_PIXELFORMAT_RGBA8 },
    }
  });

  binocle_buffer_desc vbuf_desc = {
    .type = BINOCLE_BUFFERTYPE_VERTEXBUFFER,
    .usage = BINOCLE_USAGE_STREAM,
    .size = sizeof(binocle_vpctn) * MAX_VERTICES,
  };
  state.main.vbuf = binocle_backend_make_buffer(&vbuf_desc);

  state.main.bind = (binocle_bindings){
    .vertex_buffers = {
      [0] = state.main.vbuf,
    },
  };


}

void setup_lights() {
  //binocle_gd_set_uniform_float(*shader, "material.shininess", 32.0f);
  /*
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
   */
  state.main.fs_uni.viewPos[0] = camera.position.x;
  state.main.fs_uni.viewPos[1] = camera.position.y;
  state.main.fs_uni.viewPos[2] = camera.position.z;
  {
    // point light 1
    kmVec3 ambient = {.x = 0.05f, .y = 0.05f, .z = 0.05f};
    kmVec3 diffuse = {.x = 0.8f, .y = 0.8f, .z = 0.8f};
    kmVec3 specular = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
    state.main.fs_uni.pointLight[0].position[0] = pointLightPositions[0].x;
    state.main.fs_uni.pointLight[0].position[1] = pointLightPositions[0].y;
    state.main.fs_uni.pointLight[0].position[2] = pointLightPositions[0].z;
    state.main.fs_uni.pointLight[0].ambient[0] = ambient.x;
    state.main.fs_uni.pointLight[0].ambient[1] = ambient.y;
    state.main.fs_uni.pointLight[0].ambient[2] = ambient.z;
    state.main.fs_uni.pointLight[0].diffuse[0] = diffuse.x;
    state.main.fs_uni.pointLight[0].diffuse[1] = diffuse.y;
    state.main.fs_uni.pointLight[0].diffuse[2] = diffuse.z;
    state.main.fs_uni.pointLight[0].specular[0] = specular.x;
    state.main.fs_uni.pointLight[0].specular[1] = specular.y;
    state.main.fs_uni.pointLight[0].specular[2] = specular.z;
    state.main.fs_uni.pointLight[0].constant = 1.0f;
    state.main.fs_uni.pointLight[0].linear = 0.09f;
    state.main.fs_uni.pointLight[0].quadratic = 0.032f;
  }
  {
    // point light 2
    kmVec3 ambient = {.x = 0.05f, .y = 0.05f, .z = 0.05f};
    kmVec3 diffuse = {.x = 0.8f, .y = 0.8f, .z = 0.8f};
    kmVec3 specular = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
    state.main.fs_uni.pointLight[1].position[0] = pointLightPositions[1].x;
    state.main.fs_uni.pointLight[1].position[1] = pointLightPositions[1].y;
    state.main.fs_uni.pointLight[1].position[2] = pointLightPositions[1].z;
    state.main.fs_uni.pointLight[1].ambient[0] = ambient.x;
    state.main.fs_uni.pointLight[1].ambient[1] = ambient.y;
    state.main.fs_uni.pointLight[1].ambient[2] = ambient.z;
    state.main.fs_uni.pointLight[1].diffuse[0] = diffuse.x;
    state.main.fs_uni.pointLight[1].diffuse[1] = diffuse.y;
    state.main.fs_uni.pointLight[1].diffuse[2] = diffuse.z;
    state.main.fs_uni.pointLight[1].specular[0] = specular.x;
    state.main.fs_uni.pointLight[1].specular[1] = specular.y;
    state.main.fs_uni.pointLight[1].specular[2] = specular.z;
    state.main.fs_uni.pointLight[1].constant = 1.0f;
    state.main.fs_uni.pointLight[1].linear = 0.09f;
    state.main.fs_uni.pointLight[1].quadratic = 0.032f;
  }
  {
    // point light 3
    kmVec3 ambient = {.x = 0.05f, .y = 0.05f, .z = 0.05f};
    kmVec3 diffuse = {.x = 0.8f, .y = 0.8f, .z = 0.8f};
    kmVec3 specular = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
    state.main.fs_uni.pointLight[2].position[0] = pointLightPositions[2].x;
    state.main.fs_uni.pointLight[2].position[1] = pointLightPositions[2].y;
    state.main.fs_uni.pointLight[2].position[2] = pointLightPositions[2].z;
    state.main.fs_uni.pointLight[2].ambient[0] = ambient.x;
    state.main.fs_uni.pointLight[2].ambient[1] = ambient.y;
    state.main.fs_uni.pointLight[2].ambient[2] = ambient.z;
    state.main.fs_uni.pointLight[2].diffuse[0] = diffuse.x;
    state.main.fs_uni.pointLight[2].diffuse[1] = diffuse.y;
    state.main.fs_uni.pointLight[2].diffuse[2] = diffuse.z;
    state.main.fs_uni.pointLight[2].specular[0] = specular.x;
    state.main.fs_uni.pointLight[2].specular[1] = specular.y;
    state.main.fs_uni.pointLight[2].specular[2] = specular.z;
    state.main.fs_uni.pointLight[2].constant = 1.0f;
    state.main.fs_uni.pointLight[2].linear = 0.09f;
    state.main.fs_uni.pointLight[2].quadratic = 0.032f;
  }
  {
    // point light 4
    kmVec3 ambient = {.x = 0.05f, .y = 0.05f, .z = 0.05f};
    kmVec3 diffuse = {.x = 0.8f, .y = 0.8f, .z = 0.8f};
    kmVec3 specular = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
    state.main.fs_uni.pointLight[3].position[0] = pointLightPositions[3].x;
    state.main.fs_uni.pointLight[3].position[1] = pointLightPositions[3].y;
    state.main.fs_uni.pointLight[3].position[2] = pointLightPositions[3].z;
    state.main.fs_uni.pointLight[3].ambient[0] = ambient.x;
    state.main.fs_uni.pointLight[3].ambient[1] = ambient.y;
    state.main.fs_uni.pointLight[3].ambient[2] = ambient.z;
    state.main.fs_uni.pointLight[3].diffuse[0] = diffuse.x;
    state.main.fs_uni.pointLight[3].diffuse[1] = diffuse.y;
    state.main.fs_uni.pointLight[3].diffuse[2] = diffuse.z;
    state.main.fs_uni.pointLight[3].specular[0] = specular.x;
    state.main.fs_uni.pointLight[3].specular[1] = specular.y;
    state.main.fs_uni.pointLight[3].specular[2] = specular.z;
    state.main.fs_uni.pointLight[3].constant = 1.0f;
    state.main.fs_uni.pointLight[3].linear = 0.09f;
    state.main.fs_uni.pointLight[3].quadratic = 0.032f;
  }
  // spotLight
  /*
  {
    binocle_gd_set_uniform_vec3(shader, "spotLight.position", camera.position);
    binocle_gd_set_uniform_vec3(shader, "spotLight.direction", camera.front);
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
    binocle_gd_set_uniform_float(shader, "spotLight.constant", 1.0f);
    binocle_gd_set_uniform_float(shader, "spotLight.linear", 0.09f);
    binocle_gd_set_uniform_float(shader, "spotLight.quadratic", 0.032f);
    binocle_gd_set_uniform_float(shader, "spotLight.cutOff", cosf(kmDegreesToRadians(12.5f)));
    binocle_gd_set_uniform_float(shader, "spotLight.outerCutOff", cosf(kmDegreesToRadians(15.0f)));
  }
   */
}

/*
void draw_light(kmVec3 position, kmAABB2 viewport) {
  static GLfloat g_quad_vertex_buffer_data[] = {
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f
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
  kmMat4PerspectiveProjection(&projectionMatrix, camera.fov_y, viewport.max.x / viewport.max.y, camera.near_distance, camera.far_distance);

  kmMat4 viewMatrix;
  kmMat4Identity(&viewMatrix);
  kmMat4Multiply(&viewMatrix, &viewMatrix, binocle_camera_3d_get_transform_matrix(&camera));

  kmMat4 modelMatrix;
  kmMat4Identity(&modelMatrix);

  // Scale the light to a sensible size
  kmMat4 scale;
  kmMat4Scaling(&scale, 0.1f, 0.1f, 0.1f);

  // Translate the light to the position in world space
  kmMat4 trans;
  kmMat4Translation(&trans, position.x, position.y, position.z);

  // Apply the transformations to the model to bring it to world space. Order is important
  kmMat4Multiply(&modelMatrix, &modelMatrix, &trans);
  kmMat4Multiply(&modelMatrix, &modelMatrix, &scale);

  GLuint quad_vertexbuffer;
  glCheck(glGenBuffers(1, &quad_vertexbuffer));
  glCheck(glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer));
  glCheck(glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW));

  GLint pos_id;
  glCheck(pos_id = glGetAttribLocation(lamp_shader->program_id, "vertexPosition"));
  glCheck(glVertexAttribPointer(pos_id, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0));
  glCheck(glEnableVertexAttribArray(pos_id));

  binocle_gd_set_uniform_mat4(lamp_shader, "projectionMatrix", projectionMatrix);
  binocle_gd_set_uniform_mat4(lamp_shader, "viewMatrix", viewMatrix);
  binocle_gd_set_uniform_mat4(lamp_shader, "modelMatrix", modelMatrix);

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
*/

void apply_pbr_texture(binocle_material *material) {
  state.main.bind.fs_images[0] = material->albedo_texture;
  state.main.bind.fs_images[1] = material->normal_texture;
  state.main.bind.fs_images[2] = material->metallic_texture;
  state.main.bind.fs_images[3] = material->roughness_texture;
  state.main.bind.fs_images[4] = material->ao_texture;
}

void draw_pbr_mesh(binocle_gd *gd, const struct binocle_mesh *mesh, kmAABB2 viewport, struct binocle_camera_3d *camera) {
  if (camera == NULL) {
    binocle_log_warning("Missing camera for call to draw_pbr_mesh");
    return;
  }
  binocle_gd_apply_3d_gl_states();
  binocle_gd_apply_viewport(viewport);
  binocle_gd_apply_blend_mode(mesh->material->blend_mode);
  binocle_gd_apply_shader(gd, mesh->material->shader);
  apply_pbr_texture(mesh->material);

  kmMat4 projectionMatrix;
  kmMat4Identity(&projectionMatrix);
  kmMat4PerspectiveProjection(&projectionMatrix, camera->fov_y, viewport.max.x / viewport.max.y, camera->near_distance /*camera->near + camera->position.z*/, camera->far_distance /*camera->position.z + camera->far*/);

  kmMat4 viewMatrix;
  kmMat4Identity(&viewMatrix);
  kmMat4Multiply(&viewMatrix, &viewMatrix, binocle_camera_3d_get_transform_matrix(camera));

  kmMat4 modelMatrix;
  modelMatrix = mesh->transform;

  for (int i = 0 ; i < 16 ; i++) {
    state.main.vs_uni.viewMatrix[i] = viewMatrix.mat[i];
    state.main.vs_uni.projectionMatrix[i] = projectionMatrix.mat[i];
    state.main.vs_uni.modelMatrix[i] = modelMatrix.mat[i];
  }

  binocle_backend_update_buffer(state.main.vbuf, &(binocle_range){ .ptr=mesh->vertices, .size=mesh->vertex_count * sizeof(binocle_vpctn) });

  binocle_backend_apply_pipeline(state.main.pip);
  binocle_backend_apply_bindings(&state.main.bind);
  binocle_backend_apply_uniforms(BINOCLE_SHADERSTAGE_VS, 0, &BINOCLE_RANGE(state.main.vs_uni));
  binocle_backend_apply_uniforms(BINOCLE_SHADERSTAGE_FS, 0, &BINOCLE_RANGE(state.main.fs_uni));
  binocle_backend_draw(0, mesh->vertex_count, 1);
}

void do_hit_test(int mouse_x, int mouse_y, const struct binocle_mesh *mesh, kmAABB2 viewport, struct binocle_camera_3d *camera) {
  kmVec3 origin;
  kmVec3Zero(&origin);
  kmVec3 direction;
  kmVec3Zero(&direction);

  kmMat4 modelMatrix;
  kmMat4Identity(&modelMatrix);
  kmMat4Multiply(&modelMatrix, &modelMatrix, &mesh->transform);

  binocle_log_info("Mouse X: %d Y:%d", mouse_x, mouse_y);

  binocle_camera_3d_screen_to_world_ray(camera, mouse_x, mouse_y, viewport, &direction);
  origin.x = camera->position.x;
  origin.y = camera->position.y;
  origin.z = camera->position.z;
  binocle_log_info("Ori6 X: %f Y:%f Z:%f", origin.x, origin.y, origin.z);
  binocle_log_info("Dir6 X: %f Y:%f Z:%f", direction.x, direction.y, direction.z);

  mouse_position.x = origin.x;
  mouse_position.y = origin.y;
  mouse_position.z = origin.z;
  ray_end_position.x = origin.x + direction.x;
  ray_end_position.y = origin.y + direction.y;
  ray_end_position.z = origin.z + direction.z;
  float intersection_distance;

  // The sphere has a size of 2 units centered in origin, so it extends [-1, 1] in all directions
  kmVec3 aabb_min;
  kmVec3 aabb_max;
  binocle_model_calculate_mesh_bounding_box(mesh, &aabb_min, &aabb_max);
  if (binocle_collision_ray_cast_obb(origin, direction, aabb_min, aabb_max, modelMatrix, &intersection_distance)) {
    binocle_log_info("HIT! distance: %f", intersection_distance);
  }
}

void main_loop() {
  binocle_window_begin_frame(window);
  float dt = binocle_window_get_frame_time(window) / 1000.0f;
  elapsed_time += dt;

  binocle_input_update(&input);

  if (input.resized) {
    kmVec2 oldWindowSize = {.x = window->width, .y = window->height};
    window->width = input.newWindowSize.x;
    window->height = input.newWindowSize.y;
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
    binocle_camera_3d_rotate(&camera, 0.0f, 30.0f * dt, 0.0f);
  } else if (binocle_input_is_key_pressed(&input, KEY_Q)) {
    binocle_camera_3d_rotate(&camera, 0.0f, -30.0f * dt, 0.0f);
  }

  if (binocle_input_is_key_pressed(&input, KEY_T)) {
    binocle_camera_3d_rotate(&camera, 30.0f * dt, 0.0f, 0.0f);
  } else if (binocle_input_is_key_pressed(&input, KEY_G)) {
    binocle_camera_3d_rotate(&camera, -30.0f * dt, 0.0f, 0.0f);
  }

  if (binocle_input_is_key_pressed(&input, KEY_1)) {
    kmMat4 tr;
    kmMat4Translation(&tr, -camera.position.x, -camera.position.y, -camera.position.z);
    kmMat4 rot;
    kmMat4RotationY(&rot, 1.0f * dt);
    kmMat4 tr2;
    kmMat4Translation(&tr2, camera.position.x, camera.position.y, camera.position.z);
    kmMat4 cam = camera.transform_matrix;
    //kmMat4Multiply(&cam, &cam, &tr);
    kmMat4Multiply(&cam, &cam, &rot);
    //kmMat4Multiply(&cam, &cam, &tr2);
    camera.transform_matrix = cam;
    kmMat4Inverse(&camera.inverse_transform_matrix, &camera.transform_matrix);
  }

  kmAABB2 viewport;
  viewport.min.x = 0;
  viewport.min.y = 0;
  viewport.max.x = window->width;
  viewport.max.y = window->height;

  if (binocle_input_is_mouse_pressed(input, MOUSE_LEFT)) {
    int mouse_x = binocle_input_get_mouse_x(input);
    int mouse_y = binocle_input_get_mouse_y(input);
    do_hit_test(mouse_x, mouse_y, &model.meshes[0], viewport, &camera);
  }

  binocle_gd_clear(binocle_color_black());
  binocle_gd_apply_viewport(viewport);
  binocle_gd_apply_shader(&gd, shader);
  //binocle_gd_draw_test_triangle(shader);
  //binocle_gd_draw_test_cube(shader);

  setup_lights();
  for (int i = 0 ; i < 4 ; i++) {
    kmMat4 lightMat;
    kmMat4Identity(&lightMat);

    kmMat4 tr;
    kmMat4Translation(&tr, -pointLightPositions[i].x, -pointLightPositions[i].y, -pointLightPositions[i].z);
    kmMat4 rot;
    kmMat4RotationY(&rot, 1.0f * dt);
    kmMat4 tr2;
    kmMat4Translation(&tr2, pointLightPositions[i].x, pointLightPositions[i].y, pointLightPositions[i].z);

    kmMat4Multiply(&lightMat, &rot, &tr2);

    kmVec3 trv;
    kmMat4ExtractTranslationVec3(&lightMat, &trv);
    pointLightPositions[i].x = trv.x;
    pointLightPositions[i].y = trv.y;
    pointLightPositions[i].z = trv.z;

    /*
    pointLightPositions[i].x = pointLightPositions[i].x + sinf(elapsed_time * 5.0f) * 0.07f;
    pointLightPositions[i].y = pointLightPositions[i].y + sinf(elapsed_time * 2.0f) * 0.07f;
    pointLightPositions[i].z = pointLightPositions[i].z + sinf(elapsed_time * 3.0f) * 0.07f;
     */
  }

  kmMat4Identity(&model.meshes[0].transform);

  binocle_backend_begin_default_pass(&state.main.action, window->width, window->height);

  draw_pbr_mesh(&gd, &model.meshes[0], viewport, &camera);

//  binocle_gd_apply_shader(&gd, lamp_shader);
//  for (int i = 0 ; i < 4 ; i++) {
//    draw_light(pointLightPositions[i], viewport);
//  }

//  draw_light(mouse_position, viewport);
//  draw_light(ray_end_position, viewport);

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

  window = binocle_window_new(DESIGN_WIDTH, DESIGN_HEIGHT, "Binocle 3D Model Example");
  binocle_window_set_background_color(window, binocle_color_white());
  binocle_window_set_minimum_size(window, DESIGN_WIDTH, DESIGN_HEIGHT);
  kmVec3 camera_pos;
  camera_pos.x = 0;
  camera_pos.y = 0;
  camera_pos.z = 3.0f;
  camera = binocle_camera_3d_new(camera_pos, 0.1f, 1000.0f, 60);
  //binocle_camera_3d_set_rotation(&camera, 0, -90, 0);
  input = binocle_input_new();
  gd = binocle_gd_new();
  binocle_gd_init(&gd, window);

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

  binocle_shader_desc default_shader_desc = {
    .vs.source = shader_vs_src,
    .attrs = {
      [0].name = "vertexPosition",
      [1].name = "vertexColor",
      [2].name = "vertexTCoord",
      [3].name = "vertexNormal",
    },
    .vs.uniform_blocks[0] = {
      .size = sizeof(default_shader_vs_uniforms_t),
      .uniforms = {
        [0] = { .name = "projectionMatrix", .type = BINOCLE_UNIFORMTYPE_MAT4},
        [1] = { .name = "viewMatrix", .type = BINOCLE_UNIFORMTYPE_MAT4},
        [2] = { .name = "modelMatrix", .type = BINOCLE_UNIFORMTYPE_MAT4},
      }
    },
    .fs = {
      .source = shader_fs_src,
      .uniform_blocks[0] = {
        .size = sizeof(default_shader_fs_uniforms_t),
        .uniforms = {
          [0] = { .name = "viewPos", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [1] = { .name = "dirLight.direction", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [2] = { .name = "dirLight.ambient", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [3] = { .name = "dirLight.diffuse", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [4] = { .name = "dirLight.specular", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [5] = { .name = "pointLights[0].position", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [6] = { .name = "pointLights[0].ambient", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [7] = { .name = "pointLights[0].diffuse", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [8] = { .name = "pointLights[0].specular", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [9] = { .name = "pointLights[0].constant", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [10] = { .name = "pointLights[0].linear", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [11] = { .name = "pointLights[0].quadratic", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [12] = { .name = "pointLights[1].position", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [13] = { .name = "pointLights[1].ambient", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [14] = { .name = "pointLights[1].diffuse", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [15] = { .name = "pointLights[1].specular", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [16] = { .name = "pointLights[1].constant", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [17] = { .name = "pointLights[1].linear", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [18] = { .name = "pointLights[1].quadratic", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [19] = { .name = "pointLights[2].position", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [20] = { .name = "pointLights[2].ambient", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [21] = { .name = "pointLights[2].diffuse", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [22] = { .name = "pointLights[2].specular", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [23] = { .name = "pointLights[2].constant", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [24] = { .name = "pointLights[2].linear", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [25] = { .name = "pointLights[2].quadratic", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [26] = { .name = "pointLights[3].position", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [27] = { .name = "pointLights[3].ambient", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [28] = { .name = "pointLights[3].diffuse", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [29] = { .name = "pointLights[3].specular", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [30] = { .name = "pointLights[3].constant", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [31] = { .name = "pointLights[3].linear", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [32] = { .name = "pointLights[3].quadratic", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [33] = { .name = "spotLight.position", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [34] = { .name = "spotLight.direction", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [35] = { .name = "spotLight.ambient", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [36] = { .name = "spotLight.diffuse", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [37] = { .name = "spotLight.specular", .type = BINOCLE_UNIFORMTYPE_FLOAT3},
          [38] = { .name = "spotLight.constant", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [39] = { .name = "spotLight.linear", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [40] = { .name = "spotLight.quadratic", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [41] = { .name = "spotLight.cutOff", .type = BINOCLE_UNIFORMTYPE_FLOAT},
          [42] = { .name = "spotLight.outerCutOff", .type = BINOCLE_UNIFORMTYPE_FLOAT},
        }
      },
      .images = {
        [0] = { .name = "material.albedoMap", .type = BINOCLE_IMAGETYPE_2D},
        [1] = { .name = "material.normalMap", .type = BINOCLE_IMAGETYPE_2D},
        [2] = { .name = "material.metallicMap", .type = BINOCLE_IMAGETYPE_2D},
        [3] = { .name = "material.roughnessMap", .type = BINOCLE_IMAGETYPE_2D},
        [4] = { .name = "material.aoMap", .type = BINOCLE_IMAGETYPE_2D},
        },
      },
  };
  shader = binocle_backend_make_shader(&default_shader_desc);


  sprintf(vert, "%s%s", binocle_data_dir, "lamp.vert");
  sprintf(frag, "%s%s", binocle_data_dir, "lamp.frag");

  char *lamp_shader_vs_src;
  size_t lamp_shader_vs_src_size;
  binocle_sdl_load_text_file(vert, &lamp_shader_vs_src, &lamp_shader_vs_src_size);

  char *lamp_shader_fs_src;
  size_t lamp_shader_fs_src_size;
  binocle_sdl_load_text_file(frag, &lamp_shader_fs_src, &lamp_shader_fs_src_size);

  binocle_shader_desc lamp_shader_desc = {
    .vs.source = lamp_shader_vs_src,
    .attrs = {
      [0].name = "vertexPosition",
    },
    .vs.uniform_blocks[0] = {
      .size = sizeof(lamp_shader_uniforms_t),
      .uniforms = {
        [0] = { .name = "projectionMatrix", .type = BINOCLE_UNIFORMTYPE_MAT4},
        [1] = { .name = "viewMatrix", .type = BINOCLE_UNIFORMTYPE_MAT4},
        [2] = { .name = "modelMatrix", .type = BINOCLE_UNIFORMTYPE_MAT4},
      }
    },
    .fs.source = lamp_shader_fs_src,
  };
  lamp_shader = binocle_backend_make_shader(&lamp_shader_desc);


  char filename[1024];
  sprintf(filename, "%s%s", binocle_data_dir, "sphere.model");
  char mtl_filename[1024];
  sprintf(mtl_filename, "%s%s", binocle_data_dir, "sphere.mtl");
  model = binocle_model_load_obj(filename, mtl_filename);
  /*
  kmMat4 scale;
  kmMat4Scaling(&scale, 0.000005f, 0.000005f, 0.000005f);
  kmMat4Multiply(&model.meshes[0].transform, &model.meshes[0].transform, &scale);
   */


  sprintf(filename, "%s%s", binocle_data_dir, "rustediron2_albedo.png");
  albedo_image = binocle_image_load(filename);
  model.meshes[0].material->albedo_texture = albedo_image;
  model.meshes[0].material->shader = shader;

  sprintf(filename, "%s%s", binocle_data_dir, "rustediron2_normal.png");
  normal_image = binocle_image_load(filename);
  model.meshes[0].material->normal_texture = normal_image;

  sprintf(filename, "%s%s", binocle_data_dir, "rustediron2_metallic.png");
  metallic_image = binocle_image_load(filename);
  model.meshes[0].material->metallic_texture = metallic_image;

  sprintf(filename, "%s%s", binocle_data_dir, "rustediron2_roughness.png");
  roughness_image = binocle_image_load(filename);
  model.meshes[0].material->roughness_texture = roughness_image;

  sprintf(filename, "%s%s", binocle_data_dir, "rustediron2_ao.png");
  ao_image = binocle_image_load(filename);
  model.meshes[0].material->ao_texture = ao_image;

  setup_default_pipeline();

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  while (!input.quit_requested) {
    main_loop();
  }
#endif
  binocle_log_info("Quit requested");
  free(binocle_data_dir);
  binocle_image_destroy(albedo_image);
  binocle_image_destroy(normal_image);
  binocle_image_destroy(metallic_image);
  binocle_image_destroy(roughness_image);
  binocle_image_destroy(ao_image);
  binocle_app_destroy(&app);
}


