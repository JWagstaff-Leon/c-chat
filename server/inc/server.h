#pragma once

#define DEFAULT_SERVER_SIZE 8
#define MAX_CONTENT_LENGTH 1024

#include "connections.h"

typedef struct {
    connections_t *connections;

    FILE *standard_output;
    FILE *error_output;
} server_t;



static unsigned char new_connection_fail_message[] = "Server is unable to handle new connections at the moment.";
static event_t new_connection_fail_event = {
    .code = EVENT_CONNECTION_FAILED,
    .originator_id = 0,
    .content_length = sizeof(new_connection_fail_message)
};



server_t *server_init(FILE *standard_output, FILE *error_output);
void server_destroy(server_t *server);

void server_send_global_server_event(server_t *server, event_t *event);
void server_send_server_event_to(server_t *server, event_t *event, int reciever_fd);

void server_do_tick(server_t *server); // TODO make this method in the c file with all the underneath methods
    void server_start_new_tick(server_t *server);
    // TODO make method for unintroduced users
    void server_handle_users_input(server_t *server);
    void server_check_new_users(server_t *server);