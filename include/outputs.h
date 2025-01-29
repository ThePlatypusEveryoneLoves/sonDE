#pragma once

#include "common.h"
#include "server.h"
#include "config.h"

struct sonde_output {
  struct wl_list link;
  struct wlr_output *output;
  struct wlr_output_layout_output *output_layout;
  
  sonde_server_t server;

  // for layer-shell
  struct {
    int32_t top, bottom;
  } exclusive_margin;
  
  struct wl_listener frame;
  struct wl_listener request_state;
  struct wl_listener destroy;
};

int sonde_outputs_initialize(sonde_server_t server);
void sonde_outputs_destroy(sonde_server_t server);

struct wlr_output_mode* sonde_get_output_or_preferred(struct wlr_output* output, struct sonde_config *config);
struct sonde_output *sonde_get_output_at_cursor(sonde_server_t server);
struct sonde_output *sonde_get_default_output(sonde_server_t server);
