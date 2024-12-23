#pragma once

#include "server.h"

struct sonde_toplevel {
  struct wl_list link;
  struct sonde_server *server;
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
  struct sonde_server *server;
  struct wlr_xdg_popup *popup;

  struct wl_listener commit;
  struct wl_listener destroy;
};

int sonde_xdg_shell_initialize(struct sonde_server *server);
void sonde_xdg_shell_destroy(struct sonde_server *server);
