#pragma once

#include <stdio.h>

#include "connections.h"
#include "event.h"

#define DEFAULT_SERVER_SIZE 8
#define MAX_CONTENT_LENGTH 1024

typedef struct {
    connections_t *connections;

    FILE *standard_output;
    FILE *error_output;
} server_t;



static unsigned char new_connection_fail_message[] = "Server is unable to handle new connections at the moment";
static event_t new_connection_fail_event = {
    .code = EVENT_CONNECTION_FAILED,
    .originator_id = 0,
    .content_length = sizeof(new_connection_fail_message)
};



static unsigned char username_request_message[] = "Enter username to begin chatting";
static event_t username_request_event = {
    .code = EVENT_USERNAME_REQUEST,
    .originator_id = 0,
    .content_length = sizeof(username_request_message)
};



static unsigned char username_accepted_message[] = "Username set";
static event_t username_accepted_event = {
    .code = EVENT_USERNAME_ACCEPTED,
    .originator_id = 0,
    .content_length = sizeof(username_accepted_message)
};



static event_t oversized_content_event = {
    .code = EVENT_OVERSIZED_CONTENT,
    .originator_id = 0,
    .content_length = 0
};



static unsigned char shutdown_message[] = "Server is shutting down";
static event_t shutdown_event = {
    .code = EVENT_SERVER_SHUTDOWN,
    .originator_id = 0,
    .content_length = sizeof(shutdown_message)
};




server_t *server_init(FILE *standard_output, FILE *error_output);
void server_destroy(server_t *server);

void server_send_global_server_event(server_t *server, event_t *event);
void server_send_global_server_event_with_content(server_t *server, event_t *event, unsigned char event_content[]);
void server_send_server_event_to(server_t *server, event_t *event, int reciever_fd);
void server_send_server_event_with_content_to(server_t *server, event_t *event, unsigned char event_content[], int reciever_fd);

void server_do_tick(server_t *server);
    void server_start_new_tick(server_t *server);
    void server_introduce_users(server_t *server);
    void server_handle_users_input(server_t *server);
    void server_check_new_users(server_t *server);