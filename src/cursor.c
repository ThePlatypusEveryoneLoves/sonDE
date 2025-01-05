#include "cursor.h"
#include "server.h"
#include "view.h"

static void process_cursor_motion(sonde_server_t server, uint32_t time) {
  switch (server->cursor_mode) {
  case SONDE_CURSOR_PASSTHROUGH:
    {
      // send event to toplevel
      struct wlr_surface *surface = NULL;
      double sx, sy;
      sonde_view_t sonde_view = sonde_view_at(server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);
      
      if (sonde_view == NULL) {
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
    sonde_view_t sonde_view = sonde_view_at(server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);
    sonde_view_focus(sonde_view);
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

/// warp on unlock, if a cursor hint exists
/// (the cursor hint represents where the user sees the cursor (the application might draw their own cursor))
/// we have to warp to this cursor hint to maintain consistency
void warp_unlock_pointer(
  struct sonde_pointer_constraint *sonde_pointer_constraint) {
  sonde_server_t server = sonde_pointer_constraint->server;
  struct wlr_seat *seat = server->seat;
  struct wlr_pointer_constraint_v1_state *pointer_constraint = &sonde_pointer_constraint->pointer_constraint->current;
  struct wlr_surface *current_surface = seat->pointer_state.focused_surface;
  
  if ((pointer_constraint->committed & WLR_POINTER_CONSTRAINT_V1_STATE_CURSOR_HINT) && current_surface != NULL) {
    // warp the cursor
    // note: cursor_hint gives surface-local coords
    struct wlr_box surface_coords;
    wlr_surface_get_extends(current_surface, &surface_coords);
    wlr_cursor_warp(
      server->cursor, NULL,
      surface_coords.x + pointer_constraint->cursor_hint.x,
      surface_coords.y + pointer_constraint->cursor_hint.y);
    
    // change the internal pointer position
    wlr_seat_pointer_warp(
      seat,pointer_constraint->cursor_hint.x, pointer_constraint->cursor_hint.y);  
  }
}

void sonde_cursor_apply_pointer_constraint(
  sonde_server_t server,
  struct wlr_pointer_constraint_v1 *pointer_constraint) {
  // no change
  if (server->current_pointer_constraint == pointer_constraint) return;

  // deactivate current
  if (server->current_pointer_constraint) {
    // warp unlock if unsetting
    if (pointer_constraint == NULL)
      warp_unlock_pointer(server->current_pointer_constraint->data);
    
    // deactivate current pointer constraint
    wlr_pointer_constraint_v1_send_deactivated(server->current_pointer_constraint);
  }

  if (pointer_constraint != NULL)
    wlr_pointer_constraint_v1_send_activated(pointer_constraint);
  
  server->current_pointer_constraint = pointer_constraint;
}

WL_CALLBACK(on_pointer_constraint_destroy) {
  struct sonde_pointer_constraint *sonde_pointer_constraint = wl_container_of(listener, sonde_pointer_constraint, destroy);

  wl_list_remove(&sonde_pointer_constraint->destroy.link);

  // clear the current pointer constraint if needed
  if (sonde_pointer_constraint->server->current_pointer_constraint == sonde_pointer_constraint->pointer_constraint) {
    sonde_pointer_constraint->server->current_pointer_constraint = NULL;

    warp_unlock_pointer(sonde_pointer_constraint);
  }

  free(sonde_pointer_constraint);
}

WL_CALLBACK(on_new_pointer_constraint) {
  struct wlr_pointer_constraint_v1 *pointer_constraint = data;
  sonde_server_t server = wl_container_of(listener, server, new_pointer_constraint);

  struct sonde_pointer_constraint *sonde_pointer_constraint = calloc(1, sizeof(*sonde_pointer_constraint));

  sonde_pointer_constraint->pointer_constraint = pointer_constraint;
  sonde_pointer_constraint->server = server;
  // set the data field to the sonde_pointer_constraint
  pointer_constraint->data = sonde_pointer_constraint;

  LISTEN(&pointer_constraint->events.destroy, &sonde_pointer_constraint->destroy, on_pointer_constraint_destroy);

  // if this is focused, apply constraint
  if (server->seat->pointer_state.focused_surface == pointer_constraint->surface) {
    sonde_cursor_apply_pointer_constraint(server, pointer_constraint);
  }
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

  // pointer constraints
  server->pointer_constraints = wlr_pointer_constraints_v1_create(server->display);
  if (server->pointer_constraints == NULL) {
    return 1;
  }

  LISTEN(&server->cursor->events.motion, &server->cursor_motion, on_cursor_motion);
  LISTEN(&server->cursor->events.motion_absolute, &server->cursor_motion_absolute, on_cursor_motion_absolute);
  LISTEN(&server->cursor->events.button, &server->cursor_button, on_cursor_button);
  LISTEN(&server->cursor->events.axis, &server->cursor_axis, on_cursor_axis);
  LISTEN(&server->cursor->events.frame, &server->cursor_frame, on_cursor_frame);

  LISTEN(&server->pointer_constraints->events.new_constraint, &server->new_pointer_constraint, on_new_pointer_constraint);

  return 0;
}

void sonde_cursor_destroy(sonde_server_t server) {
  wlr_xcursor_manager_destroy(server->cursor_manager);
  if (server->cursor != NULL) wlr_cursor_destroy(server->cursor);
}
