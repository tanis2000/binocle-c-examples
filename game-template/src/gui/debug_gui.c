//
// Created by Valerio Santinelli on 28/08/23.
//

#include "debug_gui.h"
#include "binocle_window.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

extern struct game_t game;

void debug_gui_entity(entity_t *e) {
  igText("Name: %s", e->name);
}

void debug_gui_scene(scene_t *e) {
  igText("Name: %s", e->name);
}

void debug_gui_scenes() {
  static bool scenes_open;
  if (igBegin("Scenes", &scenes_open, ImGuiWindowFlags_None)) {
    for (int i = 0 ; i < game.num_scenes ; i++) {
      scene_t *e = &game.scenes[i];
      if (igTreeNode_Str(e->name)) {
        debug_gui_scene(e);
        igTreePop();
      }
    }
  }
  igEnd();
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
    igText("Entities: %d", game.num_entities);
  }
  igEnd();

  static bool entities_open;
  if (igBegin("Entities", &entities_open, ImGuiWindowFlags_None)) {
    for (int i = 0 ; i < game.num_entities ; i++) {
      entity_t *e = &game.entities[i];
      if (igTreeNode_Str(e->name)) {
        debug_gui_entity(e);
        igTreePop();
      }
    }
  }
  igEnd();

  debug_gui_scenes();

  gui_wrap_render_frame(gui);
}