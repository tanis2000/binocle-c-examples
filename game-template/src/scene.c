//
// Created by Valerio Santinelli on 29/08/23.
//

#include "scene.h"
#include "binocle_log.h"
#include "cooldown.h"
#include "stb_ds.h"

extern struct game_t game;
extern struct scene_t scenes[MAX_SCENES];

void scene_system_init() {
  memset(scenes, 0, MAX_SCENES * sizeof(scenes[0]));
  game.scenes = scenes;
}


static void scene_init(scene_t *en, scene_t *parent) {
  en->in_use = true;
  en->name = "scene";
  en->parent = parent;

  cooldown_system_init(&en->pools, 16);
}

scene_t *scene_new(scene_t *parent) {
  scene_t *e = NULL;
  for (int i = 0; i < game.num_scenes; i++) {
    e = &game.scenes[i];
    if (e->in_use) {
      continue;
    }

    scene_init(e, parent);
    return e;
  }

  if (game.num_scenes == MAX_SCENES) {
    for (int i = 0; i < MAX_SCENES; i++) {
      binocle_log_info("%4i: %s", i, game.scenes[i].name);
    }
    binocle_log_error("scene_new: no free scenes");
    return NULL;
  }

  e = &game.scenes[game.num_scenes];
  game.num_scenes++;
  scene_init(e, parent);
  return e;
}

void scene_free(scene_t *e) {
  cooldown_system_shutdown(&e->pools);
  memset(e, 0, sizeof(*e));
  e->name = "freed";
}

void scene_remove_child(scene_t *s, scene_t *child) {
  for (int i = 0 ; i < s->num_children ; i++) {
    if (s->children[i] == child) {
      s->children[i] = NULL;
    }
  }
  child->parent = NULL;
}

void scene_add_child(scene_t *s, scene_t *child) {
  binocle_log_info("adding children scene %s to %s", child->name, s->name);
  if (child->parent != NULL) {
    scene_remove_child(child->parent, child);
  }
  child->parent = s;
  for (int i = 0 ; i < s->num_children ; i++) {
    if (s->children[i] == NULL) {
      s->children[i] = child;
    }
  }
}

bool scene_can_run(scene_t *s) {
  return !s->paused && !s->destroyed;
}

void scene_pause(scene_t *s) {
  s->paused = true;
}

void scene_resume(scene_t *s) {
  s->paused = false;
}

void scene_pre_update(scene_t *s, float dt) {
  s->elapsed_time += dt;
  if (scene_can_run(s) && !s->destroyed) {
    cooldown_system_update(&s->pools, dt);
    if (s->on_pre_update != NULL) {
      s->on_pre_update(s, dt);
    }
    for (int i = 0 ; i < s->num_children ; i++) {
      scene_t *child = s->children[i];
      if (child != NULL) {
        scene_pre_update(child, dt);
      }
    }
  }
}

void scene_update(scene_t *s, float dt) {
  if (scene_can_run(s) && !s->destroyed) {
    if (s->on_update != NULL) {
      s->on_update(s, dt);
    }
    for (int i = 0 ; i < s->num_children ; i++) {
      scene_t *child = s->children[i];
      if (child != NULL) {
        scene_update(child, dt);
      }
    }
  }
}

void scene_post_update(scene_t *s, float dt) {
  if (scene_can_run(s) && !s->destroyed) {
    if (s->on_post_update != NULL) {
      s->on_post_update(s, dt);
    }
    for (int i = 0; i < s->num_children; i++) {
      scene_t *child = s->children[i];
      if (child != NULL) {
        scene_post_update(child, dt);
      }
    }
  }
}

void scene_destroy(scene_t *s) {
  s->destroyed = true;
}