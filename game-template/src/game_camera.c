//
// Created by Valerio Santinelli on 27/07/23.
//

#include "game_camera.h"
#include "l_point.h"
#include "entity.h"
#include "level.h"

extern struct game_t game;

game_camera_t game_camera_new() {
  game_camera_t res = (game_camera_t){
    .raw_focus = l_point_new(),
    .clamped_focus = l_point_new(),
    .clamp_to_level_bounds = true,
    .dx = 0,
    .dy = 0,
    .target = 0,
    .target_off_x = 0,
    .target_off_y = 20,
    .dead_zone_pct_x = 0.04f,
    .dead_zone_pct_y = 0.10f,
    .base_frict = 0.89,
    .bump_off_x = 0,
    .bump_off_y = 0,
    .zoom = 1.0f,
    .tracking_speed = 1.0f,
    .brake_dist_near_bounds = 0.1f,
    .shake_power = 1.0f,
  };
  l_point_set_level_case(&res.raw_focus, 0, 0, 0.5f, 0.5f);
  l_point_set_level_case(&res.clamped_focus, 0, 0, 0.5f, 0.5f);
  return res;
}

int32_t game_camera_get_px_wid(game_camera_t *gc) {
  return DESIGN_WIDTH / gc->zoom;
}

int32_t game_camera_get_px_hei(game_camera_t *gc) {
  return DESIGN_HEIGHT / gc->zoom;
}

int32_t game_camera_get_left(game_camera_t *gc) {
  return l_point_get_level_x(&gc->clamped_focus) - game_camera_get_px_wid(gc) * 0.5f;
}

int32_t game_camera_get_right(game_camera_t *gc) {
  return l_point_get_level_x(&gc->clamped_focus) + game_camera_get_px_wid(gc) * 0.5f;
}

int32_t game_camera_get_top(game_camera_t *gc) {
  return l_point_get_level_y(&gc->clamped_focus) + game_camera_get_px_hei(gc) * 0.5f;
}

int32_t game_camera_get_bottom(game_camera_t *gc) {
  return l_point_get_level_y(&gc->clamped_focus) - game_camera_get_px_hei(gc) * 0.5f;
}

int32_t game_camera_get_center_x(game_camera_t *gc) {
  return (game_camera_get_left(gc) + game_camera_get_right(gc)) * 0.5f;
}

int32_t game_camera_get_center_y(game_camera_t *gc) {
  return (game_camera_get_bottom(gc) + game_camera_get_top(gc)) * 0.5f;
}

void game_camera_set_zoom(game_camera_t *gc, float zoom) {
  gc->zoom = kmClamp(zoom, 1, 10);
}

bool game_camera_is_on_screen(game_camera_t *gc, int32_t level_x, int32_t level_y) {
  return level_x >= game_camera_get_left(gc)
  && level_x <= game_camera_get_right(gc)
  && level_y <= game_camera_get_top(gc)
  && level_y >= game_camera_get_bottom(gc);
}

bool game_camera_is_on_screen_case(game_camera_t *gc, int32_t cx, int32_t cy) {
  int32_t pad = 32;
  return cx * GRID >= game_camera_get_left(gc) - pad
  && (cx+1) * GRID <= game_camera_get_right(gc) + pad
  && cy * GRID <= game_camera_get_top(gc) + pad
  && (cy+1) * GRID >= game_camera_get_bottom(gc) - pad;
}

void game_camera_set_tracking_speed(game_camera_t *gc, float speed) {
  gc->tracking_speed = kmClamp(speed, 0.01f, 10);
}

void game_camera_center_on_target(game_camera_t *gc) {
  if (gc->target == 0) {
    return;
  }

  l_point_set_level_x(&gc->raw_focus, entity_get_center_x(gc->target) + gc->target_off_x);
  l_point_set_level_y(&gc->raw_focus, entity_get_center_y(gc->target) + gc->target_off_y);
}

void game_camera_track_entity(game_camera_t *gc, ecs_entity_t e, bool immediate, float speed) {
  gc->target = e;
  game_camera_set_tracking_speed(gc, speed);
  if (immediate || l_point_get_level_x(&gc->raw_focus) == 0 && l_point_get_level_y(&gc->raw_focus) == 0) {
    game_camera_center_on_target(gc);
  }
}

void game_camera_apply(game_camera_t *gc) {
  int32_t cam_x = l_point_get_level_x(&gc->clamped_focus) - game_camera_get_px_wid(gc) * 0.5f;
  int32_t cam_y = l_point_get_level_y(&gc->clamped_focus) - game_camera_get_px_hei(gc) * 0.5f;

//  if (game_camera_has_cooldown(gc, "shaking")) {
//    cam_x += cosf(gc->elapsed_time * 1.1f) * 2.5f * gc->shake_power * game_camera_get_cooldown_ratio(gc, "shaking");
//    cam_y += sinf(0.3f + gc->elapsed_time * 1.7f) * 2.5f * gc->shake_power * game_camera_get_cooldown_ratio(gc, "shaking");
//  }

  binocle_camera_set_position(&game.gfx.camera, cam_x, cam_y);
}

/*
 function GameCamera:apply()
    local cam_x = math.floor(self.clamped_focus:get_level_x() - self:get_px_wid() * 0.5)
    local cam_y = math.floor(self.clamped_focus:get_level_y() - self:get_px_hei() * 0.5)

    if self.cd:has("shaking") then
        cam_x = cam_x + math.cos(self.elapsed_time*1.1)*2.5*self.shake_power * self.cd:get_ratio("shaking")
        cam_y = cam_y + math.sin(0.3+self.elapsed_time*1.7)*2.5*self.shake_power * self.cd:get_ratio("shaking")
    end

    camera.set_position(cam, cam_x, cam_y)
end

 */
void update_game_camera(ecs_iter_t *it) {
  game_camera_t *gcs = ecs_field(it, game_camera_t, 1);
  level_t *level = ecs_get(game.ecs, game.level, level_t);
  for (int i = 0 ; i < it->count; i++) {
    game_camera_t *gc = &gcs[i];
    // target tracking
    if (gc->target != 0) {
      ecs_get(game.ecs, gc->target, physics_t);
      float spd_x = 0.015f * gc->tracking_speed * gc->zoom;
      float spd_y = 0.023f * gc->tracking_speed * gc->zoom;
      float tx = entity_get_center_x(gc->target) + gc->target_off_x;
      float ty = entity_get_center_y(gc->target) + gc->target_off_y;
      float a = l_point_ang_to(&gc->raw_focus, NULL, NULL, tx, ty);
      float dist_x = fabsf(tx - l_point_get_level_x(&gc->raw_focus));
      if (dist_x >= gc->dead_zone_pct_x * game_camera_get_px_wid(gc)) {
        gc->dx += cosf(a) * (0.8f * dist_x - gc->dead_zone_pct_x * game_camera_get_px_wid(gc)) * spd_x;
      }
      float dist_y = fabsf(ty - l_point_get_level_y(&gc->raw_focus));
      if (dist_y >= gc->dead_zone_pct_y * game_camera_get_px_hei(gc)) {
        gc->dy += sinf(a) * (0.8f * dist_y - gc->dead_zone_pct_y * game_camera_get_px_hei(gc)) * spd_y;
      }
    }

    // friction
    float frict_x = gc->base_frict - gc->tracking_speed * gc->zoom * 0.027f * gc->base_frict;
    float frict_y = frict_x;
    if (gc->clamp_to_level_bounds) {
      float brake_dist = gc->brake_dist_near_bounds * game_camera_get_px_wid(gc);
      if (gc->dx <= 0) {
        float brake_ratio = 1.0f - kmClamp((l_point_get_level_x(&gc->raw_focus) - game_camera_get_px_wid(gc) * 0.5f) / brake_dist, 0, 1);
        frict_x = frict_x * 1 - 0.9f * brake_ratio;
      } else if (gc->dx > 0) {
        float brake_ratio = 1.0f - kmClamp(((level_get_px_wid(level) - game_camera_get_px_wid(gc) * 0.5f) - l_point_get_level_x(&gc->raw_focus)) / brake_dist, 0, 1);
        frict_x = frict_x * 1 - 0.9f * brake_ratio;
      }

      brake_dist = gc->brake_dist_near_bounds * game_camera_get_px_hei(gc);
      if (gc->dy > 0) {
        float brake_ratio = 1.0f - kmClamp((l_point_get_level_y(&gc->raw_focus) - game_camera_get_px_hei(gc) * 0.5f) / brake_dist, 0, 1);
        frict_y = frict_y * 1 - 0.9f * brake_ratio;
      } else if (gc->dy < 0) {
        float brake_ratio = 1.0f - kmClamp(((level_get_px_hei(level) - game_camera_get_px_hei(gc) * 0.5f) - l_point_get_level_y(&gc->raw_focus)) / brake_dist, 0, 1);
        frict_y = frict_y * 1 - 0.9f * brake_ratio;
      }
    }

    l_point_set_level_x(&gc->raw_focus, l_point_get_level_x(&gc->raw_focus) + gc->dx);
    gc->dx *= powf(frict_x, 1);
    if (fabsf(gc->dx) < 0.01f) {
      gc->dx = 0;
    }
    l_point_set_level_y(&gc->raw_focus, l_point_get_level_y(&gc->raw_focus) + gc->dy);
    gc->dy *= powf(frict_y, 1);
    if (fabsf(gc->dy) < 0.01f) {
      gc->dy = 0;
    }
    if (gc->clamp_to_level_bounds) {
      if (level_get_px_wid(level) < game_camera_get_px_wid(gc)) {
        l_point_set_level_x(&gc->clamped_focus, level_get_px_wid(level) * 0.5f); // centered small level
      } else {
        l_point_set_level_x(&gc->clamped_focus, kmClamp(l_point_get_level_x(&gc->raw_focus), game_camera_get_px_wid(gc) * 0.5f, level_get_px_wid(level) - game_camera_get_px_wid(gc) * 0.5f));
      }

      if (level_get_px_hei(level) < game_camera_get_px_hei(gc)) {
        l_point_set_level_y(&gc->clamped_focus, level_get_px_hei(level) * 0.5f); // centered small level
      } else {
        l_point_set_level_y(&gc->clamped_focus, kmClamp(l_point_get_level_y(&gc->raw_focus), game_camera_get_px_hei(gc) * 0.5f, level_get_px_hei(level) - game_camera_get_px_hei(gc) * 0.5f));
      }
    } else {
      l_point_set_level_x(&gc->clamped_focus, l_point_get_level_x(&gc->raw_focus));
      l_point_set_level_y(&gc->clamped_focus, l_point_get_level_y(&gc->raw_focus));
    }
  }
}

void post_update_game_camera(ecs_iter_t *it) {
  game_camera_t *gcs = ecs_field(it, game_camera_t, 1);
  for (int i = 0 ; i < it->count; i++) {
    game_camera_t *gc = &gcs[i];
    game_camera_apply(gc);
  }
}