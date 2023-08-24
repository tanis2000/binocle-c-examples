//
// Created by Valerio Santinelli on 20/07/23.
//

#include "level.h"
#include "cache.h"
#include "backend/binocle_material.h"
#include "stb_ds.h"

extern struct game_t game;

int level_coord_id(level_t *level, int cx, int cy) {
  return cx + cy * level->map->width;
}

bool level_is_valid(level_t *level, int cx, int cy) {
  return cx >= 0 && cx < level->map->width && cy >= 0 && cy <= level->map->height;
}

void level_set_collision(level_t *level, int x, int y, bool v) {
  if (level_is_valid(level, x, y)) {
    level->coll_map[level_coord_id(level, x, y)] = v;
  }
}

bool level_has_collision(level_t *level, int32_t cx, int32_t cy) {
  if (!level_is_valid(level, cx, cy)) {
    return true;
  }

  bool v = level->coll_map[level_coord_id(level, cx, cy)];
  if (v) {
    return true;
  }
  return false;
}

bool level_has_wall_collision(level_t *level, int32_t cx, int32_t cy) {
  return level_has_collision(level, cx, cy);
}

void level_set_mark(level_t *level, int32_t cx, int32_t cy, LEVEL_MARK mark) {
  if (level_is_valid(level, cx, cy)) {
    level->marks_map[level_coord_id(level, cx, cy)] = mark;
  }
}

bool level_has_mark(level_t *level, int32_t cx, int32_t cy, LEVEL_MARK mark) {
  if (!level_is_valid(level, cx, cy)) {
    return false;
  }
  return (level->marks_map[level_coord_id(level, cx, cy)] == mark);
}

int level_get_c_wid(level_t *level) {
  return level->map->width;
}

int level_get_c_hei(level_t *level) {
  return level->map->height;
}

int level_get_px_wid(level_t *level) {
  return level_get_c_wid(level) * GRID;
}

int level_get_px_hei(level_t *level) {
  return level_get_c_hei(level) * GRID;
}

spawner_t *level_get_hero_spawner(level_t *level) {
  return &level->hero_spawners[0];
}

void level_load_tilemap(level_t *level, const char *filename) {
  char *json = NULL;
  size_t json_length = 0;
  if (!binocle_sdl_load_text_file(filename, &json, &json_length)) {
    return;
  }

  level->map = cute_tiled_load_map_from_memory(json, json_length, 0);
  level->coll_map = malloc(level->map->width * level->map->height * sizeof(bool));
  memset(level->coll_map, 0, level->map->width * level->map->height * sizeof(bool));
  level->marks_map = malloc(level->map->width * level->map->height * sizeof(LEVEL_MARK));
  memset(level->marks_map, 0, level->map->width * level->map->height * sizeof(LEVEL_MARK));

  cute_tiled_layer_t *layer = level->map->layers;;
  while (layer) {
    int *data = layer->data;
    int data_count = layer->data_count;

    if (layer->data_count > 0) {
      // Setup collisions
      if (strcmp(layer->name.ptr, "collisions") == 0) {
        for (int i = 0; i < data_count; i++) {
          int cy = layer->height - 1 - (i / layer->width);
          int cx = i % layer->width;
          if (data[i] != 0) {
            level_set_collision(level, cx, cy, true);
          } else {
            level_set_collision(level, cx, cy, false);
          }
        }
      }

      // Setup marks
      for (int cy = 0; cy < level->map->height; cy++) {
        for (int cx = 0; cx < level->map->width; cx++) {
          if (!level_has_collision(level, cx, cy) && level_has_collision(level, cx, cy - 1)) {
            if (level_has_collision(level, cx + 1, cy) || !level_has_collision(level, cx + 1, cy - 1)) {
              level_set_mark(level, cx, cy, LEVEL_MARK_PLATFORM_END_RIGHT);
            }
            if (level_has_collision(level, cx - 1, cy) || !level_has_collision(level, cx - 1, cy - 1)) {
              level_set_mark(level, cx, cy, LEVEL_MARK_PLATFORM_END_LEFT);
            }
          }
        }
      }
    }

    if (strcmp(layer->name.ptr, "spawners") == 0) {
      cute_tiled_object_t *object = layer->objects;
      while (object) {
        if (strcmp(object->name.ptr, "hero") == 0) {
          spawner_t spawner = (spawner_t) {
            .cx = object->x / GRID,
            .cy = level->map->width - (object->y / GRID),
          };
            arrput(level->hero_spawners, spawner);
        }
        object = object->next;
      }
    }
    layer = layer->next;
  }

  cute_tiled_tileset_t *tileset = level->map->tilesets;
  while (tileset) {
    const char *image_filename = tileset->image.ptr;
    char image_path[1024];
    sprintf(image_path, "maps/%s", image_filename);
    sg_image img = cache_load_image(image_path);
    binocle_material *mat = binocle_material_new();
    mat->albedo_texture = img;
    mat->shader = game.gfx.default_shader;
    level->sprite = binocle_sprite_from_material(mat);
    level->tiles = malloc(sizeof(tile_t) * tileset->tilecount);
    for (int i = 0; i < tileset->tilecount; i++) {
      level->tiles[i] = (tile_t) {
        .gid = i,
        .sprite = binocle_sprite_from_material(mat),
      };
      int cy = i / GRID;
      int cx = i % GRID;
      binocle_subtexture sub = binocle_subtexture_with_texture(&img, 16 * cx, (tileset->imageheight - 16) - (16 * cy),
                                                               level->map->tilewidth, level->map->tileheight);
      //level->tiles[i].sprite->subtexture = sub;
      SDL_memcpy(&level->tiles[i].sprite->subtexture, &sub, sizeof(binocle_subtexture));
      level->tiles[i].sprite->origin = (kmVec2) {.x = 0, .y = 0};
    }
    tileset = tileset->next;
  }
}

void level_destroy_tilemap(level_t *level) {
  if (level->map == NULL) {
    return;
  }
  cute_tiled_free_map(level->map);
  free(level->coll_map);
  free(level->tiles);
}

void level_render(level_t *level) {
  if (level->sprite != NULL) {
    cute_tiled_layer_t *layer = level->map->layers;
    while (layer) {
      if (strcmp(layer->name.ptr, "collisions") == 0 || strcmp(layer->name.ptr, "objects") == 0 ||
          strcmp(layer->name.ptr, "fg") == 0) {
        int *data = layer->data;
        int data_count = layer->data_count;
        for (int j = 0; j < data_count; j++) {
          int value = data[j];
          int cy = j / layer->width;
          int cx = j % layer->width;
          if (value != 0) {
            float depth = LAYER_BG;
            if (strcmp(layer->name.ptr, "fg") == 0) {
              depth = LAYER_FG;
            }
            kmVec2 scale;
            scale.x = 1;
            scale.y = 1;
            kmAABB2 viewport = binocle_camera_get_viewport(game.gfx.camera);
            binocle_sprite_draw_with_sprite_batch(&game.gfx.sprite_batch, level->tiles[value - 1].sprite, &game.gfx.gd,
                                                  cx * GRID, (layer->height - 1) * GRID - cy * GRID, &viewport, 0,
                                                  &scale, &game.gfx.camera, depth);
          }
        }
      }
      layer = layer->next;
    }
  }
}