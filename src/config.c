#include "config.h"

void sonde_config_init_defaults(struct sonde_config *config) {
  // im paranoid
  config->screens = NULL;
  config->num_screens = 0;

  config->keyboards = NULL;
  config->num_keyboards = 0;
}
