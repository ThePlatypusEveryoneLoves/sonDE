#pragma once

#include "common.h"
#include "server.h"

struct sonde_layer_surface {
  sonde_server_t server;
  struct wl_list link;

  struct wl_listener destroy;
  struct wl_listener commit;
  struct wl_listener map;
  struct wl_listener unmap;

  struct wlr_scene_layer_surface_v1 *scene_tree;
  struct wlr_layer_surface_v1 *layer_surface;
  struct wlr_surface *surface;
};

int sonde_layer_shell_initialize(sonde_server_t server);
void sonde_layer_shell_destroy(sonde_server_t server);
