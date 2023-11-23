//
//  Binocle
//  Copyright(C)2015-2021 Valerio Santinelli
//

#include <stdio.h>
#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#endif
#include "binocle_sdl.h"
#include "backend/binocle_color.h"
#include "binocle_window.h"
#include "binocle_game.h"
#include "binocle_camera.h"
#include <binocle_input.h>
#include <binocle_image.h>
#include <binocle_sprite.h>
#include <backend/binocle_material.h>
#include <backend/binocle_color.h>
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
  shader_spot_light_t spotLight;
} default_shader_fs_uniforms_t;

typedef struct default_shader_fs_uniforms_point_t {
  shader_point_light_t pointLight[2];
} default_shader_fs_uniforms_point_t;

typedef struct lamp_shader_uniforms_t {
  float projectionMatrix[16];
  float viewMatrix[16];
  float modelMatrix[16];
} lamp_shader_uniforms_t;

typedef struct state_t {
  struct {
    sg_pass_action action;
    sg_pipeline pip;
    sg_bindings bind;
    default_shader_vs_uniforms_t vs_uni;
    default_shader_fs_uniforms_t fs_uni;
    default_shader_fs_uniforms_point_t fs_point1_uni;
    default_shader_fs_uniforms_point_t fs_point2_uni;
    sg_buffer vbuf;
  } main;
  struct {
    sg_pass_action action;
    sg_pipeline pip;
    sg_bindings bind;
    default_shader_vs_uniforms_t vs_uni;
    default_shader_fs_uniforms_t fs_uni;
    sg_buffer vbuf;
    sg_buffer ibuf;
  } lamp;
} state_t;

binocle_window *window;
binocle_input input;
binocle_camera_3d camera;
binocle_gd gd;
sg_shader shader;
sg_shader lamp_shader;
char *binocle_data_dir = NULL;
binocle_app app;
sg_image albedo_image;
sg_image normal_image;
sg_image metallic_image;
sg_image roughness_image;
sg_image ao_image;
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
  sg_color off_clear_color = binocle_color_black();
  sg_pass_action action = {
    .colors[0] = {
      .action = SG_ACTION_CLEAR,
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
  state.main.pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
      .attrs = {
        [0] = { .format = SG_VERTEXFORMAT_FLOAT3 }, // position
        [1] = { .format = SG_VERTEXFORMAT_FLOAT4 }, // color
        [2] = { .format = SG_VERTEXFORMAT_FLOAT2 }, // texture uv
        [3] = { .format = SG_VERTEXFORMAT_FLOAT3 }, // normal
      }
    },
    .shader = shader,
    .index_type = SG_INDEXTYPE_NONE,
    .colors = {
      [0] = { .pixel_format = SG_PIXELFORMAT_RGBA8 },
    },
    .cull_mode = SG_CULLMODE_FRONT,
    .depth = {
      .compare = SG_COMPAREFUNC_ALWAYS,
    }
  });

  sg_buffer_desc vbuf_desc = {
    .type = SG_BUFFERTYPE_VERTEXBUFFER,
    .usage = SG_USAGE_STREAM,
    .size = sizeof(binocle_vpctn) * MAX_VERTICES,
  };
  state.main.vbuf = sg_make_buffer(&vbuf_desc);

  state.main.bind = (sg_bindings){
    .vertex_buffers = {
      [0] = state.main.vbuf,
    },
  };
}

void setup_lamp_pipeline() {
  // Clear screen action for the lamp scene
  sg_color off_clear_color = binocle_color_green();
  sg_pass_action action = {
    .colors[0] = {
      .action = SG_ACTION_CLEAR,
      .value = {
        .r = off_clear_color.r,
        .g = off_clear_color.g,
        .b = off_clear_color.b,
        .a = off_clear_color.a,
      }
    }
  };
  state.lamp.action = action;

  // Pipeline state object for the screen (default) pass
  state.lamp.pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
      .attrs = {
        [0] = { .format = SG_VERTEXFORMAT_FLOAT3 }, // position
      }
    },
    .shader = lamp_shader,
    .index_type = SG_INDEXTYPE_UINT32,
    .colors = {
      [0] = { .pixel_format = SG_PIXELFORMAT_RGBA8 },
    },
    .cull_mode = SG_CULLMODE_FRONT,
    .depth = {
      .compare = SG_COMPAREFUNC_ALWAYS,
    }
  });

  sg_buffer_desc vbuf_desc = {
    .type = SG_BUFFERTYPE_VERTEXBUFFER,
    .usage = SG_USAGE_STREAM,
    .size = sizeof(GLfloat) * 3 * MAX_VERTICES,
  };
  state.lamp.vbuf = sg_make_buffer(&vbuf_desc);

  sg_buffer_desc ibuf_desc = {
    .type = SG_BUFFERTYPE_INDEXBUFFER,
    .usage = SG_USAGE_STREAM,
    .size = sizeof(GLfloat) * MAX_VERTICES,
  };
  state.lamp.ibuf = sg_make_buffer(&ibuf_desc);

  state.lamp.bind = (sg_bindings){
    .vertex_buffers = {
      [0] = state.lamp.vbuf,
    },
    .index_buffer = state.lamp.ibuf,
  };
}

void setup_lights() {
  {
    // directional light
    kmVec3 direction = {.x = -0.2f, .y = -1.0f, .z = -0.3f};
    kmVec3 ambient = { .x = 0.1f, .y = 0.1f, .z = 0.1f};
    kmVec3 diffuse = {.x = 0.4f, .y = 0.4f, .z = 0.4f};
    kmVec3 specular = {.x = 0.5f, .y = 0.5f, .z = 0.5f};
    state.main.fs_uni.dirLight.direction[0] = direction.x;
    state.main.fs_uni.dirLight.direction[1] = direction.y;
    state.main.fs_uni.dirLight.direction[2] = direction.z;
    state.main.fs_uni.dirLight.ambient[0] = ambient.x;
    state.main.fs_uni.dirLight.ambient[1] = ambient.y;
    state.main.fs_uni.dirLight.ambient[2] = ambient.z;
    state.main.fs_uni.dirLight.diffuse[0] = diffuse.x;
    state.main.fs_uni.dirLight.diffuse[1] = diffuse.y;
    state.main.fs_uni.dirLight.diffuse[2] = diffuse.z;
    state.main.fs_uni.dirLight.specular[0] = specular.x;
    state.main.fs_uni.dirLight.specular[1] = specular.y;
    state.main.fs_uni.dirLight.specular[2] = specular.z;
  }

  state.main.fs_uni.viewPos[0] = camera.position.x;
  state.main.fs_uni.viewPos[1] = camera.position.y;
  state.main.fs_uni.viewPos[2] = camera.position.z;
  {
    // point light 1
    kmVec3 ambient = {.x = 0.05f, .y = 0.05f, .z = 0.05f};
    kmVec3 diffuse = {.x = 0.8f, .y = 0.8f, .z = 0.8f};
    kmVec3 specular = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
    state.main.fs_point1_uni.pointLight[0].position[0] = pointLightPositions[0].x;
    state.main.fs_point1_uni.pointLight[0].position[1] = pointLightPositions[0].y;
    state.main.fs_point1_uni.pointLight[0].position[2] = pointLightPositions[0].z;
    state.main.fs_point1_uni.pointLight[0].ambient[0] = ambient.x;
    state.main.fs_point1_uni.pointLight[0].ambient[1] = ambient.y;
    state.main.fs_point1_uni.pointLight[0].ambient[2] = ambient.z;
    state.main.fs_point1_uni.pointLight[0].diffuse[0] = diffuse.x;
    state.main.fs_point1_uni.pointLight[0].diffuse[1] = diffuse.y;
    state.main.fs_point1_uni.pointLight[0].diffuse[2] = diffuse.z;
    state.main.fs_point1_uni.pointLight[0].specular[0] = specular.x;
    state.main.fs_point1_uni.pointLight[0].specular[1] = specular.y;
    state.main.fs_point1_uni.pointLight[0].specular[2] = specular.z;
    state.main.fs_point1_uni.pointLight[0].constant = 1.0f;
    state.main.fs_point1_uni.pointLight[0].linear = 0.09f;
    state.main.fs_point1_uni.pointLight[0].quadratic = 0.032f;
  }
  {
    // point light 2
    kmVec3 ambient = {.x = 0.05f, .y = 0.05f, .z = 0.05f};
    kmVec3 diffuse = {.x = 0.8f, .y = 0.8f, .z = 0.8f};
    kmVec3 specular = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
    state.main.fs_point1_uni.pointLight[1].position[0] = pointLightPositions[1].x;
    state.main.fs_point1_uni.pointLight[1].position[1] = pointLightPositions[1].y;
    state.main.fs_point1_uni.pointLight[1].position[2] = pointLightPositions[1].z;
    state.main.fs_point1_uni.pointLight[1].ambient[0] = ambient.x;
    state.main.fs_point1_uni.pointLight[1].ambient[1] = ambient.y;
    state.main.fs_point1_uni.pointLight[1].ambient[2] = ambient.z;
    state.main.fs_point1_uni.pointLight[1].diffuse[0] = diffuse.x;
    state.main.fs_point1_uni.pointLight[1].diffuse[1] = diffuse.y;
    state.main.fs_point1_uni.pointLight[1].diffuse[2] = diffuse.z;
    state.main.fs_point1_uni.pointLight[1].specular[0] = specular.x;
    state.main.fs_point1_uni.pointLight[1].specular[1] = specular.y;
    state.main.fs_point1_uni.pointLight[1].specular[2] = specular.z;
    state.main.fs_point1_uni.pointLight[1].constant = 1.0f;
    state.main.fs_point1_uni.pointLight[1].linear = 0.09f;
    state.main.fs_point1_uni.pointLight[1].quadratic = 0.032f;
  }
  {
    // point light 3
    kmVec3 ambient = {.x = 0.05f, .y = 0.05f, .z = 0.05f};
    kmVec3 diffuse = {.x = 0.8f, .y = 0.8f, .z = 0.8f};
    kmVec3 specular = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
    state.main.fs_point2_uni.pointLight[0].position[0] = pointLightPositions[2].x;
    state.main.fs_point2_uni.pointLight[0].position[1] = pointLightPositions[2].y;
    state.main.fs_point2_uni.pointLight[0].position[2] = pointLightPositions[2].z;
    state.main.fs_point2_uni.pointLight[0].ambient[0] = ambient.x;
    state.main.fs_point2_uni.pointLight[0].ambient[1] = ambient.y;
    state.main.fs_point2_uni.pointLight[0].ambient[2] = ambient.z;
    state.main.fs_point2_uni.pointLight[0].diffuse[0] = diffuse.x;
    state.main.fs_point2_uni.pointLight[0].diffuse[1] = diffuse.y;
    state.main.fs_point2_uni.pointLight[0].diffuse[2] = diffuse.z;
    state.main.fs_point2_uni.pointLight[0].specular[0] = specular.x;
    state.main.fs_point2_uni.pointLight[0].specular[1] = specular.y;
    state.main.fs_point2_uni.pointLight[0].specular[2] = specular.z;
    state.main.fs_point2_uni.pointLight[0].constant = 1.0f;
    state.main.fs_point2_uni.pointLight[0].linear = 0.09f;
    state.main.fs_point2_uni.pointLight[0].quadratic = 0.032f;
  }
  {
    // point light 4
    kmVec3 ambient = {.x = 0.05f, .y = 0.05f, .z = 0.05f};
    kmVec3 diffuse = {.x = 0.8f, .y = 0.8f, .z = 0.8f};
    kmVec3 specular = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
    state.main.fs_point2_uni.pointLight[1].position[0] = pointLightPositions[3].x;
    state.main.fs_point2_uni.pointLight[1].position[1] = pointLightPositions[3].y;
    state.main.fs_point2_uni.pointLight[1].position[2] = pointLightPositions[3].z;
    state.main.fs_point2_uni.pointLight[1].ambient[0] = ambient.x;
    state.main.fs_point2_uni.pointLight[1].ambient[1] = ambient.y;
    state.main.fs_point2_uni.pointLight[1].ambient[2] = ambient.z;
    state.main.fs_point2_uni.pointLight[1].diffuse[0] = diffuse.x;
    state.main.fs_point2_uni.pointLight[1].diffuse[1] = diffuse.y;
    state.main.fs_point2_uni.pointLight[1].diffuse[2] = diffuse.z;
    state.main.fs_point2_uni.pointLight[1].specular[0] = specular.x;
    state.main.fs_point2_uni.pointLight[1].specular[1] = specular.y;
    state.main.fs_point2_uni.pointLight[1].specular[2] = specular.z;
    state.main.fs_point2_uni.pointLight[1].constant = 1.0f;
    state.main.fs_point2_uni.pointLight[1].linear = 0.09f;
    state.main.fs_point2_uni.pointLight[1].quadratic = 0.032f;
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

  for (int i = 0 ; i < 16 ; i++) {
    state.lamp.vs_uni.viewMatrix[i] = viewMatrix.mat[i];
    state.lamp.vs_uni.projectionMatrix[i] = projectionMatrix.mat[i];
    state.lamp.vs_uni.modelMatrix[i] = modelMatrix.mat[i];
  }

  const uint32_t vbuf_offset = sg_append_buffer(state.lamp.vbuf, &(sg_range){ .ptr=g_quad_vertex_buffer_data, .size= 3 * 8 * sizeof(GLfloat) });
  const uint32_t ibuf_offset = sg_append_buffer(state.lamp.ibuf, &(sg_range){ .ptr=index_buffer_data, .size= 3 * 12 * sizeof(GLuint) });

  state.lamp.bind.vertex_buffer_offsets[0] = vbuf_offset;
  state.lamp.bind.index_buffer_offset = ibuf_offset;

  sg_apply_pipeline(state.lamp.pip);
  sg_apply_bindings(&state.lamp.bind);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(state.lamp.vs_uni));
//  binocle_backend_apply_uniforms(BINOCLE_SHADERSTAGE_FS, 0, &BINOCLE_RANGE(state.lamp.fs_uni));
  sg_draw(0, 3 * 12, 1);




//  GLuint quad_vertexbuffer;
//  glCheck(glGenBuffers(1, &quad_vertexbuffer));
//  glCheck(glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer));
//  glCheck(glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW));
//
//  GLint pos_id;
//  glCheck(pos_id = glGetAttribLocation(lamp_shader->program_id, "vertexPosition"));
//  glCheck(glVertexAttribPointer(pos_id, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0));
//  glCheck(glEnableVertexAttribArray(pos_id));
//
//  binocle_gd_set_uniform_mat4(lamp_shader, "projectionMatrix", projectionMatrix);
//  binocle_gd_set_uniform_mat4(lamp_shader, "viewMatrix", viewMatrix);
//  binocle_gd_set_uniform_mat4(lamp_shader, "modelMatrix", modelMatrix);
//
//  GLuint quad_indexbuffer;
//  glCheck(glGenBuffers(1, &quad_indexbuffer));
//  glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_indexbuffer));
//  glCheck(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW));
//  glCheck(glDrawElements(GL_TRIANGLES, 12 * 3, GL_UNSIGNED_INT, 0));
//
//  glCheck(glDisableVertexAttribArray(pos_id));
//  glCheck(glBindBuffer(GL_ARRAY_BUFFER, 0));
//  glCheck(glDeleteBuffers(1, &quad_vertexbuffer));
//  glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
//  glCheck(glDeleteBuffers(1, &quad_indexbuffer));
}

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

  sg_update_buffer(state.main.vbuf, &(sg_range){ .ptr=mesh->vertices, .size=mesh->vertex_count * sizeof(binocle_vpctn) });

  sg_apply_pipeline(state.main.pip);
  sg_apply_bindings(&state.main.bind);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(state.main.vs_uni));
  sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &SG_RANGE(state.main.fs_uni));
  sg_draw(0, mesh->vertex_count, 1);
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

  sg_begin_default_pass(&state.main.action, window->width, window->height);

  draw_pbr_mesh(&gd, &model.meshes[0], viewport, &camera);

//  binocle_gd_apply_shader(&gd, lamp_shader);
  for (int i = 0 ; i < 4 ; i++) {
    draw_light(pointLightPositions[i], viewport);
  }

  draw_light(mouse_position, viewport);
  draw_light(ray_end_position, viewport);

  sg_end_pass();

  sg_commit();

  binocle_window_refresh(window);
  binocle_window_end_frame(window);
}

int main(int argc, char *argv[])
{
  app = binocle_app_new();
  binocle_app_desc_t app_desc = { 0 };
  binocle_app_init(&app, &app_desc);

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

  sg_shader_desc default_shader_desc = {
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
        [0] = {.name = "projectionMatrix", .type = SG_UNIFORMTYPE_MAT4},
        [1] = {.name = "viewMatrix", .type = SG_UNIFORMTYPE_MAT4},
        [2] = {.name = "modelMatrix", .type = SG_UNIFORMTYPE_MAT4},
      }
    },
    .fs = {
      .source = shader_fs_src,
      .uniform_blocks = {
        [0] = {
          .size = sizeof(default_shader_fs_uniforms_t),
          .uniforms = {
            [0] = {.name = "viewPos", .type = SG_UNIFORMTYPE_FLOAT3},
            [1] = {.name = "dirLight.direction", .type = SG_UNIFORMTYPE_FLOAT3},
            [2] = {.name = "dirLight.ambient", .type = SG_UNIFORMTYPE_FLOAT3},
            [3] = {.name = "dirLight.diffuse", .type = SG_UNIFORMTYPE_FLOAT3},
            [4] = {.name = "dirLight.specular", .type = SG_UNIFORMTYPE_FLOAT3},
            [5] = {.name = "spotLight.position", .type = SG_UNIFORMTYPE_FLOAT3},
            [6] = {.name = "spotLight.direction", .type = SG_UNIFORMTYPE_FLOAT3},
            [7] = {.name = "spotLight.ambient", .type = SG_UNIFORMTYPE_FLOAT3},
            [8] = {.name = "spotLight.diffuse", .type = SG_UNIFORMTYPE_FLOAT3},
            [9] = {.name = "spotLight.specular", .type = SG_UNIFORMTYPE_FLOAT3},
            [10] = {.name = "spotLight.constant", .type = SG_UNIFORMTYPE_FLOAT},
            [11] = {.name = "spotLight.linear", .type = SG_UNIFORMTYPE_FLOAT},
            [12] = {.name = "spotLight.quadratic", .type = SG_UNIFORMTYPE_FLOAT},
            [13] = {.name = "spotLight.cutOff", .type = SG_UNIFORMTYPE_FLOAT},
            [14] = {.name = "spotLight.outerCutOff", .type = SG_UNIFORMTYPE_FLOAT},
          },
        },
        [1] = {
          .size = sizeof(default_shader_fs_uniforms_point_t),
          .uniforms = {
            [0] = {.name = "pointLights[0].position", .type = SG_UNIFORMTYPE_FLOAT3},
            [1] = {.name = "pointLights[0].ambient", .type = SG_UNIFORMTYPE_FLOAT3},
            [2] = {.name = "pointLights[0].diffuse", .type = SG_UNIFORMTYPE_FLOAT3},
            [3] = {.name = "pointLights[0].specular", .type = SG_UNIFORMTYPE_FLOAT3},
            [4] = {.name = "pointLights[0].constant", .type = SG_UNIFORMTYPE_FLOAT},
            [5] = {.name = "pointLights[0].linear", .type = SG_UNIFORMTYPE_FLOAT},
            [6] = {.name = "pointLights[0].quadratic", .type = SG_UNIFORMTYPE_FLOAT},
            [7] = {.name = "pointLights[1].position", .type = SG_UNIFORMTYPE_FLOAT3},
            [8] = {.name = "pointLights[1].ambient", .type = SG_UNIFORMTYPE_FLOAT3},
            [9] = {.name = "pointLights[1].diffuse", .type = SG_UNIFORMTYPE_FLOAT3},
            [10] = {.name = "pointLights[1].specular", .type = SG_UNIFORMTYPE_FLOAT3},
            [11] = {.name = "pointLights[1].constant", .type = SG_UNIFORMTYPE_FLOAT},
            [12] = {.name = "pointLights[1].linear", .type = SG_UNIFORMTYPE_FLOAT},
            [13] = {.name = "pointLights[1].quadratic", .type = SG_UNIFORMTYPE_FLOAT},
          },
        },
        [2] = {
          .size = sizeof(default_shader_fs_uniforms_point_t),
          .uniforms = {
            [0] = {.name = "pointLights[2].position", .type = SG_UNIFORMTYPE_FLOAT3},
            [1] = {.name = "pointLights[2].ambient", .type = SG_UNIFORMTYPE_FLOAT3},
            [2] = {.name = "pointLights[2].diffuse", .type = SG_UNIFORMTYPE_FLOAT3},
            [3] = {.name = "pointLights[2].specular", .type = SG_UNIFORMTYPE_FLOAT3},
            [4] = {.name = "pointLights[2].constant", .type = SG_UNIFORMTYPE_FLOAT},
            [5] = {.name = "pointLights[2].linear", .type = SG_UNIFORMTYPE_FLOAT},
            [6] = {.name = "pointLights[2].quadratic", .type = SG_UNIFORMTYPE_FLOAT},
            [7] = {.name = "pointLights[3].position", .type = SG_UNIFORMTYPE_FLOAT3},
            [8] = {.name = "pointLights[3].ambient", .type = SG_UNIFORMTYPE_FLOAT3},
            [9] = {.name = "pointLights[3].diffuse", .type = SG_UNIFORMTYPE_FLOAT3},
            [10] = {.name = "pointLights[3].specular", .type = SG_UNIFORMTYPE_FLOAT3},
            [11] = {.name = "pointLights[3].constant", .type = SG_UNIFORMTYPE_FLOAT},
            [12] = {.name = "pointLights[3].linear", .type = SG_UNIFORMTYPE_FLOAT},
            [13] = {.name = "pointLights[3].quadratic", .type = SG_UNIFORMTYPE_FLOAT},
          },
        },
      },
      .images = {
        [0] = {.name = "material.albedoMap", .image_type = SG_IMAGETYPE_2D},
        [1] = {.name = "material.normalMap", .image_type = SG_IMAGETYPE_2D},
        [2] = {.name = "material.metallicMap", .image_type = SG_IMAGETYPE_2D},
        [3] = {.name = "material.roughnessMap", .image_type = SG_IMAGETYPE_2D},
        [4] = {.name = "material.aoMap", .image_type = SG_IMAGETYPE_2D},
      },
    },
  };
  shader = sg_make_shader(&default_shader_desc);


  sprintf(vert, "%s%s", binocle_data_dir, "lamp.vert");
  sprintf(frag, "%s%s", binocle_data_dir, "lamp.frag");

  char *lamp_shader_vs_src;
  size_t lamp_shader_vs_src_size;
  binocle_sdl_load_text_file(vert, &lamp_shader_vs_src, &lamp_shader_vs_src_size);

  char *lamp_shader_fs_src;
  size_t lamp_shader_fs_src_size;
  binocle_sdl_load_text_file(frag, &lamp_shader_fs_src, &lamp_shader_fs_src_size);

  sg_shader_desc lamp_shader_desc = {
    .vs.source = lamp_shader_vs_src,
    .attrs = {
      [0].name = "vertexPosition",
    },
    .vs.uniform_blocks[0] = {
      .size = sizeof(lamp_shader_uniforms_t),
      .uniforms = {
        [0] = { .name = "projectionMatrix", .type = SG_UNIFORMTYPE_MAT4},
        [1] = { .name = "viewMatrix", .type = SG_UNIFORMTYPE_MAT4},
        [2] = { .name = "modelMatrix", .type = SG_UNIFORMTYPE_MAT4},
      }
    },
    .fs.source = lamp_shader_fs_src,
  };
  lamp_shader = sg_make_shader(&lamp_shader_desc);


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
  setup_lamp_pipeline();

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


