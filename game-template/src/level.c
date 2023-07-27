//
// Created by Valerio Santinelli on 20/07/23.
//

#include "level.h"
#include "cache.h"
#include "backend/binocle_material.h"

extern struct game_t game;

bool level_has_wall_collision(int32_t cx, int32_t cy) {
  // TODO: implement
  return cx > DESIGN_WIDTH/GRID || cy > DESIGN_HEIGHT/GRID || cx < 0 || cy < 0;
}

int level_coord_id(level_t *level, int cx, int cy) {
  return cx * cy * level->map->width;
}

bool level_is_valid(level_t *level, int cx, int cy) {
  return cx >= 0 && cx < level->map->width && cy >= 0 && cy <= level->map->height;
}

void level_set_collision(level_t *level, int x, int y, bool v) {
  if (level_is_valid(level, x, y)) {
    level->coll_map[level_coord_id(level, x, y)] = v;
  }
}

void level_load_tilemap(level_t *level, const char *filename) {
  char *json = NULL;
  size_t json_length = 0;
  if (!binocle_sdl_load_text_file(filename, &json, &json_length)) {
    return;
  }

  level->map = cute_tiled_load_map_from_memory(json, json_length, 0);
  level->coll_map = malloc(level->map->width * level->map->height * level->map->width * sizeof(bool));

  cute_tiled_layer_t* layer = level->map->layers;;
  while (layer && layer->data_count > 0) {
    int* data = layer->data;
    int data_count = layer->data_count;

    // Setup collisions
    if (strcmp(layer->name.ptr, "collisions") == 0) {
      for (int i = 0 ; i < data_count ; i++) {
        int cy = layer->height - 1 - ((i) / layer->width);
        int cx = (i) % layer->width;
        if (data[i] != 0) {
          level_set_collision(level, cx, cy, true);
        }
      }
    }

    // TODO: setup marks for platform ends
    layer = layer->next;
  }

  cute_tiled_tileset_t *tileset = level->map->tilesets;
  while(tileset) {
    const char *image_filename = tileset->image.ptr;
    char image_path[1024];
    sprintf(image_path, "maps/%s", image_filename);
    sg_image img = cache_load_image(image_path);
    binocle_material *mat = binocle_material_new();
    mat->albedo_texture = img;
    mat->shader = game.gfx.default_shader;
    level->sprite = binocle_sprite_from_material(mat);
    level->tiles = malloc(sizeof(tile_t) * tileset->tilecount);
    for (int i = 0 ; i < tileset->tilecount ; i++) {
      level->tiles[i] = (tile_t){
        .gid = i,
        .sprite = binocle_sprite_from_material(mat),
      };
      int cy = i / GRID;
      int cx = i % GRID;
      binocle_subtexture sub = binocle_subtexture_with_texture(&img, 16 * cx, (tileset->imageheight - 16) - (16 * cy), level->map->tilewidth, level->map->tileheight);
      //level->tiles[i].sprite->subtexture = sub;
      SDL_memcpy(&level->tiles[i].sprite->subtexture, &sub, sizeof(binocle_subtexture));
      level->tiles[i].sprite->origin = (kmVec2){.x = 0, .y = 0};
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

void level_render(ecs_iter_t *it) {
  level_t *level = ecs_field(it, level_t, 1);
  for (int i = 0 ; i < it->count; i++) {
    if (level->sprite != NULL) {
      cute_tiled_layer_t *layer = level->map->layers;
      while(layer) {
        if (strcmp(layer->name.ptr, "collisions") == 0 || strcmp(layer->name.ptr, "objects") == 0 || strcmp(layer->name.ptr, "fg") == 0) {
          int *data = layer->data;
          int data_count = layer->data_count;
          for (int j = 0 ; j < data_count ; j++) {
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
              binocle_sprite_draw_with_sprite_batch(&game.gfx.sprite_batch, level->tiles[value-1].sprite, &game.gfx.gd, cx * GRID, (layer->height-1) * GRID - cy * GRID, &viewport, 0, &scale, &game.gfx.camera, depth);
            }
          }
        }
        layer = layer->next;
      }
    }
  }
}