#pragma once

#include "common.h"
#include "xdg-shell.h"

struct sonde_xdg_decoration {
  struct wlr_xdg_toplevel_decoration_v1 *xdg_decoration;
  enum wlr_xdg_toplevel_decoration_v1_mode client_mode;
  sonde_xdg_view_t sonde_xdg_view;

  struct wlr_scene_rect *rect;

  // events
  struct wl_listener destroy;
  struct wl_listener request_mode;
  struct wl_listener surface_commit;
};

void sonde_apply_decorations(struct sonde_xdg_decoration *sonde_xdg_decoration);
void sonde_decoration_set_focus(struct sonde_xdg_decoration *decoration,
                                bool focused);
