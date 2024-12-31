#pragma once

#include "common.h"
#include "server.h"
#include "xdg-shell.h"

struct sonde_xdg_decoration {
  struct wlr_xdg_toplevel_decoration_v1 *xdg_decoration;
  enum wlr_xdg_toplevel_decoration_v1_mode client_mode;
  sonde_xdg_view_t sonde_xdg_view;

  // events
  struct wl_listener destroy;
  struct wl_listener request_mode;
  struct wl_listener surface_commit;
};

int sonde_decoration_manager_initialize(sonde_server_t server);
void sonde_decoration_manager_destroy(sonde_server_t server);
