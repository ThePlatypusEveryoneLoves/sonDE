#pragma once

#include "common.h"
#include "server.h"
#include "decorations.h"
#include "outputs.h"

struct sonde_view {
  sonde_server_t server;
  struct wl_list link;

  struct sonde_output *output;

  struct wl_listener destroy;
  struct wl_listener surface_destroy;
  struct wl_listener commit;
  struct wl_listener request_move;
  struct wl_listener request_resize;
  struct wl_listener request_maximize;
  struct wl_listener request_minimize;
  struct wl_listener request_fullscreen;
  struct wl_listener set_title;
  struct wl_listener map;
  struct wl_listener unmap;

  struct sonde_xdg_decoration decoration;

  struct wlr_scene_tree *scene_tree;
  struct wlr_scene_tree *surface_scene_tree;
  struct wlr_surface *surface;

  // TODO: better tiling
  bool is_on_right;
  
  enum {
    SONDE_XDG,
#ifdef SONDE_XWAYLAND
    SONDE_XWAYLAND
#endif
  } type;
};

typedef struct sonde_view *sonde_view_t;

void sonde_view_focus(sonde_view_t sonde_view);

/// finds the toplevel and surface at position (lx, ly)
/// returns the toplevel, sets surface to the corresponding surface,
/// and sets sx and sy to the surface-local coordinates
sonde_view_t sonde_view_at(sonde_server_t server, double lx, double ly,
                           struct wlr_surface **surface, double *sx,
                           double *sy);
void sonde_view_change_tiling(sonde_view_t sonde_view, bool on_right);

void sonde_view_update_sizing(sonde_view_t view);
void sonde_view_update_sizing_all(sonde_server_t server);
