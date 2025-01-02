#include "config.h"
#include "user_config.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <unistd.h>

/// Gets a value from a Lua table (at TABLE_POS). If the type is not VALUE_TYPE,
/// returns DEFAULT
/// Uses VALUE_FN to coerce the value
/// -0 +1
#define LUA_TABLE_TRY_GET(LUA_STATE, TABLE_POS, KEY, VALUE_TYPE, VALUE_FN,     \
                          DEFAULT)                                             \
  lua_getfield((LUA_STATE), (TABLE_POS), (KEY)) == (VALUE_TYPE)                \
      ? VALUE_FN((LUA_STATE), -1)                                              \
      : (DEFAULT)

/// Convenience macros
#define LUA_TABLE_TRY_GET_INT(LUA_STATE, TABLE_POS, KEY)                       \
  LUA_TABLE_TRY_GET(LUA_STATE, TABLE_POS, KEY, LUA_TNUMBER, lua_tointeger, -1)

#define LUA_TABLE_TRY_GET_FLOAT(LUA_STATE, TABLE_POS, KEY)                     \
  LUA_TABLE_TRY_GET(LUA_STATE, TABLE_POS, KEY, LUA_TNUMBER, lua_tonumber, -1)

#define LUA_TABLE_TRY_GET_STRING(LUA_STATE, TABLE_POS, KEY) \
  LUA_TABLE_TRY_GET(LUA_STATE, TABLE_POS, KEY, LUA_TSTRING, lua_tostring, NULL)

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
    // we own these
    free((char *)keyboard_config->inner.keymap.layout);
    free((char *)keyboard_config->inner.keymap.model);
    free((char *)keyboard_config->inner.keymap.rules);
    free((char *)keyboard_config->inner.keymap.options);
    free((char *)keyboard_config->inner.keymap.variant);
  }

  // free arrays
  ARRAY_CLEAR(&config->screens);
  ARRAY_CLEAR(&config->keyboards);
}

int sonde_config_initialize(struct sonde_config *config) {
  ARRAY_CLEAR(&config->screens);
  ARRAY_CLEAR(&config->keyboards);

  // get possible conf files
  char* xdg_config_home = getenv("XDG_CONFIG_HOME");
  char* xdg_config_dirs = getenv("XDG_CONFIG_DIRS");
  char* home = getenv("HOME");

  char* system_config = NULL;
  char* user_config = NULL;

  const char sonde_config_path[] = "%s/sonde/config.lua";

  if (xdg_config_dirs == NULL) {
    // default to /etc/xdg
    system_config = strdup("/etc/xdg/sonde/config.lua");
  } else {
    int len = snprintf(NULL, 0, sonde_config_path, xdg_config_dirs);
    system_config = malloc(len + 1);
    sprintf(system_config, sonde_config_path, xdg_config_dirs);
  }

  if (xdg_config_home == NULL) {
    // default to $HOME/.config
    int len = snprintf(NULL, 0, "%s/.config/sonde/config.lua", home);
    user_config = malloc(len + 1);
    sprintf(user_config, "%s/.config/sonde/config.lua", home);
  } else {
    int len = snprintf(NULL, 0, sonde_config_path, xdg_config_home);
    user_config = malloc(len + 1);
    sprintf(user_config, sonde_config_path, xdg_config_home);
  }

  // user overrides system
  config->conf_files[0] = system_config;
  config->conf_files[1] = user_config;

  // init lua
  config->lua_state = luaL_newstate();
  if (config->lua_state == NULL) {
    wlr_log(WLR_ERROR, "failed to initialize lua");
  }
  luaL_openlibs(config->lua_state);

  return 0;
}

static void update_or_insert_screen(struct sonde_config *config, const char *screen_name, uint32_t width, uint32_t height, float_t refresh_rate) {
  struct sonde_screen_config *item = NULL;
  ARRAY_FOREACH(&config->screens, item) {
    if (strcmp(item->name, screen_name) == 0) {
      // update this item
      item->width = width;
      item->height = height;
      item->refresh_rate = refresh_rate;
      return;
    }
  }
  // insert a new item
  struct sonde_screen_config new_item = {
    .width = width,
    .height = height,
    .refresh_rate = refresh_rate,
    .name = strdup(screen_name) // we don't own the strings lua gives us
  };
  ARRAY_APPEND(&config->screens, new_item);
}

static void update_or_insert_keyboard(struct sonde_config *config, const char *keyboard_name, struct sonde_keyboard_config_inner kb_config) {
  struct sonde_keyboard_config *item = NULL;
  ARRAY_FOREACH(&config->keyboards, item) {
    if (strcmp(item->name, keyboard_name) == 0) {
      // update this item

      if (kb_config.repeat_rate != -1)
        item->inner.repeat_rate = kb_config.repeat_rate;
      if (kb_config.repeat_delay != -1)
        item->inner.repeat_delay = kb_config.repeat_delay;

      // cast from const char ** to char ** is safe because we always own anything in item.keymap
      str_replace((char **)&item->inner.keymap.rules, kb_config.keymap.rules);
      str_replace((char **)&item->inner.keymap.model, kb_config.keymap.model);
      str_replace((char **)&item->inner.keymap.layout, kb_config.keymap.layout);
      str_replace((char **)&item->inner.keymap.variant, kb_config.keymap.variant);
      str_replace((char **)&item->inner.keymap.options, kb_config.keymap.options);
      
      return;
    }
  }
  // insert a new item
  struct sonde_keyboard_config new_item = {
    .name = strdup(keyboard_name), // we don't own the strings lua gives us
    .inner = {
      .repeat_rate = kb_config.repeat_rate == -1 ? SONDE_KEYBOARD_DEFAULT_REPEAT_RATE : kb_config.repeat_rate,
      .repeat_delay = kb_config.repeat_delay == -1 ? SONDE_KEYBOARD_DEFAULT_REPEAT_DELAY : kb_config.repeat_delay,
      .keymap = {
        .rules = STRDUP(kb_config.keymap.rules),
        .model = STRDUP(kb_config.keymap.model),
        .layout = STRDUP(kb_config.keymap.layout),
        .variant = STRDUP(kb_config.keymap.variant),
        .options = STRDUP(kb_config.keymap.options)
      }
    }
  };
  ARRAY_APPEND(&config->keyboards, new_item);
}

static int sonde_config_lua_exec(struct sonde_config *config,
                                 const char *filename) {
  if (luaL_loadfile(config->lua_state, filename)) {
    // error mesage at top of stack
    wlr_log(WLR_ERROR, "could not open lua config %s: %s", filename, lua_tostring(config->lua_state, -1));
    return 1;
  }

  // define the sonde global
  lua_createtable(config->lua_state, 0, 2);

  // create screens table. if we have existing screens, preallocate with that
  lua_createtable(config->lua_state, 0, config->screens.length);
  lua_setfield(config->lua_state, -2, "screens"); // table is at -2

  // keyboard
  lua_createtable(config->lua_state, 0, config->keyboards.length);
  lua_setfield(config->lua_state, -2, "keyboards"); // table is at -2

  lua_setglobal(config->lua_state, "sonde"); // table = "sonde"

  // run
  if (lua_pcall(config->lua_state, 0, LUA_MULTRET, 0)) {
    wlr_log(WLR_ERROR, "failed to execute lua config %s: %s", filename, lua_tostring(config->lua_state, -1));
    lua_settop(config->lua_state, 0);
    return 1;
  }

  // retrieve configured dict
  if (lua_getglobal(config->lua_state, "sonde") != LUA_TTABLE) {
    wlr_log(WLR_ERROR, "lua config %s has changed type of sonde dict", filename);
    lua_settop(config->lua_state, 0);
    return 1;
  }

  // push screens onto the stack
  if (lua_getfield(config->lua_state, -1, "screens") != LUA_TTABLE) {
    wlr_log(WLR_ERROR, "lua config %s: invalid screens type", filename);
    lua_settop(config->lua_state, 0);
    return 1;
  }

  // iterate over screens table
  lua_pushnil(config->lua_state); // "initial value" of the key     -0 +1 = 1
  while (lua_next(config->lua_state, -2)) { // table is at -2       -1 +2 = 2
    // lua_next pushes a key and value onto the stack

    int width_t = lua_getfield(config->lua_state, -1, "width");  // -0 +1 = 3
    int height_t = lua_getfield(config->lua_state, -2, "height");// -0 +1 = 4
    int refresh_rate_t = lua_getfield(config->lua_state, -3, "refresh_rate"); // -0 +1 = 5

    // low effort way to make sure they're all nums
    if (width_t == height_t && height_t == refresh_rate_t && width_t == LUA_TNUMBER) {
      update_or_insert_screen(config,
                              lua_tostring(config->lua_state, -5),
                              lua_tointeger(config->lua_state, -3),
                              lua_tointeger(config->lua_state, -2),
                              lua_tonumber(config->lua_state, -1)
                              );
    } // else ignore

    // pop the value off, as well as width, height, ad refresh rate, leave the key
    lua_pop(config->lua_state, 4); //                               -4 +0 = 1
  }

  // pop screens
  lua_pop(config->lua_state, 1);

  // push keyboards onto the stack
  if (lua_getfield(config->lua_state, -1, "keyboards") != LUA_TTABLE) {
    wlr_log(WLR_ERROR, "lua config %s: invalid keyboards type", filename);
    lua_settop(config->lua_state, 0);
    return 1;
  }

  // iterate over keyboards table
  lua_pushnil(config->lua_state); // "initial value" of the key     -0 +1 = 1
  while (lua_next(config->lua_state, -2)) { // table is at -2       -1 +2 = 2
    // lua_next pushes a key and value onto the stack

    const char *name = lua_tostring(config->lua_state, -2);

    uint32_t rep_delay = LUA_TABLE_TRY_GET_INT(config->lua_state, -1, "repeat_delay"); // 3
    uint32_t rep_rate = LUA_TABLE_TRY_GET_INT(config->lua_state, -2, "repeat_rate"); // 4
    const char *xkb_rules = LUA_TABLE_TRY_GET_STRING(config->lua_state, -3, "xkb_rules"); // 5
    const char *xkb_model = LUA_TABLE_TRY_GET_STRING(config->lua_state, -4, "xkb_model"); // 6
    const char *xkb_layout = LUA_TABLE_TRY_GET_STRING(config->lua_state, -5, "xkb_layout"); // 7
    const char *xkb_variant = LUA_TABLE_TRY_GET_STRING(config->lua_state, -6, "xkb_variant"); // 8
    const char *xkb_options = LUA_TABLE_TRY_GET_STRING(config->lua_state, -7, "xkb_options"); // 9

    struct sonde_keyboard_config_inner kb_config = {
      .repeat_delay = rep_delay,
      .repeat_rate = rep_rate,
      .keymap = {
        .rules = xkb_rules,
        .model = xkb_model,
        .layout = xkb_layout,
        .variant = xkb_variant,
        .options = xkb_options
      }
    };

    update_or_insert_keyboard(config, name, kb_config);

    // pop the value off, as well as width, height, ad refresh rate, leave the key
    lua_pop(config->lua_state, 8); //                               -8 +0 = 1
  }

  // clear the stack
  lua_settop(config->lua_state, 0);

  return 0;
}

int sonde_config_reload(struct sonde_config *config) {
  sonde_config_reset(config);

  // TODO: apply compile time config here (user-config.h)
  
  // apply config, in order
  for (int i = 0; i < sizeof(config->conf_files) / sizeof(char*); i++) {
    if (access(config->conf_files[i], F_OK) == 0) {
      wlr_log(WLR_DEBUG, "executing lua config: %s", config->conf_files[i]);
      // if the file exists, execute it
      if (sonde_config_lua_exec(config, config->conf_files[i])) return 1;
    }
  }

  return 0;
}

void sonde_config_destroy(struct sonde_config *config) {
  sonde_config_reset(config);

  // free paths
  free(config->conf_files[0]);
  free(config->conf_files[1]);

  lua_close(config->lua_state);
}
