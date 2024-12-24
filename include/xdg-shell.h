#pragma once

#include "server.h"

struct sonde_toplevel {
  struct wl_list link;
  
  sonde_server_t server;
  struct wlr_xdg_toplevel *toplevel;
  struct wlr_scene_tree *scene_tree;

  // events
  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener commit;
  struct wl_listener destroy;
  struct wl_listener request_move;
  struct wl_listener request_resize;
  struct wl_listener request_maximize;
  struct wl_listener request_fullscreen;
};

struct sonde_popup {
  sonde_server_t server;
  struct wlr_xdg_popup *popup;

  struct wl_listener commit;
  struct wl_listener destroy;
};

int sonde_xdg_shell_initialize(sonde_server_t server);
void sonde_xdg_shell_destroy(sonde_server_t server);

void sonde_toplevel_focus(struct sonde_toplevel *toplevel);
