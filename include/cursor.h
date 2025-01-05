#pragma once

#include "common.h"
#include "server.h"

struct sonde_pointer_constraint {
  sonde_server_t server;
  struct wlr_pointer_constraint_v1 *pointer_constraint;

  // events
  struct wl_listener destroy;
};

int sonde_cursor_initialize(sonde_server_t server);
void sonde_cursor_destroy(sonde_server_t server);
void sonde_cursor_set_pointer_constraint(
    sonde_server_t server,
    struct wlr_pointer_constraint_v1 *pointer_constraint);
