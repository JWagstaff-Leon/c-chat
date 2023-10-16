#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "event.h"
#include "user.h"

#define GROW_FACTOR 1.8

typedef struct {
    user_t *users;
    size_t count;
    size_t size;
} connections_t;



bool connections_update_fds(connections_t *connections);
void connections_relay_event_from(const connections_t *connections, event_t *event, int sender);
void connections_relay_message_from(const connections_t *connections, char *message, int sender);
int connections_add_connection(connections_t *connections, int new_connection, size_t username_size);
void connections_close_connection(connections_t *connections, unsigned int index);
void connections_shutdown(connections_t *connections);