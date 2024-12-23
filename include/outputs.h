#pragma once

#include "common.h"
#include "server.h"

struct sonde_output {
  struct wl_list link;
  struct wlr_output *output;
  sonde_server_t server;
  
  struct wl_listener frame;
  struct wl_listener request_state;
  struct wl_listener destroy;
};

int sonde_outputs_initialize(sonde_server_t server);
void sonde_outputs_destroy(sonde_server_t server);
