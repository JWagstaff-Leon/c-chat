#pragma once

#include "connections.h"

typedef struct {
    connections_t *connections;
} server_t;

enum server_init_result {
    SINIT_SOCKET_FAILED     = -5,
    SINIT_OPTIONS_FAILED    = -4,
    SINIT_BIND_FAILED       = -3,
    SINIT_LISTEN_FAILED     = -2,
    SINIT_ALLOCATION_FAILED = -1,
    SINIT_SUCCESS           =  0
};

server_init_result server_initialize(server_t *out_server);
void server_update_status(server_t *server);
void server_cleanup(server_t *server);