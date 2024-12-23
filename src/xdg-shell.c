#include "xdg-shell.h"

void on_new_toplevel(struct wl_listener *listener, void *data) {
  struct sonde_server *server = wl_container_of(listener, server, new_toplevel);

  struct wlr_xdg_toplevel *toplevel = data;
}

void on_new_popup(struct wl_listener *listener, void *data) {
  struct sonde_server *server = wl_container_of(listener, server, new_popup);
}


int sonde_xdg_shell_initialize(struct sonde_server *server) {
  // XDG Shell
  wl_list_init(&server->toplevels);
  server->xdg_shell = wlr_xdg_shell_create(server->display, 3);
  if (server->xdg_shell == NULL) {
    wlr_log(WLR_ERROR, "failed to create xdg-shell");
    return 1;
  }
  
  server->new_toplevel.notify = on_new_toplevel;
  server->new_popup.notify = on_new_popup;

  wl_signal_add(&server->xdg_shell->events.new_toplevel, &server->new_toplevel);
  wl_signal_add(&server->xdg_shell->events.new_popup, &server->new_popup);

  return 0;
}

void sonde_xdg_shell_destroy(struct sonde_server *server) {
  // nothing to do here for now
}
