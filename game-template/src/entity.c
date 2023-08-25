//
// Created by Valerio Santinelli on 19/07/23.
//

#include "entity.h"
#include "binocle_image.h"
#include "backend/binocle_material.h"
#include "binocle_sdl.h"
#include "binocle_log.h"
#include "binocle_ttfont.h"
#include "cache.h"
#include "level.h"
#include "stb_ds.h"
#include "cooldown.h"

extern struct game_t game;
extern struct entity_t entities[MAX_ENTITIES];

void entity_system_init() {
  memset(entities, 0, MAX_ENTITIES * sizeof(entities[0]));
  game.entities = entities;
}

void entity_system_update() {
  for (int i = 0 ; i < game.num_entities ; i++) {
    entity_t *e = &game.entities[i];
    if (e->destroyed) {
      entity_free(e);
    }
  }
}

static void entity_init(entity_t *en) {
  en->in_use = true;
  en->cx = 0;
  en->cy = 0;
  en->xr = 0.5f;
  en->yr = 0;

  en->dx = 0;
  en->dy = 0;
  en->bdx = 0;
  en->bdy = 0;

  en->gravity = 0.025f;

  en->frict = 0.82f;
  en->bump_frict = 0.93f;

  en->hei = GRID;
  en->wid = GRID;
  en->radius = GRID * 0.5f;

  en->depth = 0;

  en->pivot_x = 0.5f;
  en->pivot_y = 0;

  en->sprite = NULL;
  en->frames = NULL;

  en->sprite_x = 0;
  en->sprite_y = 0;
  en->sprite_scale_x = 1;
  en->sprite_scale_y = 1;
  en->sprite_scale_set_x = 1;
  en->sprite_scale_set_y = 1;
  en->visible = true;

  en->time_mul = 1;
  en->dir = 1;

  en->destroyed = false;

  en->has_collisions = true;

  en->name = SDL_strdup("entity");

  en->parent = NULL;

  cooldown_system_init(&en->pools, 16);
}

entity_t *entity_spawn() {
  entity_t *e = NULL;
  for (int i = 0; i < game.num_entities; i++) {
    e = &game.entities[i];
    if (e->in_use) {
      continue;
    }

    entity_init(e);
    return e;
  }

  if (game.num_entities == MAX_ENTITIES) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
      binocle_log_info("%4i: %s", i, game.entities[i].name);
    }
    binocle_log_error("entity_spawn: no free entities");
    return NULL;
  }

  e = &game.entities[game.num_entities];
  game.num_entities++;
  entity_init(e);
  return e;
}

void entity_free(entity_t *e) {
  cooldown_system_shutdown(&e->pools);
  memset(e, 0, sizeof(*e));
  e->name = "freed";
}

void entity_draw(entity_t *e) {
  if (e->visible && e->sprite != NULL) {
    kmVec2 scale;
    scale.x = e->sprite_scale_x;
    scale.y = e->sprite_scale_y;
    kmAABB2 viewport = binocle_camera_get_viewport(game.gfx.camera);
    binocle_sprite_draw_with_sprite_batch(&game.gfx.sprite_batch, e->sprite, &game.gfx.gd, e->sprite_x, e->sprite_y,
                                          &viewport, 0, &scale, &game.gfx.camera, e->depth);
  }
}

void entity_update(entity_t *e, float dt) {
  if (e->on_pre_update != NULL) {
    e->on_pre_update(e, dt);
  }
  level_t *level = &game.level;

  // X
  float steps = ceilf(fabsf((e->dx + e->bdx) * e->time_mul));
  float step = ((e->dx + e->bdx) * e->time_mul) / steps;
  while (steps > 0) {
    e->xr += step;
    // Add X collision checks
    if (e->on_pre_step_x != NULL) {
      e->on_pre_step_x(e, level);
    } else {
      entity_on_pre_step_x(e, level);
    }
    while (e->xr > 1) {
      e->xr -= 1;
      e->cx += 1;
    }
    while (e->xr < 0) {
      e->xr += 1;
      e->cx -= 1;
    }
    steps -= 1;
  }
  e->dx *= powf(e->frict, e->time_mul);
  e->bdx *= powf(e->bump_frict, e->time_mul);
  if (fabsf(e->dx) <= 0.0005f * e->time_mul) {
    e->dx = 0;
  }
  if (fabsf(e->bdx) <= 0.0005f * e->time_mul) {
    e->bdx = 0;
  }

  // Y
  if (!entity_on_ground(e)) {
    e->dy -= e->gravity;
  }
  steps = ceilf(fabsf((e->dy + e->bdy) * e->time_mul));
  step = ((e->dy + e->bdy) * e->time_mul) / steps;
  while (steps > 0) {
    e->yr += step;
    // Add Y collision checks
    if (e->on_pre_step_y != NULL) {
      e->on_pre_step_y(e, level);
    } else {
      entity_on_pre_step_y(e, level);
    }
    while (e->yr > 1) {
      e->yr -= 1;
      e->cy += 1;
    }
    while (e->yr < 0) {
      e->yr += 1;
      e->cy -= 1;
    }
    steps -= 1;
  }
  e->dy *= powf(e->frict, e->time_mul);
  e->bdy *= powf(e->bump_frict, e->time_mul);
  if (fabsf(e->dy) <= 0.0005f * e->time_mul) {
    e->dy = 0;
  }
  if (fabsf(e->bdy) <= 0.0005f * e->time_mul) {
    e->bdy = 0;
  }
  entity_update_animation(e, dt);
  if (e->on_update != NULL) {
    e->on_update(e, dt);
  }

  cooldown_system_update(&e->pools, dt);
}

void entity_post_update(entity_t *e, float dt) {
  if (e->sprite == NULL) {
    return;
  }

  e->sprite_x = (e->cx + e->xr) * GRID;
  e->sprite_y = (e->cy + e->yr) * GRID;
  e->sprite_scale_x = e->dir * e->sprite_scale_set_x;
  e->sprite_scale_y = e->sprite_scale_set_y;
}

void entity_load_image(entity_t *e, const char *filename, uint32_t width, uint32_t height) {
  sg_image img = cache_load_image(filename);
  binocle_material *mat = binocle_material_new();
  mat->albedo_texture = img;
  mat->shader = game.gfx.default_shader;
  e->sprite = binocle_sprite_from_material(mat);
  sg_image_desc img_info = sg_query_image_desc(img);
  size_t num_frames = (img_info.width / width) * (img_info.height / height);
  //arrsetlen(g->frames, num_frames);
  for (size_t y = 0; y < img_info.height / height; y++) {
    for (size_t x = 0; x < img_info.width / width; x++) {
      binocle_subtexture frame = binocle_subtexture_with_texture(&img, x * width, y * height, width, height);
      SDL_memcpy(&e->sprite->subtexture, &frame, sizeof(binocle_subtexture));
        arrput(e->frames, frame);
    }
  }
  e->sprite->origin.x = width * e->pivot_x;
  e->sprite->origin.y = height * e->pivot_y;
}

void entity_set_pos_grid(entity_t *e, int32_t x, int32_t y) {
  e->cx = floor(x);
  e->cy = floor(y);
  e->xr = 0.5f;
  e->yr = 0;
}

void entity_set_pos_pixel(entity_t *e, int32_t x, int32_t y) {
  e->cx = x / GRID;
  e->cy = y / GRID;
  e->xr = (float) (x - e->cx * GRID) / (float) GRID;
  e->yr = (float) (y - e->cy * GRID) / (float) GRID;
}

void entity_bump(entity_t *e, float x, float y) {
  e->bdx += x;
  e->bdy += y;
}

void entity_set_speed(entity_t *e, float x, float y) {
  e->speed_x = x;
  e->speed_y = y;
}

void entity_cancel_velocities(entity_t *e) {
  e->dx = 0;
  e->dy = 0;
  e->bdx = 0;
  e->bdy = 0;
}

bool entity_on_ground(entity_t *e) {
  return level_has_wall_collision(&game.level, e->cx, e->cy - 1) && e->yr == 0 && e->dy <= 0;
}

void entity_on_touch_wall(entity_t *e, int32_t direction) {
}

void entity_on_land(entity_t *e) {
}

void entity_on_pre_step_x(entity_t *e, level_t *level) {
  // Right collisions
  if (e->has_collisions && e->xr > 0.8f && level_has_wall_collision(level, e->cx + 1, e->cy)) {
    if (e->on_touch_wall != NULL) {
      e->on_touch_wall(e, 1);
    } else {
      entity_on_touch_wall(e, 1);
    }
    e->xr = 0.8f;
  }

  // Left collisions
  if (e->has_collisions && e->xr < 0.2f && level_has_wall_collision(level, e->cx - 1, e->cy)) { ;
    if (e->on_touch_wall != NULL) {
      e->on_touch_wall(e, -1);
    } else {
      entity_on_touch_wall(e, -1);
    }
    e->xr = 0.2f;
  }
}

void entity_on_pre_step_y(entity_t *e, level_t *level) {
  // Down collisions
  if (e->has_collisions && e->yr < 0.0f && level_has_wall_collision(level, e->cx, e->cy - 1)) {
    e->dy = 0;
    e->yr = 0;
    e->bdx *= 0.5f;
    e->bdx = 0;
    if (e->on_land != NULL) {
      e->on_land(e);
    } else {
      entity_on_land(e);
    }
  }

  // Up collisions
  if (e->has_collisions && e->yr > 0.5f && level_has_wall_collision(level, e->cx, e->cy + 1)) {
    e->yr = 0.5f;
  }
}

float entity_get_attach_x(entity_t *e) {
  return (e->cx + e->xr) * GRID;
}

float entity_get_attach_y(entity_t *e) {
  return (e->cy + e->yr) * GRID;
}

float entity_get_left(entity_t *e) {
  return entity_get_attach_x(e) + (0.0f - e->pivot_x) * e->wid;
}

float entity_get_right(entity_t *e) {
  return entity_get_attach_x(e) + (1.0f - e->pivot_x) * e->wid;
}

float entity_get_top(entity_t *e) {
  return entity_get_attach_y(e) + (1.0f - e->pivot_y) * e->hei;
}

float entity_get_bottom(entity_t *e) {
  return entity_get_attach_y(e) + (0.0f - e->pivot_y) * e->hei;
}

float entity_get_center_x(entity_t *e) {
  return entity_get_attach_x(e) + (0.5f - e->pivot_x) * e->wid;
}

float entity_get_center_y(entity_t *e) {
  return entity_get_attach_y(e) + (0.5f - e->pivot_y) * e->hei;
}

bool entity_is_inside(entity_t *e, float px, float py) {
  return (px >= entity_get_left(e)
          && px <= entity_get_right(e)
          && py >= entity_get_bottom(e)
          && py <= entity_get_top(e));
}

/*
void entity_draw_debug(physics_t *physics, graphics_t *graphics, collider_t *collider) {
  if (game.debug_enabled) {
    kmAABB2 viewport = binocle_camera_get_viewport(game.gfx.camera);
    char s[1024];
    sprintf(s, "(%d,%d) (%.0f, %.0f)", physics->cx, physics->cy, entity_get_center_x(physics, graphics, collider), entity_get_center_y(physics, graphics, collider));
    binocle_ttfont_draw_string(&game.gfx.default_font, s, &game.gfx.gd, entity_get_center_x(physics, graphics, collider), entity_get_top(physics, graphics, collider), viewport, binocle_color_white(), &game.gfx.camera, LAYER_TEXT);
    kmAABB2 rect;
    kmVec2 center;
    center.x = entity_get_center_x(physics, graphics, collider);
    center.y = entity_get_center_y(physics, graphics, collider);
    kmAABB2Initialize(&rect, &center, collider->wid, collider->hei, 0);
    binocle_gd_draw_rect(&game.gfx.gd, rect, binocle_color_green_translucent(), viewport, &game.gfx.camera, NULL, LAYER_TEXT + 1);
  }
}
*/

bool entity_is_alive(entity_t *e) {
  if (e->on_is_alive != NULL) {
    return e->on_is_alive(e);
  } else {
    return !e->destroyed;
  }
}

//void entity_add_animation(graphics_t *graphics, ANIMATION_ID id, int frames[], int frames_count, float period, bool loop) {
//  float delay = 1.0f / fabsf(period);
//  binocle_sprite_add_animation_with_frames(graphics->sprite, id, loop, delay, frames, frames_count);
//}

void entity_add_animation(entity_t *e, ANIMATION_ID id, int frames[], int frames_count, float period, bool loop, void (*on_finish)(struct entity_t *e)) {
  animation_frame_t af = {
    .frames = NULL,
    .frames_count = frames_count,
    .period = 1.0f / fabsf(period),
    .loop = loop,
    .on_finish = on_finish,
  };
  for (int i = 0; i < frames_count; i++) {
      arrput(af.frames, frames[i]);
  }
    arrput(e->animations, af);
}

void entity_play_animation(entity_t *e, ANIMATION_ID id, bool force) {
  e->animation = &e->animations[id];
  if (force) {
    e->animation_timer = e->animation->period;
    e->animation_frame = 0;
    e->frame = e->animation->frames[0];
  }
}

void entity_stop_animation(entity_t *e) {
  e->animation = NULL;
}

void entity_update_animation(entity_t *e, float dt) {
  if (e->animation == NULL) {
    return;
  }

  e->animation_timer -= dt;

  if (e->animation_timer <= 0) {
    e->animation_frame += 1;
    animation_frame_t *anim = e->animation;
    if (e->animation_frame >= anim->frames_count) {
      if (anim->loop) {
        e->animation_frame = 0;
      } else {
        entity_stop_animation(e);

        if (anim->on_finish != NULL) {
          anim->on_finish(e);
        }
        return;
      }
    }

    e->animation_timer += anim->period;
    e->frame = anim->frames[e->animation_frame];
    SDL_memcpy(&e->sprite->subtexture, &e->frames[e->frame], sizeof(binocle_subtexture));
  }
}

float entity_dir_to_ang(entity_t *e) {
  if (e->dir == 1) {
    return 0;
  }
  return M_PI;
}

void entity_kill(entity_t *e) {
  e->destroyed = true;
}