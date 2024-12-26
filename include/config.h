#pragma once

#include "common.h"
#include "array.h"
#include <lua.h>

struct sonde_screen_config {
  uint32_t width, height;
  uint32_t refresh_rate;
  char* name;
};

struct sonde_keyboard_config {
  char* name;
  uint32_t repeat_delay; // ms
  uint32_t repeat_rate; // hertz
  struct xkb_rule_names keymap;
};

struct sonde_config {
  ARRAY(struct sonde_screen_config) screens;
  ARRAY(struct sonde_keyboard_config) keyboards;

  lua_State *lua_state;
};


/// initialize a sonde_config struct to defaults, and instantiate lua
void sonde_config_init(struct sonde_config *config);

/// free config and close lua
void sonde_config_destroy(struct sonde_config *config);
