#include <bits/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

struct sonde_server {
  struct wl_display *display;
  struct wlr_backend *backend;
  struct wlr_renderer *renderer;
  struct wlr_allocator *allocator;
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

  // seat
  struct wl_list keyboards;
  struct wl_listener new_input;
  struct wlr_seat *seat;
  struct wl_listener request_cursor;
  struct wl_listener request_set_selection;
  
  const char* socket;
};

struct sonde_output {
  struct wl_list link;
  struct wlr_output *output;
  struct sonde_server *server;
  
  struct wl_listener frame;
  struct wl_listener request_state;
  struct wl_listener destroy;
};

void on_output_frame(struct wl_listener *listener, void *data) {
  struct sonde_output *sonde_output = wl_container_of(listener, sonde_output, frame);
  struct wlr_scene *scene = sonde_output->server->scene;

  struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(scene, sonde_output->output);

  // render
  wlr_scene_output_commit(scene_output, NULL);

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  wlr_scene_output_send_frame_done(scene_output, &now);
}
void on_output_request_state(struct wl_listener *listener, void *data) {
  struct sonde_output *sonde_output = wl_container_of(listener, sonde_output, request_state);
  const struct wlr_output_event_request_state *event = data;
  // commit the state again
  wlr_output_commit_state(sonde_output->output, event->state);
}
void on_output_destroy(struct wl_listener *listener, void *data) {
  struct sonde_output *sonde_output = wl_container_of(listener, sonde_output, destroy);

  wl_list_remove(&sonde_output->frame.link);
  wl_list_remove(&sonde_output->request_state.link);
  wl_list_remove(&sonde_output->destroy.link);
  wl_list_remove(&sonde_output->link);
  free(sonde_output);
}

void on_new_output(struct wl_listener *listener, void *data) {
  struct sonde_server *server = wl_container_of(listener, server, new_output);
  struct wlr_output *output = data;

  // configure output
  wlr_output_init_render(output, server->allocator, server->renderer);

  struct wlr_output_state state;
  wlr_output_state_init(&state);

  // enable
  wlr_output_state_set_enabled(&state, true);

  // mode is resolution and refresh rate
  // set mode to output's perferred if needed
  // TODO: add output configuration
  struct wlr_output_mode *mode = wlr_output_preferred_mode(output);
  if (mode != NULL) {
    wlr_output_state_set_mode(&state, mode);
  }

  // commit this state
  wlr_output_commit_state(output, &state);
  wlr_output_state_finish(&state);

  struct sonde_output *sonde_output = calloc(1, sizeof(*sonde_output));
  sonde_output->output = output;
  sonde_output->server = server;

  sonde_output->frame.notify = on_output_frame;
  sonde_output->request_state.notify = on_output_request_state;
  sonde_output->destroy.notify = on_output_destroy;

  wl_signal_add(&output->events.frame, &sonde_output->frame);
  wl_signal_add(&output->events.request_state, &sonde_output->request_state);
  wl_signal_add(&output->events.destroy, &sonde_output->destroy);

  wl_list_insert(&server->outputs, &sonde_output->link);

  // add to layout
  struct wlr_output_layout_output *l_output = wlr_output_layout_add_auto(server->output_layout, output);
  struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene, output);
  wlr_scene_output_layout_add_output(server->scene_layout, l_output, scene_output);
}

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

int start_server(struct sonde_server *server) {
  server->display = wl_display_create();

  // backend - abstracts input/output - allows running in existing X/Wayland sessiosn
  server->backend = wlr_backend_autocreate(wl_display_get_event_loop(server->display), NULL);
  if (server->backend == NULL) {
    wlr_log(WLR_ERROR, "failed to create backend");
    return 1;
  }

  server->renderer = wlr_renderer_autocreate(server->backend);
  if (server->renderer == NULL) {
    wlr_log(WLR_ERROR, "failed to create renderer");
    return 1;
  }

  wlr_renderer_init_wl_display(server->renderer, server->display);

  // allocator - handles screen buffers
  server->allocator = wlr_allocator_autocreate(server->backend, server->renderer);
  if (server->allocator == NULL) {
    wlr_log(WLR_ERROR, "failed to create allocator");
    return 1;
  }

  wlr_compositor_create(server->display, /* version */ 5, server->renderer);
  wlr_subcompositor_create(server->display);
  wlr_data_device_manager_create(server->display);

  // handles screen arrangement
  server->output_layout = wlr_output_layout_create(server->display);

  // outputs
  wl_list_init(&server->outputs);
  server->new_output.notify = on_new_output;
  wl_signal_add(&server->backend->events.new_output, &server->new_output);

  server->scene = wlr_scene_create();
  server->scene_layout = wlr_scene_attach_output_layout(server->scene, server->output_layout);

  // XDG Shell
  wl_list_init(&server->toplevels);
  server->xdg_shell = wlr_xdg_shell_create(server->display, 3);
  
  server->new_toplevel.notify = on_new_toplevel;
  server->new_popup.notify = on_new_popup;

  wl_signal_add(&server->xdg_shell->events.new_toplevel, &server->new_toplevel);
  wl_signal_add(&server->xdg_shell->events.new_popup, &server->new_popup);


  // Seat
  wl_list_init(&server->keyboards);
  server->new_input.notify = on_new_input;
  server->seat = wlr_seat_create(server->display, "seat0");
  server->request_cursor.notify = on_request_cursor;
  server->request_set_selection.notify = on_request_set_selection;

  wl_signal_add(&server->backend->events.new_input, &server->new_input);
  wl_signal_add(&server->seat->events.request_set_cursor, &server->request_cursor);
  wl_signal_add(&server->seat->events.request_set_selection, &server->request_set_selection);

  server->socket = wl_display_add_socket_auto(server->display);
  if (server->socket == NULL) {
    wlr_log(WLR_ERROR, "failed to create Wayland socket");
    return 1;
  }

  setenv("WAYLAND_DISPLAY", server->socket, true);

  // start!
  if (!wlr_backend_start(server->backend)) {
    wlr_log(WLR_ERROR, "failed to start backend");
    return 1;
  }

  return 0;
}

void stop_server(struct sonde_server *server) {
  wlr_output_layout_destroy(server->output_layout);
  wlr_allocator_destroy(server->allocator);
  wlr_renderer_destroy(server->renderer);
  wlr_backend_destroy(server->backend);
  wl_display_destroy(server->display);
}
int main() {
  // TODO: customizable log levels
  wlr_log_init(WLR_DEBUG, NULL);

  
  wlr_log(WLR_INFO, "Hi wayland");
  struct sonde_server server = {0};
  if (start_server(&server) != 0) {
    // failed to create server
    stop_server(&server);
    return 1;
  }

  // start a shell
  if (fork() == 0) {
    execl("/bin/sh", "/bin/sh", NULL);
  }
  
  wlr_log(WLR_INFO, "Server has been started on WAYLAND_DISPLAY=%s", server.socket);
  wl_display_run(server.display);

  // stop
  stop_server(&server);
  wlr_log(WLR_INFO, "Server has been stopped");

  return 0;
}
