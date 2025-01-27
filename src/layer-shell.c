#include "layer-shell.h"

WL_CALLBACK(on_new_surface) {
  
}

int sonde_layer_shell_initialize(sonde_server_t server) {
  server->layer_shell = wlr_layer_shell_v1_create(server->display, 5);
  if (server->layer_shell == NULL) {
    wlr_log(WLR_ERROR, "failed to create wlr layer shell");
    return 1;
  }

  LISTEN(&server->layer_shell->events.new_surface, &server->new_layer_shell_surface, on_new_surface);

  return 0;
}
