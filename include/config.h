#include "common.h"
#include <stdint.h>


struct sonde_screen {
  uint32_t width, height;
  uint32_t refresh_rate;
  char* name;
};
struct sonde_config {
  struct sonde_screen* screens;
  uint32_t screencount;
};


