#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "connections.h"
#include "event.h"
#include "messages.h"
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



int main(int argc, const char *argv[])
{
    set_signal_handler();

    server_t *server = server_init(stdout, stderr);
    assert(server != NULL);
    printf("Server ready;\nWaiting for connections...\n");

    while(!exiting)
    {
        server_start_new_tick(server);
        
        for(int sender = 1; sender < connections.size; sender++)
        {
#pragma region check unintroduced users
            if(connections.users[sender].state == USER_CONNECTED && connections.users[sender].pollfd.revents & POLLOUT)
            {
                send(connections.users[sender].pollfd.fd, &username_request_event, sizeof(event_t), 0);
                send(connections.users[sender].pollfd.fd, &username_request_message, sizeof(username_request_message), 0);
                connections.users[sender].state = USER_NO_USERNAME;
            }
#pragma endregion

#pragma region handle users input
            
#pragma endregion
        server_check_new_users(server);
    }

    printf("\nShutting down\n");
    server_destory(server);
    shutdown(master_socket, SHUT_RDWR);
    close(master_socket);
    return 0;
};