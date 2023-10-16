#pragma once

#include <poll.h>
#include <stddef.h>

enum user_state {
    USER_UNINITIALIZED = 0,
    USER_CONNECTED,
    USER_NO_USERNAME,
    USER_ACTIVE
};



typedef struct {
    unsigned char *username;
    struct pollfd pollfd;
    enum user_state state;
} user_t;



static const user_t blank_user = {.username = NULL, .pollfd = {.fd = -1, .events = 0, .revents = 0}, .state = USER_UNINITIALIZED};