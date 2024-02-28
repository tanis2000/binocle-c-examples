//
// Created by Valerio Santinelli on 28/08/23.
//

#include "debug_gui.h"
#include "binocle_window.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

extern struct game_t game;

void debug_gui_entity(profile_component_t *e) {
  igText("Name: %s", e->name);
}

void debug_gui_draw(float dt) {
  if (!game.debug) {
    return;
  }

  struct gui_t *gui = gui_resources_get_gui("debug");
  gui_set_context(gui);
  int w = 0;
  int h = 0;
  int display_w = 0;
  int display_h = 0;
  SDL_GetWindowSize(game.gfx.window->window, &w, &h);
  SDL_GL_GetDrawableSize(game.gfx.window->window, &display_w, &display_h);
  gui_wrap_new_frame(game.gfx.window, dt, w, h, display_w, display_h);
  static bool info_open;
  if (igBegin("Info", &info_open, ImGuiWindowFlags_None)) {
    igText("Memory:   unknown");
  }
  igEnd();

  static bool entities_open;
  if (igBegin("Entities", &entities_open, ImGuiWindowFlags_None)) {
    ecs_query_t *q = ecs_query(game.ecs, {
      .filter.terms = {
        { .id = ecs_id(profile_component_t) }
      }
    });
    ecs_iter_t it = ecs_query_iter(game.ecs, q);
    while (ecs_query_next(&it)) {
      profile_component_t *p = ecs_field(&it, profile_component_t, 1);
      if (igTreeNode_Str(p->name)) {
        debug_gui_entity(p);
        igTreePop();
      }
    }
  }
  igEnd();

  gui_wrap_render_frame(gui);
}