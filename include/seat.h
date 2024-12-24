#pragma once

#include "common.h"
#include "server.h"

struct sonde_keyboard {
  struct wl_list link;
  
  struct sonde_server *server;
  struct wlr_keyboard *keyboard;
  
  struct wl_listener modifiers;
  struct wl_listener key;
  struct wl_listener destroy;
};

int sonde_seat_initialize(sonde_server_t server);
void sonde_seat_destroy(sonde_server_t server);
