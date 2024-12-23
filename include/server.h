#pragma once

#include "common.h"

struct sonde_server {
  struct wl_display *display;
  struct wlr_backend *backend;
  struct wlr_renderer *renderer;
  struct wlr_allocator *allocator;

  // outputs
  struct wlr_output_layout *output_layout;
  struct wl_list outputs;
  struct wl_listener new_output;
  struct wlr_scene *scene;
  struct wlr_scene_output_layout *scene_layout;

  // xdg-shell
  struct wl_list toplevels;
  struct wlr_xdg_shell *xdg_shell;
  struct wl_listener new_toplevel;
  struct wl_listener new_popup;
  struct sonde_toplevel *grabbed_toplevel;

  // seat
  struct wl_list keyboards;
  struct wl_listener new_input;
  struct wlr_seat *seat;
  struct wl_listener request_cursor;
  struct wl_listener request_set_selection;
  
  const char* socket;
};

typedef struct sonde_server *sonde_server_t;

int sonde_server_create(sonde_server_t server);
int sonde_server_start(sonde_server_t server);
void sonde_server_destroy(sonde_server_t server);
