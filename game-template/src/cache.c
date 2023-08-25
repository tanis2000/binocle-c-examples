//
// Created by Valerio Santinelli on 19/07/23.
//

#include "cache.h"
#include "binocle_sdl.h"
#include "binocle_image.h"

extern struct game_t game;

void cache_system_init() {
  for (size_t i = 0 ; i < MAX_CACHED_IMAGES ; i++) {
    game.cache.images[i] = (cached_image_t){ 0 };
  }
  for (size_t i = 0 ; i < MAX_CACHED_MUSIC ; i++) {
    game.cache.music[i] = (cached_music_t){ 0 };
  }
  for (size_t i = 0 ; i < MAX_CACHED_SOUNDS ; i++) {
    game.cache.sounds[i] = (cached_sound_t){ 0 };
  }
}

sg_image cache_load_image(const char *filename) {
  for (size_t i = 0 ; i < game.cache.images_num ; i++) {
    cached_image_t *img = &game.cache.images[i];
    if (img->filename != NULL && SDL_strcmp(img->filename, filename) == 0) {
      return img->img;
    }
  }
  assert(game.cache.images_num < MAX_CACHED_IMAGES);
  sg_image img = binocle_image_load(filename);
  game.cache.images[game.cache.images_num].img = img;
  game.cache.images[game.cache.images_num].filename = SDL_strdup(filename);
  game.cache.images_num++;
  return img;
}

binocle_audio_music cache_load_music(const char *filename) {
  for (size_t i = 0 ; i < game.cache.music_num ; i++) {
    cached_music_t *music = &game.cache.music[i];
    if (music->filename != NULL && SDL_strcmp(music->filename, filename) == 0) {
      return music->music;
    }
  }
  assert(game.cache.music_num < MAX_CACHED_MUSIC);
  binocle_audio_load_desc desc = {
    .filename = filename,
    .fs = BINOCLE_FS_PHYSFS,
  };
  binocle_audio_music music = binocle_audio_load_music_stream_with_desc(&game.audio, &desc);
  game.cache.music[game.cache.music_num].music = music;
  game.cache.music[game.cache.music_num].filename = SDL_strdup(filename);
  game.cache.music_num++;
  return music;
}