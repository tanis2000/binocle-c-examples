//
// Created by Valerio Santinelli on 19/07/23.
//

#ifndef GAME_TEMPLATE_CACHE_H
#define GAME_TEMPLATE_CACHE_H

#include "types.h"

void cache_system_init();

sg_image cache_load_image(const char *filename);
binocle_audio_music cache_load_music(const char *filename);

#endif //GAME_TEMPLATE_CACHE_H
