#include <bits/time.h>
#include <time.h>
#include "server.h"
#include "common.h"
#include "util.h"

int main() {
  // TODO: customizable log levels
  INIT_LOG()

  struct sonde_server server = {0};
  if (sonde_server_create(&server) != 0) {
    // failed to create server
    sonde_server_destroy(&server);
    return 1;
  }

  if (sonde_server_start(&server) != 0) {
    // failed to start server
    sonde_server_destroy(&server);
    return 1;
  }

  // stop
  sonde_server_destroy(&server);

  return 0;
}
