#include "common.h"
#include "server.h"

struct sonde_view {
  sonde_server_t server;
  struct wl_list link;

  struct wl_listener destroy;
  struct wl_listener surface_destroy;
  struct wl_listener commit;
  struct wl_listener request_move;
  struct wl_listener request_resize;
  struct wl_listener request_maximize;
  struct wl_listener request_minimize;
  struct wl_listener request_fullsize;
  struct wl_listener set_title;
  struct wl_listener map;
  struct wl_listener unmap;

  struct wlr_scene_tree *scene_tree;
  struct wlr_surface surface;
  enum {
    SONDE_XDG,
#ifdef SONDE_XWAYLAND
    SONDE_XWAYLAND
#endif
  } type;
};

struct sonde_xdg_view {
  struct sonde_view base;
  struct wlr_xdg_surface xdg_surface;
  struct wlr_xdg_toplevel *toplevel;
  struct wl_listener set_app_id;
  struct wl_listener show_window_menu;
  struct wl_listener new_popup;
};
