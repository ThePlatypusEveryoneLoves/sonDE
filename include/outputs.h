#pragma once

#include "common.h"
#include "server.h"

struct sonde_output {
  struct wl_list link;
  struct wlr_output *output;
  struct sonde_server *server;
  
  struct wl_listener frame;
  struct wl_listener request_state;
  struct wl_listener destroy;
};

int sonde_outputs_initialize(struct sonde_server *server);
void sonde_outputs_destroy(struct sonde_server *server);
