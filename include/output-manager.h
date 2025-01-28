#pragma once

#include "common.h"
#include "server.h"

/// Update the output config and send info to clients
void sonde_output_manager_update(sonde_server_t server);
int sonde_output_manager_initialize(sonde_server_t server);
void sonde_output_manager_destroy(sonde_server_t server);
