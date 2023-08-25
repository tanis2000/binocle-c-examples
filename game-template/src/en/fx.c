//
// Created by Valerio Santinelli on 24/08/23.
//

#include "fx.h"
#include "entity.h"
#include "cache.h"

#define MAX_FX_FRAMES (16)

entity_t *fx_new(const char *filename, int32_t frames, float duration) {
  entity_t *e = entity_spawn();
  e->name = "fx";
  e->has_collisions = true;
  sg_image img = cache_load_image(filename);
  sg_image_desc desc = sg_query_image_desc(img);
  int32_t frame_width = desc.width / frames;
  e->hei = frame_width;
  e->wid = desc.height;
  e->depth = LAYER_FX;
  entity_load_image(e, filename, frame_width, desc.height);
  int f[MAX_FX_FRAMES];
  for (int i = 0 ; i < frames ; i++) {
    f[i] = i;
  }
  entity_add_animation(e, ANIMATION_ID_FX_MAIN, f, frames, frames/duration, false, fx_on_finish);
  entity_play_animation(e, ANIMATION_ID_FX_MAIN, true);
  return e;
}

void fx_on_finish(entity_t *e) {
  entity_free(e);
}