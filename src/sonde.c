#include <bits/time.h>
#include <time.h>
#include "server.h"
#include "common.h"

void on_new_toplevel(struct wl_listener *listener, void *data) {
  struct sonde_server *server = wl_container_of(listener, server, new_toplevel);

  struct wlr_xdg_toplevel *toplevel = data;
}

void on_new_popup(struct wl_listener *listener, void *data) {
  struct sonde_server *server = wl_container_of(listener, server, new_popup);
}
void on_new_input(struct wl_listener *listener, void *data) {
  struct sonde_server *server = wl_container_of(listener, server, new_input);
}
void on_request_cursor(struct wl_listener *listener, void *data) {
  struct sonde_server *server = wl_container_of(listener, server, request_cursor);
}
void on_request_set_selection(struct wl_listener *listener, void *data) {
  struct sonde_server *server = wl_container_of(listener, server, request_set_selection);
  struct wlr_seat_request_set_selection_event *event = data;
  wlr_seat_set_selection(server->seat, event->source, event->serial);
}


int main() {
  // TODO: customizable log levels
  wlr_log_init(WLR_DEBUG, NULL);

  struct sonde_server server = {0};
  if (sonde_server_create(&server) != 0) {
    // failed to create server
    sonde_server_destroy(&server);
    return 1;
  }

  if (sonde_server_start(&server) != 0) {
    // failed to start server
    sonde_server_destroy(&server);
    return 1;
  }

  // stop
  sonde_server_destroy(&server);

  return 0;
}
