#include "cursor.h"
#include "server.h"
#include "xdg-shell.h"

static void process_cursor_motion(sonde_server_t server, uint32_t time) {
  switch (server->cursor_mode) {
  case SONDE_CURSOR_PASSTHROUGH:
    {
      // send event to toplevel
      struct wlr_surface *surface = NULL;
      double sx, sy;
      struct sonde_toplevel *toplevel = sonde_toplevel_at(server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);
      
      if (toplevel == NULL) {
        wlr_cursor_set_xcursor(server->cursor, server->cursor_manager, "default");
      }
      
      if (surface != NULL) {
        wlr_seat_pointer_notify_enter(server->seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(server->seat, time, sx, sy);
      } else {
        // stop events from being sent to the last surface
        wlr_seat_pointer_clear_focus(server->seat);
      }
      
      break;
    }
  case SONDE_SERVER_MOVE_WINDOW:
    {
      // TODO: stuff here
      break;
    }
  case SONDE_SERVER_RESIZE_WINDOW:
    {
      break;
    }
  }
}

WL_CALLBACK(on_cursor_motion) {
  sonde_server_t server = wl_container_of(listener, server, cursor_motion);
  struct wlr_pointer_motion_event *event = data;

  // actually move the cursor
  wlr_cursor_move(server->cursor, &event->pointer->base, event->delta_x, event->delta_y);

  process_cursor_motion(server, event->time_msec);
}
WL_CALLBACK(on_cursor_motion_absolute) {
  sonde_server_t server = wl_container_of(listener, server, cursor_motion_absolute);
  struct wlr_pointer_motion_absolute_event *event = data;

  // actually move the cursor
  wlr_cursor_warp_absolute(server->cursor, &event->pointer->base, event->x, event->y);

  process_cursor_motion(server, event->time_msec);
}
WL_CALLBACK(on_cursor_button) {
  sonde_server_t server = wl_container_of(listener, server, cursor_button);
  struct wlr_pointer_button_event *event = data;
  // send event to focused client
  wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);

  if (event->state == WL_POINTER_BUTTON_STATE_PRESSED) {
    // focus client
    struct wlr_surface *surface = NULL;
    double sx, sy;
    struct sonde_toplevel *toplevel = sonde_toplevel_at(server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);
    sonde_toplevel_focus(toplevel);
  }
}
WL_CALLBACK(on_cursor_axis) {
  // scroll
  sonde_server_t server = wl_container_of(listener, server, cursor_axis);
  struct wlr_pointer_axis_event *event = data;
  // send event to focused client
  wlr_seat_pointer_notify_axis(
                               server->seat, event->time_msec, event->orientation,
                               event->delta, event->delta_discrete, event->source,
                               event->relative_direction);
}
WL_CALLBACK(on_cursor_frame) {
  sonde_server_t server = wl_container_of(listener, server, cursor_frame);
  wlr_seat_pointer_notify_frame(server->seat);
}

int sonde_cursor_initialize(sonde_server_t server) {
  server->cursor = wlr_cursor_create();
  if (server->cursor == NULL) {
    wlr_log(WLR_ERROR, "could not create cursor");
    return 1;
  }
  wlr_cursor_attach_output_layout(server->cursor, server->output_layout);
  server->cursor_manager = wlr_xcursor_manager_create(NULL, 24);
  if (server->cursor_manager == NULL) {
    wlr_log(WLR_ERROR, "could not create cursor manager");
    return 1;
  }

  server->cursor_mode = SONDE_CURSOR_PASSTHROUGH;

  LISTEN(&server->cursor->events.motion, &server->cursor_motion, on_cursor_motion);
  LISTEN(&server->cursor->events.motion_absolute, &server->cursor_motion_absolute, on_cursor_motion_absolute);
  LISTEN(&server->cursor->events.button, &server->cursor_button, on_cursor_button);
  LISTEN(&server->cursor->events.axis, &server->cursor_axis, on_cursor_axis);
  LISTEN(&server->cursor->events.frame, &server->cursor_frame, on_cursor_frame);

  return 0;
}

void sonde_cursor_destroy(sonde_server_t server) {
  wlr_xcursor_manager_destroy(server->cursor_manager);
  wlr_cursor_destroy(server->cursor);
}
