#include "config.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


/// reset config to initial state, freeing all arrays
static void sonde_config_reset(struct sonde_config *config) {
  // free all strings
  struct sonde_screen_config *screen_config = NULL;
  ARRAY_FOREACH(&config->screens, screen_config) {
    free(screen_config->name);
  }

  struct sonde_keyboard_config *keyboard_config = NULL;
  ARRAY_FOREACH(&config->keyboards, keyboard_config) {
    free(keyboard_config->name);
  }

  // free arrays
  ARRAY_CLEAR(&config->screens);
  ARRAY_CLEAR(&config->keyboards);
}

void sonde_config_init(struct sonde_config *config) {
  ARRAY_CLEAR(&config->screens);
  ARRAY_CLEAR(&config->keyboards);

  config->lua_state = luaL_newstate();
  luaL_openlibs(config->lua_state);
}

void sonde_config_destroy(struct sonde_config *config) {
  sonde_config_reset(config);

  lua_close(config->lua_state);
}
