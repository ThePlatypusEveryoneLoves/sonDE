#pragma once

#include "common.h"
#include "cairo-buffer.h"
#include <pango/pangocairo.h>

#define SONDE_DECORATION_BORDER_WIDTH 1
#define SONDE_DECORATION_TITLEBAR_HEIGHT 20

struct sonde_xdg_decoration {
  struct wlr_xdg_toplevel_decoration_v1 *xdg_decoration;
  enum wlr_xdg_toplevel_decoration_v1_mode client_mode;

  struct wlr_scene_tree *scene_tree;
  struct wlr_scene_rect *rect;

  // titlebar
  struct sonde_cairo_buffer *titlebar;
  struct wlr_scene_buffer *titlebar_node;
  PangoLayout *titlebar_pango;

  bool focused;

  // events
  struct wl_listener destroy;
  struct wl_listener request_mode;
  struct wl_listener surface_commit;
};

void sonde_apply_decorations(struct sonde_xdg_decoration *decoration);
void sonde_decoration_set_focus(
  struct sonde_xdg_decoration *decoration, bool focused);
void sonde_decoration_destroy(
  struct sonde_xdg_decoration *sonde_xdg_decoration);
void sonde_decoration_update_title(
  struct sonde_xdg_decoration *decoration);
void sonde_decoration_update_size(struct sonde_xdg_decoration *decoration);
