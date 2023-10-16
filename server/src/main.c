#include <arpa/inet.h>
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

static bool exiting = false;



static void handle_signal(int signal_type)
{
    exiting = true;
};



int main(int argc, const char *argv[])
{
    struct sigaction signal_action = {.sa_handler = &handle_signal, .sa_flags = 0};
    sigemptyset(&signal_action.sa_mask);
    sigaction(SIGHUP, &signal_action, NULL);
    sigaction(SIGINT, &signal_action, NULL);
    sigaction(SIGABRT, &signal_action, NULL);
    sigaction(SIGTERM, &signal_action, NULL);

    int master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(master_socket < 0)
    {
        fprintf(stderr, "Unable to create socket;\n\t%s\n", strerror(errno));
        return 1;
    }

    int opt = 1;
    if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        fprintf(stderr, "Unable to set socket options;\n\t%s\n", strerror(errno));
        return 1;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
    if(bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        fprintf(stderr, "Unable to bind;\n\t%s\n", strerror(errno));
        return 1;
    }
    
    if(listen(master_socket, 16) < 0)
    {
        fprintf(stderr, "Unable to listen;\n\t%s\n", strerror(errno));
        return 1;
    }

    unsigned char buffer[1025];
    memset(buffer, '\0', sizeof(buffer));

    int addr_len = sizeof(address);

    connections_t connections;
    connections.count = 1;
    connections.size = 8;
    connections.users = calloc(connections.size, sizeof(user_t));

    if(connections.users == NULL)
    {
        fprintf(stderr, "Unable to allocate memory for connections\n");
        return 1;
    }

    connections.users[0].username = "Server";
    connections.users[0].pollfd.fd = master_socket;
    connections.users[0].pollfd.events = POLLIN;
    connections.users[0].pollfd.revents = 0;

    for(int i = 1; i < connections.size; i++)
        connections.users[i] = blank_user;

    printf("Server ready;\nWaiting for connections...\n");
    while(!exiting)
    {
        if(!connections_update_fds(&connections))
        {
            fprintf(stderr, "Unable to update fds\n");
            continue;
        }
        
        for(int sender = 1; sender < connections.size; sender++)
        {
            if(connections.users[sender].state == USER_CONNECTED && connections.users[sender].pollfd.revents & POLLOUT)
            {
                unsigned char username_message[] = "Enter username to begin chatting";

                size_t username_request_size = sizeof(event_t) + sizeof(username_message);
                event_t *username_request = (event_t *)malloc(username_request_size);

                username_request->code = EVENT_USERNAME_REQUEST;
                username_request->originator_id = 0;
                username_request->content_length = sizeof(username_message);
                strcpy(username_request->content, username_message);

                send(connections.users[sender].pollfd.fd, username_request, username_request_size, 0);
                free(username_request);
                connections.users[sender].state = USER_NO_USERNAME;

            }
            if(connections.users[sender].pollfd.revents & POLLIN)
            {
                memset(buffer, '\0', sizeof(buffer));
                read(connections.users[sender].pollfd.fd, buffer, sizeof(buffer));
                if(buffer[0] == '\0')
                {
                    printf("Client %d disconnected\n", sender);
                    size_t username_length = strlen(connections.users[sender].username) + 1;
                    event_t *user_leave_event = malloc(sizeof(event_t) + username_length);
                    if(user_leave_event != NULL)
                    {
                        user_leave_event->code = EVENT_USER_LEAVE;
                        user_leave_event->originator_id = sender;
                        user_leave_event->content_length = username_length;
                        strcpy(user_leave_event->content, connections.users[sender].username);
                        user_leave_event->content[username_length - 1] = '\0';

                        connections_relay_event_from(&connections, user_leave_event, sender);
                    }
                    free(user_leave_event);
                    connections_close_connection(&connections, sender);
                }
                else
                {
                    unsigned char *sanitized = allocate_sanitized_message(buffer);
                    switch(connections.users[sender].state)
                    {
                        case USER_NO_USERNAME:
                            strcpy(connections.users[sender].username, sanitized);
                            connections.users[sender].username[strlen(connections.users[sender].username)] = '\0';

                            char accpeted_message[] = "Username set";
                            size_t username_accepted_size = sizeof(event_t) + sizeof(accpeted_message);
                            event_t *username_accepted = (event_t *)malloc(username_accepted_size);

                            username_accepted->code = EVENT_USERNAME_ACCEPTED;
                            username_accepted->originator_id = 0;
                            username_accepted->content_length = sizeof(accpeted_message);
                            strcpy(username_accepted->content, accpeted_message);

                            send(connections.users[sender].pollfd.fd, username_accepted, username_accepted_size, 0);
                            free(username_accepted);

                            event_t *user_join_event = malloc(sizeof(event_t) + strlen(sanitized) + 1);
                            if(user_join_event != NULL)
                            {
                                user_join_event->code = EVENT_USER_JOIN;
                                user_join_event->originator_id = sender;
                                user_join_event->content_length = strlen(sanitized) + 1;
                                strcpy(user_join_event->content, sanitized);

                                connections_relay_event_from(&connections, user_join_event, sender);
                            }
                            free(user_join_event);
                            printf("Client %d set username as %s\n", sender, connections.users[sender].username);
                            connections.users[sender].state = USER_ACTIVE;

                            break;
                        
                        case USER_ACTIVE:
                            printf("Got message from client %d:\n%s\n", sender, sanitized);
                            connections_relay_message_from(&connections, sanitized, sender);
                            break;

                        case USER_CONNECTED:
                        case USER_UNINITIALIZED:
                        default:
                            break;
                    }
                    free(sanitized);
                }
            }
        }

        // new connection
        if(connections.users[0].pollfd.revents & POLLIN)
        {
            int new_connection = accept(master_socket, (struct sockaddr *)&address, (socklen_t *)&addr_len);
            if(new_connection < 0)
            {
                fprintf(stderr, "New connection failed to accept;\n\t%s\n", strerror(errno));
                continue;
            }

            int add_connection_result = connections_add_connection(&connections, new_connection, sizeof(buffer));
            if(add_connection_result <= 0)
            {
                fprintf(stderr, "Unable to allocate memory for new connection\n");
                continue;
            }
            printf("New connection as client %d\n", add_connection_result);
        }
    }

    printf("\nShutting down\n");
    connections_shutdown(&connections);
    shutdown(master_socket, SHUT_RDWR);
    return 0;
};