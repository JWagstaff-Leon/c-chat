// #include <arpa/inet.h>
#include <assert.h>
// #include <errno.h>
// #include <netinet/in.h>
// #include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <unistd.h>

// #include "connections.h"
// #include "event.h"
// #include "messages.h"
#include "server.h"

static bool exiting = false;



static void handle_signal(int signal_type)
{
    exiting = true;
};



static void set_signal_handler(void)
{
    struct sigaction signal_action = {.sa_handler = &handle_signal, .sa_flags = 0};
    sigemptyset(&signal_action.sa_mask);
    sigaction(SIGHUP, &signal_action, NULL);
    sigaction(SIGINT, &signal_action, NULL);
    sigaction(SIGABRT, &signal_action, NULL);
    sigaction(SIGTERM, &signal_action, NULL);
};



int main(int argc, const char *argv[])
{
    set_signal_handler();

    server_t *server = server_init(stdout, stderr);
    assert(server != NULL);

    while(!exiting)
        server_do_tick(server);

    server_destroy(server);
    return 0;
};