#include "server.h"
#include "cursor.h"
#include "layer-shell.h"
#include "outputs.h"
#include "seat.h"
#include "config.h"
#include "wlr/util/log.h"
#include "xdg-shell.h"
#include "decoration-manager.h"
#include "output-manager.h"
#include <unistd.h>

int sonde_server_create(sonde_server_t server) {
  if (sonde_config_initialize(&server->config) != 0) {
    wlr_log(WLR_ERROR, "failed to initialize config");
    return 1;
  }

  if (sonde_config_reload(&server->config) != 0) {
    wlr_log(WLR_ERROR, "failed to execute lua config; continuing with setup");
    //return 1;
  }
  
  server->display = wl_display_create();
  server->event_loop = wl_display_get_event_loop(server->display);
  // backend - abstracts input/output - allows running in existing X/Wayland sessiosn
  server->backend = wlr_backend_autocreate(server->event_loop, NULL);
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

  if (sonde_outputs_initialize(server) != 0) {
    wlr_log(WLR_ERROR, "Failed to initialize outputs");
    return 1;
  }

  if (sonde_xdg_shell_initialize(server) != 0) {
    wlr_log(WLR_ERROR, "Failed to initialize xdg_shell");
    return 1;
  }

  if (sonde_layer_shell_initialize(server) != 0) {
    wlr_log(WLR_ERROR, "Failed to initialize layer_shell");
    return 1;
  }

  if (sonde_output_manager_initialize(server) != 0) {
    wlr_log(WLR_ERROR, "Failed to initialize output_manager");
    return 1;
  }

  if (sonde_decoration_manager_initialize(server) != 0) {
    wlr_log(WLR_ERROR, "failed to initialize XDG decoration manager");
    return 1;
  }

  if (sonde_cursor_initialize(server) != 0) {
    wlr_log(WLR_ERROR, "Failed to initialize cursor");
    return 1;
  }

  if (sonde_seat_initialize(server) != 0) {
    wlr_log(WLR_ERROR, "Failed to initialize seat");
    return 1;
  }

  return 0;
}

int sonde_server_start(sonde_server_t server) {
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
  
  wlr_log(WLR_INFO, "Server has been started on WAYLAND_DISPLAY=%s", server->socket);
  wl_display_run(server->display);

  return 0;
}

void sonde_server_destroy(sonde_server_t server) {
  if (server->display != NULL) wl_display_destroy_clients(server->display);
  sonde_layer_shell_destroy(server);
  sonde_decoration_manager_destroy(server);
  sonde_xdg_shell_destroy(server);
  sonde_outputs_destroy(server);
  sonde_cursor_destroy(server);
  wlr_allocator_destroy(server->allocator);
  wlr_renderer_destroy(server->renderer);
  wlr_backend_destroy(server->backend);
  sonde_seat_destroy(server);
  sonde_output_manager_destroy(server);
  if (server->display != NULL) wl_display_destroy(server->display);

  sonde_config_destroy(&server->config);
}
