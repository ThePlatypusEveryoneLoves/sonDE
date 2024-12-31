#pragma once

#include "server.h"
#include "common.h"
#include "view.h"

struct sonde_xdg_view {
  struct sonde_view base;
  struct wlr_xdg_toplevel *toplevel;
  struct wl_listener set_app_id;
  struct wl_listener show_window_menu;
  struct wl_listener new_popup;
};

struct sonde_popup {
  sonde_server_t server;
  struct wlr_xdg_popup *popup;

  struct wl_listener commit;
  struct wl_listener destroy;
};

int sonde_xdg_shell_initialize(sonde_server_t server);
void sonde_xdg_shell_destroy(sonde_server_t server);

typedef struct sonde_xdg_view* sonde_xdg_view_t;

static inline sonde_xdg_view_t sonde_xdg_view_from_sonde_view(sonde_view_t sonde_view) {
  sonde_xdg_view_t sonde_xdg_view = wl_container_of(sonde_view, sonde_xdg_view, base);
  return sonde_xdg_view;
}
