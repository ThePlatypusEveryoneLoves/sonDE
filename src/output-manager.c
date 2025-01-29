#include "output-manager.h"
#include "outputs.h"


void sonde_output_manager_update(sonde_server_t server) {
  struct wlr_output_configuration_v1 *output_config = wlr_output_configuration_v1_create();


  // add each output to the config
  struct sonde_output *output;
  wl_list_for_each(output, &server->outputs, link) {
    wlr_output_configuration_head_v1_create(output_config, output->output);
  }

  // update
  wlr_output_manager_v1_set_configuration(server->wlr_output_manager, output_config);
}

int sonde_output_manager_initialize(sonde_server_t server) {
  server->wlr_output_manager = wlr_output_manager_v1_create(server->display);
  if (server->wlr_output_manager == NULL) {
    wlr_log(WLR_ERROR, "failed to create wlr output manager");
    return 1;
  }

  server->xdg_output_manager = wlr_xdg_output_manager_v1_create(server->display, server->output_layout);
  if (server->xdg_output_manager == NULL) {
    wlr_log(WLR_ERROR, "failed to create xdg output manager");
    return 1;
  }
  
  // TODO: actually implement the client config part of output manager
  
  return 0;
}

void sonde_output_manager_destroy(sonde_server_t server) {
  // nothing to do here for now
}
