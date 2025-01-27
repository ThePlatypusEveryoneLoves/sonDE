#pragma once

#include "common.h"
#include "config.h"

enum sonde_cursor_mode {
  SONDE_CURSOR_PASSTHROUGH,
  SONDE_SERVER_MOVE_WINDOW,
  SONDE_SERVER_RESIZE_WINDOW
};

struct sonde_server {
  struct wl_display *display;
  struct wl_event_loop *event_loop;
  struct wlr_backend *backend;
  struct wlr_renderer *renderer;
  struct wlr_allocator *allocator;

  struct sonde_config config;

  // outputs
  struct wlr_output_layout *output_layout;
  struct wl_list outputs;
  struct wl_listener new_output;
  struct wlr_scene *scene;
  struct wlr_scene_output_layout *scene_layout;

  // views
  struct wl_list views;

  // xdg-shell
  struct wlr_xdg_shell *xdg_shell;
  struct wl_listener new_toplevel;
  struct wl_listener new_popup;

  // TODO: xwayland-shell

  // xdg-shell decoration
  struct wlr_xdg_decoration_manager_v1 *decoration_manager;
  struct wl_listener new_toplevel_decoration;
  //struct wlr_server_decoration_manager *kde_decoration_manager;

  // layer-shell
  struct wlr_layer_shell_v1 *layer_shell;
  struct wl_listener new_layer_shell_surface;

  // seat
  struct wl_list keyboards;
  struct wl_listener new_input;
  struct wlr_seat *seat;
  struct wl_listener request_cursor;
  struct wl_listener request_set_selection;

  // cursor
  struct wlr_cursor *cursor;
  enum sonde_cursor_mode cursor_mode;
  struct wlr_pointer_constraints_v1 *pointer_constraints;
  struct wlr_pointer_constraint_v1 *current_pointer_constraint;
  struct wlr_xcursor_manager *cursor_manager;
  struct wl_listener cursor_motion;
  struct wl_listener cursor_motion_absolute;
  struct wl_listener cursor_button;
  struct wl_listener cursor_axis;
  struct wl_listener cursor_frame;
  struct wl_listener new_pointer_constraint;

  const char *socket;
};

typedef struct sonde_server *sonde_server_t;

int sonde_server_create(sonde_server_t server);
int sonde_server_start(sonde_server_t server);
void sonde_server_destroy(sonde_server_t server);
