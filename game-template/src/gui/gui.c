//
// Created by Valerio Santinelli on 05/07/21.
//

#include <binocle_window.h>
#include <binocle_input.h>
#include <binocle_gd.h>
#include "gui.h"
#include "binocle_sdl.h"
#include "binocle_fs.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "lua.h"
#include "binocle_window_wrap.h"
#include "binocle_gd_wrap.h"
#include "binocle_camera_wrap.h"
#include "binocle_camera.h"
#include "binocle_viewport_adapter.h"
#include "binocle_log.h"
#include "binocle_array.h"

#define MAX_LOG_ENTRIES 4096
#define max_ui_vertices (1 << 16)
#define max_ui_indices (max_ui_vertices * 3)

typedef struct imgui_vs_params_t {
  ImVec2 disp_size;
  float projmat[4][4];
} imgui_vs_params_t;

typedef struct imgui_to_offscreen_shader_fs_params_t {
  float resolution[2];
} imgui_to_offscreen_shader_fs_params_t;

ImFontAtlas *shared_font_atlas;
sg_pass_action imgui_pass_action;
sg_pipeline imgui_pip;
sg_bindings imgui_bind;



sg_pass imgui_to_offscreen_pass;
sg_pipeline imgui_to_offscreen_pip;
sg_shader imgui_to_offscreen_shader;
sg_buffer imgui_to_offscreen_vbuf;
sg_buffer imgui_to_offscreen_ibuf;
sg_bindings imgui_to_offscreen_bind;
sg_pass_action imgui_to_offscreen_action;

sg_pass_action gui_screen_pass_action;

sg_bindings gui_screen_bind;
sg_pass gui_screen_pass;
sg_buffer gui_screen_vbuf;
sg_buffer gui_screen_ibuf;

ImVec2 *ImVec2Zero;

typedef struct screen_vs_params_t {
  kmMat4 transform;
} screen_vs_params_t;

typedef struct screen_fs_params_t {
  float resolution[2];
  float scale[2];
  float viewport[2];
} screen_fs_params_t;

// Clipboard
char *clipboard_text_data;

typedef struct gui_console_item_t {
  char text[1024];
  int type;
} gui_console_item_t;

typedef struct gui_state_t {
  struct {
    bool scroll_to_bottom;
    gui_console_item_t items[MAX_LOG_ENTRIES];
    uint32_t items_size;
    char input_buf[256];
  } console;
} gui_state_t;

typedef struct gui_t {
  struct ImGuiContext *ctx;
  const char *name;
  sg_image imgui_render_target;
  sg_pass imgui_pass;
  sg_pipeline gui_screen_pip;
  float viewport_w;
  float viewport_h;
  float rt_w;
  float rt_h;
  binocle_viewport_adapter *viewport_adapter;
  bool apply_scissor;
} gui_t;

typedef struct gui_resources_t {
  gui_t *guis_array;
  gui_t *current_context_gui;
} gui_resources_t;

gui_state_t gui_state;
gui_resources_t gui_resources;

static void strtrim(char *str) {
  char *str_end = str + strlen(str);
  while (str_end > str && str_end[-1] == ' ') str_end--;
  *str_end = 0;
}

const char *get_clipboard_text(void *caller) {
  if (clipboard_text_data != NULL) {
    SDL_free(clipboard_text_data);
  }
  clipboard_text_data = SDL_GetClipboardText();
  return clipboard_text_data;
}

void set_clipboard_text(void *caller, const char * text) {
  SDL_SetClipboardText(text);
}

void gui_resources_setup() {
  gui_resources = (gui_resources_t){ 0 };
  binocle_array_set_capacity(gui_resources.guis_array, 0);
}

gui_handle_t gui_resources_create_gui(const char *name) {
  gui_handle_t handle = {.id=-1};
  gui_t gui = {0};
  gui.name = SDL_strdup(name);
  gui_t *res = binocle_array_push(gui_resources.guis_array, gui);
  for (int i = 0 ; i < binocle_array_size(gui_resources.guis_array) ; i++) {
    gui_t *g = &gui_resources.guis_array[i];
    if (g == res) {
      handle.id = i;
      return handle;
    }
  }
  return handle;
}

gui_t *gui_resources_get_gui_with_handle(gui_handle_t handle) {
  return &gui_resources.guis_array[handle.id];
}

gui_t *gui_resources_get_gui(const char *name) {
  for (int i = 0 ; i < binocle_array_size(gui_resources.guis_array) ; i++) {
    gui_t *gui = &gui_resources.guis_array[i];
    if (SDL_strcmp(gui->name, name) == 0) {
      return gui;
    }
  }
  return NULL;
}

void gui_set_context(gui_t *gui) {
  igSetCurrentContext(gui->ctx);
  gui_resources.current_context_gui = gui;
}

void gui_set_viewport(gui_handle_t handle, int width, int height) {
  gui_t *gui = gui_resources_get_gui_with_handle(handle);
  gui->viewport_w = width;
  gui->viewport_h = height;
}

void gui_set_viewport_adapter(gui_handle_t handle, binocle_viewport_adapter *viewport_adapter) {
  gui_t *gui = gui_resources_get_gui_with_handle(handle);
  gui->viewport_adapter = viewport_adapter;
}

void gui_set_apply_scissor(gui_handle_t handle, bool value) {
  gui_t *gui = gui_resources_get_gui_with_handle(handle);
  gui->apply_scissor = value;
}

void gui_recreate_imgui_render_target(gui_handle_t handle, int width, int height) {
  gui_t *gui = gui_resources_get_gui_with_handle(handle);
  gui_set_context(gui);
  gui->rt_w = width;
  gui->rt_h = height;

  if (sg_query_image_state(gui->imgui_render_target) == SG_RESOURCESTATE_VALID) {
    sg_destroy_image(gui->imgui_render_target);
  }
  sg_image_desc rt_desc = {
    .render_target = true,
    .width = width,
    .height = height,
    .min_filter = SG_FILTER_LINEAR,
    .mag_filter = SG_FILTER_LINEAR,
#ifdef BINOCLE_GL
    .pixel_format = SG_PIXELFORMAT_RGBA8,
#else
    .pixel_format = BINOCLE_PIXELFORMAT_BGRA8,
#endif
    .sample_count = 1,
  };
  gui->imgui_render_target = sg_make_image(&rt_desc);
  gui->imgui_pass = sg_make_pass(&(sg_pass_desc){
    .color_attachments[0].image = gui->imgui_render_target,
  });
}

void gui_init_imgui(gui_handle_t handle, float width, float height, float viewport_width, float viewport_height) {
  gui_t *gui = gui_resources_get_gui_with_handle(handle);
  ImVec2Zero = ImVec2_ImVec2_Float(0, 0);
  shared_font_atlas = ImFontAtlas_ImFontAtlas();
  gui->ctx = igCreateContext(shared_font_atlas);
  gui->viewport_w = viewport_width;
  gui->viewport_h = viewport_height;
  gui->rt_w = width;
  gui->rt_h = height;
  igSetCurrentContext(gui->ctx);
  ImGuiIO *io = igGetIO(); (void)io;
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

  // Setup Dear ImGui style
  ImGuiStyle *dst = ImGuiStyle_ImGuiStyle();
  igStyleColorsDark(dst);
//  igStyleColorsClassic(dst);

  io->KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
  io->KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
  io->KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
  io->KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
  io->KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
  io->KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
  io->KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
  io->KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
  io->KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
  io->KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
  io->KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
  io->KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
  io->KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
  io->KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
  io->KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
  io->KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
  io->KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
  io->KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
  io->KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
  io->KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
  io->KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;

  io->GetClipboardTextFn = get_clipboard_text;
  io->SetClipboardTextFn = set_clipboard_text;
  io->ClipboardUserData = NULL;

  io->ConfigWindowsMoveFromTitleBarOnly = true;

  // dynamic vertex- and index-buffers for imgui-generated geometry
  sg_buffer_desc vbuf_desc = {0};
  vbuf_desc.usage = SG_USAGE_STREAM;
  vbuf_desc.size = max_ui_vertices * sizeof(ImDrawVert);
  imgui_bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

  sg_buffer_desc ibuf_desc = {0};
  ibuf_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
  ibuf_desc.usage = SG_USAGE_STREAM;
  ibuf_desc.size = max_ui_indices * sizeof(ImDrawIdx);
  imgui_bind.index_buffer = sg_make_buffer(&ibuf_desc);

  // font texture for imgui's default font
  unsigned char* font_pixels;
  int font_width, font_height, bpp;
  ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &font_pixels, &font_width, &font_height, &bpp);
  sg_image_desc img_desc = {0};
  img_desc.width = font_width;
  img_desc.height = font_height;
  img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
  img_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
  img_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
  img_desc.min_filter = SG_FILTER_LINEAR;
  img_desc.mag_filter = SG_FILTER_NEAREST;
  img_desc.data.subimage[0][0] = (sg_range){font_pixels, (size_t)(font_width * font_height * 4)};
  imgui_bind.fs_images[0] = sg_make_image(&img_desc);

  // shader object for imgui rendering
  sg_shader_desc shd_desc = {0};
  shd_desc.attrs[0].name = "position";
  shd_desc.attrs[1].name = "texcoord0";
  shd_desc.attrs[2].name = "color0";
  shd_desc.vs.uniform_blocks[0].size = sizeof(imgui_vs_params_t);
  shd_desc.vs.uniform_blocks[0].uniforms[0].name = "disp_size";
  shd_desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;
  shd_desc.vs.uniform_blocks[0].uniforms[1].name = "projmtx";
  shd_desc.vs.uniform_blocks[0].uniforms[1].type = SG_UNIFORMTYPE_MAT4;
#if defined(__IPHONEOS__) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
  shd_desc.vs.source =
    "#version 300 es\n"
    "precision mediump float;\n"
    "precision mediump int;\n"
    "uniform vec2 disp_size;\n"
    "uniform mat4 projmtx;\n"
    "in vec2 position;\n"
    "in vec2 texcoord0;\n"
    "in vec4 color0;\n"
    "out vec2 uv;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "    gl_Position = vec4(((position/disp_size)-0.5)*vec2(2.0,-2.0), 0.5, 1.0);\n"
//    "    gl_Position = projmtx * vec4(position.xy,0,1);\n"
    "    uv = texcoord0;\n"
    "    color = color0;\n"
    "}\n";
#else
  shd_desc.vs.source =
    "#version 330\n"
    "uniform vec2 disp_size;\n"
    "uniform mat4 projmtx;\n"
    "layout(location=0) in vec2 position;\n"
    "layout(location=1) in vec2 texcoord0;\n"
    "layout(location=2) in vec4 color0;\n"
    "out vec2 uv;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "    gl_Position = vec4(((position/disp_size)-0.5)*vec2(2.0,-2.0), 0.5, 1.0);\n"
//    "    gl_Position = projmtx * vec4(position.xy,0,1);\n"
    "    uv = texcoord0;\n"
    "    color = color0;\n"
    "}\n";
#endif
  shd_desc.fs.images[0].name = "tex";
  shd_desc.fs.images[0].image_type = SG_IMAGETYPE_2D;
  shd_desc.fs.images[0].sampler_type = SG_SAMPLERTYPE_FLOAT;
#if defined(__IPHONEOS__) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
  shd_desc.fs.source =
    "#version 300 es\n"
    "precision mediump float;\n"
    "precision mediump int;\n"
    "uniform sampler2D tex;\n"
    "in vec2 uv;\n"
    "in vec4 color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = texture(tex, uv) * color;\n"
    "}\n";
#else
  shd_desc.fs.source =
    "#version 330\n"
    "uniform sampler2D tex;\n"
    "in vec2 uv;\n"
    "in vec4 color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = texture(tex, uv) * color;\n"
    "}\n";
#endif
  binocle_log_info("Compiling GUI shader for init");
  sg_shader shd = sg_make_shader(&shd_desc);
  binocle_log_info("Done compiling GUI shader for init");

  // pipeline object for imgui rendering
  sg_pipeline_desc pip_desc = {
    .layout = {
      .buffers[0] = {
        .stride = sizeof(ImDrawVert)
      },
      .attrs = {
        [0] = {
          .format = SG_VERTEXFORMAT_FLOAT2,
          .offset = offsetof(ImDrawVert, pos)
        },
        [1] = {
          .format = SG_VERTEXFORMAT_FLOAT2,
          .offset = offsetof(ImDrawVert, uv)
        },
        [2] = {
          .format = SG_VERTEXFORMAT_UBYTE4N,
          .offset = offsetof(ImDrawVert, col)
        }
      }
    },
    .shader = shd,
    .index_type = SG_INDEXTYPE_UINT16,
    .colors[0] = {
      .blend = {
        .enabled = true,
        .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
        .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .src_factor_alpha = SG_BLENDFACTOR_ONE,
        .dst_factor_alpha = SG_BLENDFACTOR_ONE
      },
      .write_mask = SG_COLORMASK_RGBA
    },
    .depth = {
      .pixel_format = SG_PIXELFORMAT_NONE
    }
  };
  binocle_log_info("Creating GUI pipeline");
  imgui_pip = sg_make_pipeline(&pip_desc);
  binocle_log_info("Done creating GUI pipeline");

  // initial clear color
//  imgui_pass_action.colors[0].action = BINOCLE_ACTION_CLEAR;
//  imgui_pass_action.colors[0].value = (binocle_color){ 0.0f, 0.5f, 0.7f, 1.0f };
  imgui_pass_action.colors[0].action = SG_ACTION_CLEAR;
  imgui_pass_action.colors[0].value = (sg_color){ 0.0f, 0.0f, 0.0f, 0.0f };

  // Create the render target image
  gui_recreate_imgui_render_target(handle, (int)width, (int)height);
}

void draw_imgui(ImDrawData* draw_data) {
  assert(draw_data);
  if (draw_data->CmdListsCount == 0) {
    return;
  }

  // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
  int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
  int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
  if (fb_width <= 0 || fb_height <= 0) {
    return;
  }

  // render the command list
  sg_apply_pipeline(imgui_pip);
  imgui_vs_params_t vs_params;
  ImGuiIO *io = igGetIO();
  ImTextureID tex_id = io->Fonts->TexID;
  vs_params.disp_size.x = io->DisplaySize.x;
  vs_params.disp_size.y = io->DisplaySize.y;
  sg_apply_viewport(0, 0, fb_width, fb_height, false);

  float L = draw_data->DisplayPos.x;
  float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
  float T = draw_data->DisplayPos.y;
  float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
  const float ortho_projection[4][4] =
    {
      { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
      { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
      { 0.0f,         0.0f,        -1.0f,   0.0f },
      { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
    };

  for (int i = 0 ; i < 4 ; i++) {
    for (int j = 0 ; j < 4 ; j++) {
      vs_params.projmat[i][j] = ortho_projection[i][j];
    }
  }

  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(vs_params));

  // Will project scissor/clipping rectangles into framebuffer space
  struct ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
  struct ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

  for (int cl_index = 0; cl_index < draw_data->CmdListsCount; cl_index++) {
    const ImDrawList* cl = draw_data->CmdLists[cl_index];

    // append vertices and indices to buffers, record start offsets in resource binding struct
    const uint32_t vtx_size = cl->VtxBuffer.Size * sizeof(ImDrawVert);
    const uint32_t idx_size = cl->IdxBuffer.Size * sizeof(ImDrawIdx);
    const uint32_t vb_offset = sg_append_buffer(imgui_bind.vertex_buffers[0], &(sg_range){ cl->VtxBuffer.Data, vtx_size });
    const uint32_t ib_offset = sg_append_buffer(imgui_bind.index_buffer, &(sg_range){ cl->IdxBuffer.Data, idx_size });
    /* don't render anything if the buffer is in overflow state (this is also
        checked internally in sokol_gfx, draw calls that attempt from
        overflowed buffers will be silently dropped)
    */
    if (sg_query_buffer_overflow(imgui_bind.vertex_buffers[0]) ||
        sg_query_buffer_overflow(imgui_bind.index_buffer))
    {
      continue;
    }

    imgui_bind.vertex_buffer_offsets[0] = vb_offset;
    imgui_bind.index_buffer_offset = ib_offset;
    sg_apply_bindings(&imgui_bind);

    int base_element = 0;
    for (int cmd_i = 0; cmd_i < cl->CmdBuffer.Size; cmd_i++)
    {
      const ImDrawCmd* pcmd = &cl->CmdBuffer.Data[cmd_i];
      if (pcmd->UserCallback) {
        pcmd->UserCallback(cl, pcmd);
      }
      else {
        if (tex_id != pcmd->TextureId) {
          tex_id = pcmd->TextureId;
          imgui_bind.fs_images[0].id = (uint32_t)(uintptr_t)tex_id;
          sg_apply_bindings(&imgui_bind);
        }
        const int scissor_x = (int) ((pcmd->ClipRect.x - clip_off.x) * clip_scale.x);
        const int scissor_y = (int) ((pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
        const int scissor_w = (int) ((pcmd->ClipRect.z - clip_off.x) * clip_scale.x);
        const int scissor_h = (int) ((pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
        if (scissor_x < fb_width && scissor_y < fb_height && scissor_w >= 0 && scissor_h >= 0) {
          sg_apply_scissor_rect(scissor_x, scissor_y, scissor_w, scissor_h, true);
          sg_draw(base_element, pcmd->ElemCount, 1);
        }
      }
      base_element += pcmd->ElemCount;
    }
  }
}

void gui_clear_log() {
  gui_state.console.items_size = 0;
}

void gui_add_to_log(gui_console_item_t item) {
  if (gui_state.console.items_size >= MAX_LOG_ENTRIES) {
    return;
  }
  gui_state.console.items[gui_state.console.items_size] = item;
  gui_state.console.items_size++;
}

int gui_text_edit_callback(ImGuiInputTextCallbackData *user_data) {
  return 0;
}

void gui_draw_console_window(bool *show) {
  ImGuiStyle *style = igGetStyle();

  igSetNextWindowSize((ImVec2){ 400, 200}, ImGuiCond_FirstUseEver);
  if (!igBegin("Console", show, ImGuiWindowFlags_None)) {
    igEnd();
    return;
  }

  igTextWrapped("Welcome to the debug console.");

  if (igSmallButton("Add text")) {
    gui_console_item_t item = {
      .text = "ciao",
      .type = 0,
    };
    gui_add_to_log(item);
  }
  igSameLine(0, -1.0f);

  if (igSmallButton("Clear")) {
    gui_clear_log();
  }
  igSameLine(0, -1.0f);

  bool copy_to_clipboard = igSmallButton("Copy");
  igSameLine(0, -1.0f);

  if (igSmallButton("Scroll to bottom")) {
    gui_state.console.scroll_to_bottom = true;
  }
  igSeparator();

  const float footer_height_to_reserve = style->ItemSpacing.y + igGetFrameHeightWithSpacing(); // 1 separator, 1 input text
  igBeginChild_Str("ScrollingRegion", (ImVec2){0, -footer_height_to_reserve}, false, ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText
  if (igBeginPopupContextWindow("ScrollingRegionPopup", 0)) {
    if (igSelectable_Bool("Clear", false, ImGuiSelectableFlags_None, (ImVec2){100, 50})) {
      gui_clear_log();
    }
    igEndPopup();
  }

  igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, (ImVec2){4, 1});
  if (copy_to_clipboard) {
    igLogToClipboard(0);
  }

  for (int i = 0 ; i < gui_state.console.items_size ; i++) {
    const gui_console_item_t* item = &gui_state.console.items[i];
    bool pop_color = false;
    if (item->type == 1) {
      igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.0f, 0.4f, 0.4f, 1.0f});
      pop_color = true;
    }
    igTextUnformatted(item->text, NULL);
    if (pop_color) {
      igPopStyleColor(1);
    }
  }

  if (copy_to_clipboard) {
    igLogFinish();
  }
  if (gui_state.console.scroll_to_bottom) {
    igSetScrollHereY(1.0f);
  }
  gui_state.console.scroll_to_bottom = false;
  igPopStyleVar(1);
  igEndChild();
  igSeparator();

  // Command-line
  bool reclaim_focus = false;
  if (igInputText("Input", gui_state.console.input_buf, sizeof(gui_state.console.input_buf), ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CallbackCompletion|ImGuiInputTextFlags_CallbackHistory, &gui_text_edit_callback, (void*)NULL))
  {
    char* s = gui_state.console.input_buf;
    strtrim(s);
//    if (s[0])
//      ExecCommand(s);
    strcpy(s, "");
    reclaim_focus = true;
  }

  // Auto-focus on window apparition
  igSetItemDefaultFocus();
  if (reclaim_focus)
    igSetKeyboardFocusHere(-1); // Auto focus previous widget

  igEnd();

}

/*void gui_draw(binocle_window *window, binocle_input *input, float dt) {
  // Start the Dear ImGui frame
  //ImGui_ImplOpenGL3_NewFrame();
  //ImGui_ImplSDL2_NewFrame(window);

  ImGuiIO *io = igGetIO();
  int w, h;
  int display_w, display_h;
  SDL_GetWindowSize(window->window, &w, &h);
  SDL_GL_GetDrawableSize(window->window, &display_w, &display_h);
  io->DisplaySize.x = ImVec2_ImVec2_Float((float)w, (float)h)->x;
  io->DisplaySize.y = ImVec2_ImVec2_Float((float)w, (float)h)->y;
  io->DisplayFramebufferScale.x = ImVec2_ImVec2_Float(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0)->x;
  io->DisplayFramebufferScale.y = ImVec2_ImVec2_Float(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0)->y;
  io->DeltaTime = dt;

  igNewFrame();

  static bool show_demo_window = true;
  static bool show_console_window = true;
  // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
  igShowDemoWindow(&show_demo_window);

  // Main menu
  if (igBeginMainMenuBar()) {
    if (igBeginMenu("File", true)) {
      if (igMenuItem_Bool("New", "CTRL+N", false, true)) {
      }
      if (igMenuItem_Bool("Open...", "CTRL+O", false, true)) {
      }
      if (igMenuItem_Bool("Save as...", "CTRL+A", false, true)) {
      }
      if (igMenuItem_Bool("Save", "CTRL+S", false, false)) {
      }
      if (igMenuItem_Bool("Properties...", "CTRL+P", false, true)) {
      }
      if (igMenuItem_Bool("Quit", "ALT+F4", false, true)) {
        input->quit_requested = true;
      }
      igEndMenu();
    }

    if (igBeginMenu("View", true)) {
      if (igMenuItem_Bool("Console", "CTRL+C", show_console_window, true)) {
        show_console_window = !show_console_window;
      }
      igEndMenu();
    }

    igEndMainMenuBar();
  }

  gui_draw_console_window(&show_console_window);

  // Rendering
  sg_begin_pass(imgui_pass, &imgui_pass_action);
  igRender();
  draw_imgui(igGetDrawData());
  sg_end_pass();

//  binocle_gd_set_render_target(&ui_buffer);
//  binocle_gd_apply_shader(&gd, ui_shader);

//  glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
//  glClearColor(0, 0, 0, 0);
//  glClear(GL_COLOR_BUFFER_BIT);
//
//  ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
}*/

/*void gui_setup_imgui_to_offscreen_pipeline(binocle_gd *gd, const char *binocle_assets_dir) {
  binocle_log_info("Setting up IMGUI offscreen pipeline");
  char vert[4096];
  char frag[4096];

  static const float vertices[] = {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    -1.0f, 1.0f,
    -1.0f, 1.0f,
    1.0f, -1.0f,
    1.0f, 1.0f,
  };

  uint16_t indices[] = {
    0, 1, 2, 3, 4, 5,
  };

#ifdef BINOCLE_GL
  // Quad shader
#if defined(__IPHONEOS__) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
  sprintf(vert, "%sshaders/gles/%s", binocle_assets_dir, "quad_vert.glsl");
  sprintf(frag, "%sshaders/gles/%s", binocle_assets_dir, "quad_frag.glsl");
#else
  sprintf(vert, "%sshaders/gl33/%s", binocle_assets_dir, "quad_vert.glsl");
  sprintf(frag, "%sshaders/gl33/%s", binocle_assets_dir, "quad_frag.glsl");
#endif
  char *screen_shader_vs_src;
  size_t screen_shader_vs_src_size;
  binocle_fs_load_text_file(vert, &screen_shader_vs_src, &screen_shader_vs_src_size);

  char *screen_shader_fs_src;
  size_t screen_shader_fs_src_size;
  binocle_fs_load_text_file(frag, &screen_shader_fs_src, &screen_shader_fs_src_size);
#endif

  sg_shader_desc screen_shader_desc = {
#ifdef BINOCLE_GL
    .vs.source = screen_shader_vs_src,
#else
    .vs.byte_code = screen_vs_bytecode,
    .vs.byte_code_size = sizeof(screen_vs_bytecode),
#endif
    .attrs = {
      [0].name = "position"
    },
#ifdef BINOCLE_GL
    .fs.source = screen_shader_fs_src,
#else
    .fs.byte_code = screen_fs_bytecode,
    .fs.byte_code_size = sizeof(screen_fs_bytecode),
#endif
    .fs.images[0] = { .name = "texture", .image_type = SG_IMAGETYPE_2D},
    .fs.uniform_blocks[0] = {
      .size = sizeof(struct imgui_to_offscreen_shader_fs_params_t),
      .uniforms = {
        [0] = { .name = "resolution", .type = SG_UNIFORMTYPE_FLOAT2 },
      },
    },
  };
  binocle_log_info("Compiling offscreen GUI shader");
  imgui_to_offscreen_shader = sg_make_shader(&screen_shader_desc);
  binocle_log_info("Done compiling offscreen GUI shader");

  imgui_to_offscreen_action.colors[0].action = SG_ACTION_DONTCARE;
  imgui_to_offscreen_action.depth.action = SG_ACTION_DONTCARE;
  imgui_to_offscreen_action.stencil.action = SG_ACTION_DONTCARE;

  // Render pass that renders to the offscreen render target
  imgui_to_offscreen_pass = sg_make_pass(&(sg_pass_desc){
    .color_attachments[0].image = gd->offscreen.render_target,
  });

  // Pipeline state object for the offscreen rendered sprite
  imgui_to_offscreen_pip = sg_make_pipeline(&(sg_pipeline_desc) {
    .layout = {
      .attrs = {
        [0] = { .format = SG_VERTEXFORMAT_FLOAT2 }, // position
      },
    },
    .shader = imgui_to_offscreen_shader,
    .index_type = SG_INDEXTYPE_UINT16,
    .depth = {
      .pixel_format = SG_PIXELFORMAT_NONE,
      .compare = SG_COMPAREFUNC_NEVER,
      .write_enabled = false,
    },
    .stencil = {
      .enabled = false,
    },
    .colors = {
      [0] = {
#ifdef BINOCLE_GL
        .pixel_format = SG_PIXELFORMAT_RGBA8,
#else
        .pixel_format = BINOCLE_PIXELFORMAT_BGRA8,
#endif
        .blend = {
          .enabled = true,
          .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
          .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        },
//          .write_mask = BINOCLE_COLORMASK_RGB,
      }
    }
  });

  sg_buffer_desc vbuf_desc = {
    .data = SG_RANGE(vertices),
  };
  imgui_to_offscreen_vbuf = sg_make_buffer(&vbuf_desc);

  sg_buffer_desc display_ibuf_desc = {
    .type = SG_BUFFERTYPE_INDEXBUFFER,
    .data = SG_RANGE(indices)
  };
  imgui_to_offscreen_ibuf = sg_make_buffer(&display_ibuf_desc);


  imgui_to_offscreen_bind = (sg_bindings){
    .vertex_buffers = {
      [0] = imgui_to_offscreen_vbuf,
    },
    .index_buffer = imgui_to_offscreen_ibuf,
    .fs_images = {
      [0] = imgui_render_target,
    }
  };
  binocle_log_info("Done setting up IMGUI offscreen pipeline");
}*/

void gui_imgui_to_offscreen_render(float width, float height) {
  imgui_to_offscreen_shader_fs_params_t uniforms = {
    .resolution = {
      width, height,
    }
  };

  sg_begin_pass(imgui_to_offscreen_pass, &imgui_to_offscreen_action);
  sg_apply_pipeline(imgui_to_offscreen_pip);
  sg_apply_bindings(&imgui_to_offscreen_bind);
  sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &SG_RANGE(uniforms));
  sg_draw(0, 6, 1);
  sg_end_pass();
}

void gui_pass_input_to_imgui(gui_handle_t handle, binocle_input *input) {
  gui_t *gui = gui_resources_get_gui_with_handle(handle);
  gui_set_context(gui);
  float ratio = 1.0f;

  kmVec2 pos = {
    .x = input->mouseX,
    .y = input->mouseY,
  };
  if (gui->viewport_adapter != NULL) {
    ratio = gui->rt_w / gui->viewport_adapter->viewport.max.x;
//    binocle_log_info("M: %.0f %.0f {%.1f}", pos.x, pos.y, ratio);
    pos = binocle_viewport_adapter_point_to_virtual_viewport(*gui->viewport_adapter, pos);
//    binocle_log_info("V: %.0f %.0f {%.0f %.0f %.0f %.0f}", pos.x, pos.y, gui->viewport_adapter->viewport.min.x, gui->viewport_adapter->viewport.min.y, gui->viewport_adapter->viewport.max.x, gui->viewport_adapter->viewport.max.y);
  } else {
    ratio = gui->rt_w / gui->viewport_w;
  }

  pos.x *= ratio;
  pos.y *= ratio;

  ImGuiIO *io = igGetIO();
  io->MouseDown[0] = input->currentMouseButtons[MOUSE_LEFT];
  io->MouseDown[1] = input->currentMouseButtons[MOUSE_RIGHT];
  io->MouseDown[2] = input->currentMouseButtons[MOUSE_MIDDLE];
  io->MousePos.x = pos.x;
  io->MousePos.y = pos.y;
  if (input->mouseWheelX < 0) {
    io->MouseWheelH += 1;
  }
  if (input->mouseWheelX > 0) {
    io->MouseWheelH -= 1;
  }
  if (input->mouseWheelY > 0) {
    io->MouseWheel += 1;
  }
  if (input->mouseWheelY < 0) {
    io->MouseWheel -= 1;
  }
  for (int i = 0 ; i < sizeof(input->currentKeys) ; i++) {
    io->KeysDown[i] = input->currentKeys[i];
  }
  io->KeyShift = binocle_input_shift(*input);
  io->KeyCtrl = binocle_input_ctrl(*input);
  io->KeyAlt = binocle_input_alt(*input);
  ImGuiIO_AddInputCharactersUTF8(io, input->text);
}

void gui_setup_screen_pipeline(gui_handle_t handle, sg_shader display_shader, bool pixel_perfect) {
  // Render to screen pipeline

  gui_t *gui = gui_resources_get_gui_with_handle(handle);

  // shader object for imgui rendering
  sg_shader_desc shd_desc = {0};
  shd_desc.attrs[0].name = "position";
  shd_desc.vs.uniform_blocks[0].size = sizeof(screen_vs_params_t);
  shd_desc.vs.uniform_blocks[0].uniforms[0].name = "transform";
  shd_desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_MAT4;
#if defined(__IPHONEOS__) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
  shd_desc.vs.source =
    "#version 300 es\n"
    "precision mediump float;\n"
    "precision mediump int;\n"
    "in vec3 position;\n"
    "uniform mat4 transform;\n"
    "out vec2 uvCoord;\n"
    "void main(void) {\n"
    "gl_Position = transform * vec4( position, 1.0 );\n"
    "uvCoord = (position.xy + vec2(1,1))/2.0;\n"
    "}\n";
#else
  shd_desc.vs.source =
    "#version 330\n"
    "in vec3 position;\n"
    "uniform mat4 transform;\n"
    "out vec2 uvCoord;\n"
    "void main(void) {\n"
    "gl_Position = transform * vec4( position, 1.0 );\n"
    "uvCoord = (position.xy + vec2(1,1))/2.0;\n"
    "}\n";
#endif
  shd_desc.fs.images[0].name = "tex0";
  shd_desc.fs.images[0].image_type = SG_IMAGETYPE_2D;
  shd_desc.fs.images[0].sampler_type = SG_SAMPLERTYPE_FLOAT;
  shd_desc.fs.uniform_blocks[0].size = sizeof(screen_fs_params_t);
  shd_desc.fs.uniform_blocks[0].uniforms[0].name = "resolution";
  shd_desc.fs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;
  shd_desc.fs.uniform_blocks[0].uniforms[1].name = "scale";
  shd_desc.fs.uniform_blocks[0].uniforms[1].type = SG_UNIFORMTYPE_FLOAT2;
  shd_desc.fs.uniform_blocks[0].uniforms[2].name = "viewport";
  shd_desc.fs.uniform_blocks[0].uniforms[2].type = SG_UNIFORMTYPE_FLOAT2;
#if defined(__IPHONEOS__) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
  if (pixel_perfect) {
      shd_desc.fs.source =
    "#version 300 es\n"
    "precision mediump float;\n"
    "precision mediump int;\n"
    "uniform sampler2D tex0;\n"
    "in vec2 uvCoord;\n"
    "out vec4 fragColor;\n"
          "vec2 uv_iq( vec2 uv, vec2 texture_size ) {\n"
      "    vec2 pixel = uv * texture_size;\n"
      "\n"
      "    vec2 seam = floor(pixel + 0.5);\n"
      "    vec2 dudv = fwidth(pixel);\n"
      "    pixel = seam + clamp( (pixel - seam) / dudv, -0.5, 0.5);\n"
      "\n"
      "    return pixel / texture_size;\n"
      "}\n"
      "\n"
      "void main() {\n"
      "\n"
      "    vec2 uv = (gl_FragCoord.xy - floor(viewport.xy)) / resolution.xy * scale;\n"
      "    vec2 pixelPerfectUV = uv_iq(uv, resolution.xy);\n"
      "    fragColor = texture( tex0, pixelPerfectUV );\n"
      "\n"
      "}\n";
  } else {
      shd_desc.fs.source =
    "#version 300 es\n"
    "precision mediump float;\n"
    "precision mediump int;\n"
    "uniform sampler2D tex0;\n"
    "in vec2 uvCoord;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    vec4 texcolor = texture(tex0, uvCoord);\n"
    "    fragColor = texcolor;\n"
    "    //gl_FragColor = texture2D(tex, uv) * color;\n"
    "}\n";
  }
#else
  if (pixel_perfect) {
    shd_desc.fs.source =
      "#version 330\n"
      "uniform vec2 resolution;\n"
      "uniform sampler2D tex0;\n"
      "uniform vec2 scale;\n"
      "uniform vec2 viewport;\n"
      "out vec4 fragColor;\n"
      "in vec2 uvCoord;\n"
      "vec2 uv_iq( vec2 uv, ivec2 texture_size ) {\n"
      "    vec2 pixel = uv * texture_size;\n"
      "\n"
      "    vec2 seam = floor(pixel + 0.5);\n"
      "    vec2 dudv = fwidth(pixel);\n"
      "    pixel = seam + clamp( (pixel - seam) / dudv, -0.5, 0.5);\n"
      "\n"
      "    return pixel / texture_size;\n"
      "}\n"
      "\n"
      "void main() {\n"
      "\n"
      "    vec2 uv = (gl_FragCoord.xy - floor(viewport.xy)) / resolution.xy * scale;\n"
      "    vec2 pixelPerfectUV = uv_iq(uv, ivec2(resolution.xy));\n"
      "    fragColor = texture(tex0, pixelPerfectUV);\n"
      "}\n";
  } else {
    shd_desc.fs.source =
      "#version 330\n"
      "uniform vec2 resolution;\n"
      "uniform sampler2D tex0;\n"
      "uniform vec2 scale;\n"
      "uniform vec2 viewport;\n"
      "out vec4 fragColor;\n"
      "in vec2 uvCoord;\n"
      "void main(void) {\n"
      "vec4 texcolor = texture(tex0, uvCoord);\n"
      "//if (texcolor.a < 1) discard;\n"
      "fragColor = texcolor;\n"
      "}\n";
  }
#endif
  binocle_log_info("Compiling GUI shader for screen pipeline");
  sg_shader shd = sg_make_shader(&shd_desc);
  binocle_log_info("Done compiling GUI shader for screen pipeline");

  // Clear screen action for the actual screen
  sg_color clear_color = binocle_color_green();
  sg_pass_action default_action = {
    .colors[0] = {
      .action = SG_ACTION_DONTCARE,
      .value = {
        .r = clear_color.r,
        .g = clear_color.g,
        .b = clear_color.b,
        .a = clear_color.a,
      }
    }
  };
  gui_screen_pass_action = default_action;

  binocle_log_info("Creating GUI pipeline");
  // Pipeline state object for the screen (default) pass
  gui->gui_screen_pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
      .attrs = {
        [0] = { .format = SG_VERTEXFORMAT_FLOAT3 }, // position
//        [1] = { .format = SG_VERTEXFORMAT_FLOAT4 }, // color
//        [2] = { .format = SG_VERTEXFORMAT_FLOAT2 }, // texture uv
      }
    },
    .shader = shd,
    .index_type = SG_INDEXTYPE_UINT16,
#if !defined(BINOCLE_GL)
    .depth = {
      .pixel_format = SG_PIXELFORMAT_NONE,
      .compare = SG_COMPAREFUNC_NEVER,
      .write_enabled = false,
    },
    .stencil = {
      .enabled = false,
    },
#endif
    .colors = {
#ifdef BINOCLE_GL
      [0] = {
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .blend = {
          .enabled = true,
          .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
          .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
          .op_alpha = SG_BLENDOP_ADD,
          .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
          .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
          .op_rgb = SG_BLENDOP_ADD,
        }
      }
#else
      [0] = { .pixel_format = SG_PIXELFORMAT_BGRA8 }
#endif
    }
  });
  binocle_log_info("Done creating GUI pipeline");

//  float vertices[] = {
//    /* pos                  color                       uvs */
//    -1.0f, -1.0f, 0.0f,    1.0f, 1.0f, 1.0f, 1.0f,     0.0f, 0.0f,
//    1.0f, -1.0f, 0.0f,    1.0f, 1.0f, 1.0f, 1.0f,     1.0f, 0.0f,
//    1.0f,  1.0f, 0.0f,    1.0f, 1.0f, 1.0f, 1.0f,     1.0f, 1.0f,
//    -1.0f,  1.0f, 0.0f,    1.0f, 1.0f, 1.0f, 1.0f,     0.0f, 1.0f,
//  };
  float vertices[] = {
    /* pos                  color                       uvs */
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
  };
  sg_buffer_desc display_vbuf_desc = {
    .data = SG_RANGE(vertices)
  };
  gui_screen_vbuf = sg_make_buffer(&display_vbuf_desc);

  uint16_t indices[] = {
    0, 1, 2,  0, 2, 3,
  };
  sg_buffer_desc display_ibuf_desc = {
    .type = SG_BUFFERTYPE_INDEXBUFFER,
    .data = SG_RANGE(indices)
  };
  gui_screen_ibuf = sg_make_buffer(&display_ibuf_desc);

  gui_screen_bind = (sg_bindings){
    .vertex_buffers = {
      [0] = gui_screen_vbuf,
    },
    .index_buffer = gui_screen_ibuf,
    .fs_images = {
      [0] = gui->imgui_render_target,
    }
  };
}

void gui_render_to_screen(gui_t *gui, binocle_gd *gd, struct binocle_window *window, float design_width, float design_height, kmAABB2 viewport, kmMat4 matrix, float scale) {
  // Render the offscreen to the display



  screen_vs_params_t screen_vs_params;
  screen_fs_params_t screen_fs_params;

//  screen_vs_params.transform = matrix;
  kmMat4Identity(&screen_vs_params.transform);

  screen_fs_params.resolution[0] = design_width;
  screen_fs_params.resolution[1] = design_height;
  screen_fs_params.scale[0] = scale;
  screen_fs_params.scale[1] = scale;
  screen_fs_params.viewport[0] = viewport.min.x;
  screen_fs_params.viewport[1] = viewport.min.y;

  gui_screen_bind.fs_images[0] = gui->imgui_render_target;

  sg_begin_default_pass(&gui_screen_pass_action, window->width, window->height);
  sg_apply_pipeline(gui->gui_screen_pip);
  sg_apply_bindings(&gui_screen_bind);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(screen_vs_params));
  sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &SG_RANGE(screen_fs_params));
  if (gui->apply_scissor) {
    sg_apply_scissor_rect(viewport.min.x, viewport.min.y, design_width / scale, design_height / scale, false);
  }
  sg_draw(0, 6, 1);
  sg_end_pass();

  sg_commit();
}

void gui_wrap_new_frame(binocle_window *window, float dt, int w, int h, int display_w, int display_h) {
  ImGuiIO *io = igGetIO();

  io->DisplaySize.x = (float)w; //ImVec2_ImVec2Float((float)w, (float)h)->x;
  io->DisplaySize.y = (float)h; //ImVec2_ImVec2Float((float)w, (float)h)->y;
  io->DisplayFramebufferScale.x = w > 0 ? ((float)display_w / w) : 1; //ImVec2_ImVec2Float(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0)->x;
  io->DisplayFramebufferScale.y = h > 0 ? ((float)display_h / h) : 1; //ImVec2_ImVec2Float(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0)->y;
  io->DeltaTime = dt;

  igNewFrame();

  igSetNextWindowPos(*ImVec2Zero, ImGuiCond_Once, *ImVec2Zero);
}

int l_gui_wrap_new_frame(lua_State *L) {
  l_binocle_window_t *window_wrapper = luaL_checkudata(L, 1, "binocle_window");
  float dt = (float)lua_tonumber(L, 2);
  int w = lua_tonumber(L, 3);
  int h = lua_tonumber(L, 4);
  int display_w = w;
  int display_h = h;
  if (w == 0 || h == 0) {
    SDL_GetWindowSize(window_wrapper->window->window, &w, &h);
    SDL_GL_GetDrawableSize(window_wrapper->window->window, &display_w, &display_h);
  }
  gui_wrap_new_frame(window_wrapper->window, dt, w, h, display_w, display_h);
  return 0;
}

void gui_wrap_render_frame(gui_t *gui) {
  sg_begin_pass(gui->imgui_pass, &imgui_pass_action);
  igRender();
  draw_imgui(igGetDrawData());
  sg_end_pass();
}


int l_gui_wrap_render_frame(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  gui_t *gui = gui_resources_get_gui(name);
  gui_wrap_render_frame(gui);
  return 0;
}

int l_gui_wrap_render_to_screen(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  l_binocle_gd_t *gd = luaL_checkudata(L, 2, "binocle_gd");
  l_binocle_window_t *window = luaL_checkudata(L, 3, "binocle_window");
  float design_width = (float)luaL_checknumber(L, 4);
  float design_height = (float)luaL_checknumber(L, 5);
  kmAABB2 **vp = lua_touserdata(L, 6);
  l_binocle_camera_t *camera = luaL_checkudata(L, 7, "binocle_camera");
  gui_t *gui = gui_resources_get_gui(name);
  gui_render_to_screen(gui, gd->gd, window->window, design_width, design_height, **vp, camera->camera->viewport_adapter->scale_matrix, camera->camera->viewport_adapter->inverse_multiplier);
  return 0;
}

int l_gui_wrap_set_context(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  gui_t *gui = gui_resources_get_gui(name);
  if (gui != NULL) {
    gui_set_context(gui);
    lua_pushboolean(L, true);
  } else {
    lua_pushboolean(L, false);
  }
  return 1;
}

int l_gui_wrap_get_want_capture_mouse(lua_State *L) {
  ImGuiIO *io = igGetIO();
  lua_pushboolean(L, io->WantCaptureMouse);
  return 1;
}

int l_gui_wrap_get_style_frame_padding(lua_State *L) {
  ImGuiStyle *style = igGetStyle();
  ImVec2 padding = style->FramePadding;
  lua_pushnumber(L, padding.x);
  lua_pushnumber(L, padding.y);
  return 2;
}