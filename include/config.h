#pragma once

#include "user_config.h"
#include "common.h"
#include "array.h"
#include <lua.h>

struct sonde_screen_config {
  uint32_t width, height;
  float_t refresh_rate;
  char* name;
};

struct sonde_keyboard_config_inner {
  uint32_t repeat_delay; // ms
  uint32_t repeat_rate; // hertz
  struct xkb_rule_names keymap;
};

struct sonde_keyboard_config {
  char* name;
  struct sonde_keyboard_config_inner inner;
};

struct sonde_config {
  ARRAY(struct sonde_screen_config) screens;
  ARRAY(struct sonde_keyboard_config) keyboards;

  lua_State *lua_state;
  // possible config.lua files, based on the XDG Base dir spec
  char *conf_files[2];
};


/// initialize a sonde_config struct to defaults, and instantiate lua
int sonde_config_initialize(struct sonde_config *config);

/// reload dynamic lua config
int sonde_config_reload(struct sonde_config *config);

/// free config and close lua
void sonde_config_destroy(struct sonde_config *config);
