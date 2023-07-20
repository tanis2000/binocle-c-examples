//
// Created by Valerio Santinelli on 20/07/23.
//

#include "level.h"

bool level_has_wall_collision(int32_t cx, int32_t cy) {
  // TODO: implement
  return cx > DESIGN_WIDTH/GRID || cy > DESIGN_HEIGHT/GRID || cx < 0 || cy < 0;
}