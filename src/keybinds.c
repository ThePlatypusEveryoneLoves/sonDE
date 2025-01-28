#include "keybinds.h"
#include "array.h"

void sonde_keybinds_clear(sonde_keybinds_t keybinds) {
  for (int i = 0; i < (1 << SONDE_KEYBIND_NUM_MODIFIERS); i++) {
    struct sonde_keybind *item;
    // free all aux data
    ARRAY_FOREACH(&keybinds[i], item) {
      free(item->command.data);
    }
    ARRAY_CLEAR(&keybinds[i]);
  }
}

struct sonde_keybind_command *sonde_keybind_find(sonde_keybinds_t keybinds, uint32_t wlr_modifiers, xkb_keysym_t key) {
  // convert the 32 bit wlr_modifiers into our simpler 4 bit
  bool ctrl = (wlr_modifiers & WLR_MODIFIER_CTRL) != 0;
  bool alt = (wlr_modifiers & WLR_MODIFIER_ALT) != 0;
  bool shift = (wlr_modifiers & WLR_MODIFIER_SHIFT) != 0;
  bool super = (wlr_modifiers & WLR_MODIFIER_LOGO) != 0;
  uint8_t modifiers = (ctrl << 3) | (alt << 2) | (shift << 1) | (super << 0);

  // convert uppercase keys into lowercase keys
  if (key >= 'A' && key <= 'Z') key += 'a' - 'A';
  
  struct sonde_keybind *item;
  // lookup the array of keybinds in the modifiers "hashmap"
  ARRAY_FOREACH_REVERSE(&keybinds[modifiers], item) {
    // return the last one that matches
    if (item->key == key) {
      return &item->command;
    }
  }

  return NULL;
}
