#pragma once

#include <poll.h>
#include <stdbool.h>
#include <stdlib.h>

#include "event.h"
#include "user.h"

#define GROW_FACTOR 1.8

typedef struct {
    user_t *users;
    struct pollfd *pollfds; // convenience to avoid extra allocs; this is only used temporarily so as to keep the source of truth in a users[x].pollfd
    size_t count;
    size_t size;
} connections_t;



connections_t *connections_init(size_t size);
void connections_destroy(connections_t *connections);

void connections_update(connections_t *connections);

bool connections_is_connection_readable(connections_t *connections, int connection);
bool connections_is_connection_writeable(connections_t *connections, int connection);

enum user_state connections_get_user_state(connections_t *connections, int user);
void connections_set_user_state(connections_t *connections, int user, enum user_state state);

void connections_send_event_to_fd(connections_t *connections, event_t *event, int reciever_fd);
void connections_send_event_with_content_to_fd(connections_t *connections, event_t *event, unsigned char event_content[], int reciever_fd);
void connections_send_event_to(connections_t *connections, event_t *event, int reciever);
void connections_send_event_with_content_to(connections_t *connections, event_t *event, unsigned char event_content[], int reciever);

void connections_relay_event_from(connections_t *connections, event_t *event, int sender);
void connections_relay_event_with_content_from(connections_t *connections, event_t *event, unsigned char event_content[], int sender);
void connections_relay_message_from(connections_t *connections, unsigned char *message, int sender);

int connections_add_connection(connections_t *connections, int new_connection);
void connections_close_connection(connections_t *connections, unsigned int index);