//
// Created by Valerio Santinelli on 05/07/21.
//

#ifndef BINOCLE_C_ADVENTURE_GAME_GUI_H
#define BINOCLE_C_ADVENTURE_GAME_GUI_H

#include "kazmath/kazmath.h"

struct lua_State;
struct binocle_input;
struct binocle_viewport_adapter;
struct binocle_window;
struct binocle_gd;
struct sg_shader;
struct ImGuiContext;
struct gui_t;

typedef struct gui_handle_t {
  int id;
} gui_handle_t;

void gui_resources_setup();
gui_handle_t gui_resources_create_gui(const char *name);
struct gui_t *gui_resources_get_gui(const char *name);
struct gui_t *gui_resources_get_gui_with_handle(gui_handle_t handle);
void gui_pass_input_to_imgui(gui_handle_t handle, struct binocle_input *input);
void gui_draw(struct binocle_window *window, struct binocle_input *input, float dt);
void gui_imgui_to_offscreen_render(float width, float height);
void gui_init_imgui(gui_handle_t handle, float width, float height, float viewport_width, float viewport_height);
void gui_setup_imgui_to_offscreen_pipeline(binocle_gd *gd, const char *binocle_assets_dir);
void gui_render_to_screen(struct gui_t *gui, struct binocle_gd *gd, struct binocle_window *window, float design_width, float design_height, kmAABB2 viewport, kmMat4 matrix, float scale);
void gui_setup_screen_pipeline(gui_handle_t handle, struct sg_shader display_shader, bool pixel_perfect);
void gui_recreate_imgui_render_target(gui_handle_t handle, int width, int height);
void gui_set_viewport(gui_handle_t handle, int width, int height);
void gui_set_viewport_adapter(gui_handle_t handle, struct binocle_viewport_adapter *viewport_adapter);
void gui_set_apply_scissor(gui_handle_t handle, bool value);
int l_gui_wrap_new_frame(struct lua_State *L);
int l_gui_wrap_render_frame(struct lua_State *L);
int l_gui_wrap_render_to_screen(struct lua_State *L);
int l_gui_wrap_set_context(struct lua_State *L);
int l_gui_wrap_get_want_capture_mouse(struct lua_State *L);
int l_gui_wrap_get_style_frame_padding(struct lua_State *L);
#endif //BINOCLE_C_ADVENTURE_GAME_GUI_H
