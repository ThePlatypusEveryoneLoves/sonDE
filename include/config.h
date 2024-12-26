#pragma once

#include "common.h"

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
  struct sonde_screen_config* screens;
  uint32_t num_screens;

  struct sonde_keyboard_config *keyboards;
  uint32_t num_keyboards;
};


/// initialize a sonde_config struct to defaults
void sonde_config_init_defaults(struct sonde_config *config);

#define SONDE_CONFIG_FIND_DEVICE(CONFIG, ARRAY_NAME, DEVICE_NAME, DEVICE_TYPE, OUT_NAME) \
  struct DEVICE_TYPE *OUT_NAME = NULL;                                  \
  for (uint32_t i = 0; i < (CONFIG)->num_ ## ARRAY_NAME; i++) {         \
    if (strcmp((DEVICE_NAME), (CONFIG)->ARRAY_NAME[i].name) == 0) {     \
      OUT_NAME = &(CONFIG)->ARRAY_NAME[i];                              \
    }                                                                   \
  }
