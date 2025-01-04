#pragma once

#include "common.h"
#include "array.h"

#define SONDE_KEYBIND_NUM_MODIFIERS 4

/// ctrl alt shift super
typedef uint8_t sonde_keybind_modifiers_t;
typedef xkb_keysym_t sonde_keybind_key_t;

enum sonde_keybind_command_type {
  SONDE_KEYBIND_COMMAND_EXIT,
  SONDE_KEYBIND_COMMAND_LAUNCH, // data is char*
  SONDE_KEYBIND_COMMAND_NEXT_VIEW,
  SONDE_KEYBIND_COMMAND_PREV_VIEW,
};

struct sonde_keybind_command {
  enum sonde_keybind_command_type type;
  void *data;
};

struct sonde_keybind_specifier {
  sonde_keybind_modifiers_t modifiers;
  sonde_keybind_key_t key;
};

struct sonde_keybind {
  sonde_keybind_key_t key;
  struct sonde_keybind_command command;
};

/// Keybinds are stored as a "hashmap" of modifiers to array of sonde_keybind
/// 
/// Because there are only 4 modifiers, the hashmap is really just a `1 << 4 = 16` length array
typedef ARRAY(struct sonde_keybind)
    sonde_keybinds_t[1 << SONDE_KEYBIND_NUM_MODIFIERS];

/// Clear and free all keybinds
void sonde_keybinds_clear(sonde_keybinds_t keybinds);

/// Find a command given a list of keybindings and a keypress
///
/// Returns a pointer to the command or NULL
struct sonde_keybind_command *sonde_keybind_find(sonde_keybinds_t keybinds, uint32_t wlr_modifiers, xkb_keysym_t key);
