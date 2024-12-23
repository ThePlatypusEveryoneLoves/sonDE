#include "seat.h"

void on_new_input(struct wl_listener *listener, void *data) {
  sonde_server_t server = wl_container_of(listener, server, new_input);
}
void on_request_cursor(struct wl_listener *listener, void *data) {
  sonde_server_t server = wl_container_of(listener, server, request_cursor);
}
void on_request_set_selection(struct wl_listener *listener, void *data) {
  sonde_server_t server = wl_container_of(listener, server, request_set_selection);
  struct wlr_seat_request_set_selection_event *event = data;
  wlr_seat_set_selection(server->seat, event->source, event->serial);
}

int sonde_seat_initialize(sonde_server_t server) {
  wl_list_init(&server->keyboards);
  server->new_input.notify = on_new_input;
  server->seat = wlr_seat_create(server->display, "seat0");

  if (server->seat == NULL) {
    wlr_log(WLR_ERROR, "failed to create seat");
    return 1;
  }
  
  server->request_cursor.notify = on_request_cursor;
  server->request_set_selection.notify = on_request_set_selection;

  wl_signal_add(&server->backend->events.new_input, &server->new_input);
  wl_signal_add(&server->seat->events.request_set_cursor, &server->request_cursor);
  wl_signal_add(&server->seat->events.request_set_selection, &server->request_set_selection);

  return 0;
}

void sonde_seat_destroy(sonde_server_t server) {
  wlr_seat_destroy(server->seat);
}
