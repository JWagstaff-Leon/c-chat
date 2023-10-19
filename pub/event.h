#pragma once

#include <stdint.h>

enum event_code {
    EVENT_UNDEFINED = 0,

    EVENT_CONNECTION_FAILED, // The server was unable to complete the connection
    EVENT_OVERSIZED_CONTENT, // The content length is too big

    EVENT_USERNAME_REQUEST,  // The server is requesting the client's username
    EVENT_USERNAME_SUBMIT,   // The client is submitting their username
    EVENT_USERNAME_ACCEPTED, // The server accepted the client's username
    EVENT_USERNAME_REJECTED, // The server rejected the client's username

    EVENT_SERVER_SHUTDOWN,   // The server is shutting down

    EVENT_USER_LIST,         // Server is sending a client a list of active users
    EVENT_USER_JOIN,         // User set username
    EVENT_USER_LEAVE,        // User with username disconnected;
                             //   sent from server is a notify to other users
                             //   sent from client is a notice to the server

    EVENT_MESSAGE,            // The content is a plain-text, ascii message

    MIN = EVENT_USERNAME_REQUEST,
    MAX = EVENT_MESSAGE
};

typedef struct {
    enum event_code code;
    int originator_id;
    size_t content_length;
    unsigned char content[];
} event_t;