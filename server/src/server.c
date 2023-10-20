#include "server.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "connections.h"
#include "event.h"
#include "messages.h"
#include "user.h"

server_t *server_init(FILE *standard_output, FILE *error_output)
{
    server_t *new_server = malloc(sizeof(server_t));
    if(new_server == NULL)
        return NULL;

    int master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(master_socket < 0)
    {
        if(error_output != NULL)
            fprintf(error_output, "Unable to create socket;\n\t%s\n", strerror(errno));
        free(new_server);
        return NULL;
    }

    int opt = 1;
    if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        if(error_output != NULL)
            fprintf(error_output, "Unable to set socket options;\n\t%s\n", strerror(errno));
        free(new_server);
        return NULL;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
    if(bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        if(error_output != NULL)
            fprintf(error_output, "Unable to bind;\n\t%s\n", strerror(errno));
        free(new_server);
        return NULL;
    }
    
    if(listen(master_socket, 16) < 0)
    {
        if(error_output != NULL)
            fprintf(stderr, "Unable to listen;\n\t%s\n", strerror(errno));
        free(new_server);
        return NULL;
    }

    connections_t *connections = connections_init(DEFAULT_SERVER_SIZE);
    if(connections == NULL)
    {
        if(error_output != NULL)
            fprintf(stderr, "Unable to allocate memory for connections\n");
        free(new_server);
        return NULL;
    }

    connections->count = 1;
    
    connections->users[0].username = "Server";
    connections->users[0].pollfd.fd = master_socket;
    connections->users[0].pollfd.events = POLLIN;
    connections->users[0].pollfd.revents = 0;
    connections->users[0].state = USER_SERVER;

    for(int i = 1; i < connections->size; i++)
        connections->users[i] = blank_user;

    new_server->connections = connections;
    new_server->standard_output = standard_output;
    new_server->error_output = error_output;

    if(standard_output != NULL)
        fprintf(standard_output, "Server ready;\nWaiting for connections...\n");
    return new_server;
};



void server_destroy(server_t *server)
{
    if(server->standard_output != NULL)
        fprintf(server->standard_output, "\nServer shutting down\n");

    connections_relay_event_with_content_from(server->connections, &shutdown_event, shutdown_message, 0);

    connections_destroy(server->connections);
    free(server);
};



void server_send_global_server_event(server_t *server, event_t *event)
{
    connections_relay_event_from(server->connections, event, 0);
};



void server_send_global_server_event_with_content(server_t *server, event_t *event, unsigned char event_content[])
{
    connections_relay_event_with_content_from(server->connections, event, event_content, 0);
};



void server_send_server_event_to(server_t *server, event_t *event, int reciever_fd)
{
    connections_send_event_to_fd(server->connections, event, reciever_fd);
};



void server_send_server_event_with_content_to(server_t *server, event_t *event, unsigned char event_content[], int reciever_fd)
{
    connections_send_event_with_content_to_fd(server->connections, event, event_content, reciever_fd);
};



void server_do_tick(server_t *server)
{
    server_start_new_tick(server);
    server_introduce_users(server);
    server_handle_users_input(server);
    server_check_new_users(server);
};



void server_start_new_tick(server_t *server)
{
    connections_update(server->connections);
};



void server_introduce_users(server_t *server)
{
    for(int user = 1; user < server->connections->size; user++)
    {
        if(connections_get_user_state(server->connections, user) == USER_UNINITIALIZED
           && connections_is_connection_writeable(server->connections, user))
        {
            send(server->connections->users[user].pollfd.fd, &username_request_event, sizeof(event_t), 0);
            send(server->connections->users[user].pollfd.fd, &username_request_message, sizeof(username_request_message), 0);
            server->connections->users[user].state = USER_NO_USERNAME;
        }
    }
};



void server_handle_users_input(server_t *server)
{

    for(int sender = 1; sender < server->connections->size; sender++)
    {
        
        if(! (server->connections->users[sender].pollfd.revents & POLLIN))
            continue;

        event_t *incoming_event = malloc(sizeof(event_t));

        read(server->connections->users[sender].pollfd.fd, incoming_event, sizeof(event_t));
        if(incoming_event->content_length > MAX_CONTENT_LENGTH)
        {
            send(server->connections->users[sender].pollfd.fd, &oversized_content_event, sizeof(event_t), 0);
            
            unsigned char ignore_buffer;
            for(
                int i = 0;
                i < incoming_event->content_length
                && read(server->connections->users[sender].pollfd.fd, &ignore_buffer, 1) > 0;
                i++
            );

            free(incoming_event);
            continue;
        }

        incoming_event = realloc(incoming_event, sizeof(event_t) + incoming_event->content_length);
        assert(incoming_event != NULL); // REVIEW maybe don't assert in operating path
        read(server->connections->users[sender].pollfd.fd, incoming_event->content, incoming_event->content_length);
        unsigned char *sanitized = NULL;
        switch(incoming_event->code)
        {
            case EVENT_USER_LEAVE:
            case EVENT_UNDEFINED:
            default:
                printf("Client %d disconnected\n", sender);
                size_t username_length = strlen(server->connections->users[sender].username) + 1;
                event_t *user_leave_event = malloc(sizeof(event_t) + username_length);
                if(user_leave_event != NULL)
                {
                    user_leave_event->code = EVENT_USER_LEAVE;
                    user_leave_event->originator_id = sender;
                    user_leave_event->content_length = username_length;
                    strcpy(user_leave_event->content, server->connections->users[sender].username);
                    user_leave_event->content[username_length - 1] = '\0';

                    connections_relay_event_from(server->connections, user_leave_event, sender);
                }
                free(user_leave_event);
                connections_close_connection(server->connections, sender);
                break;


            case EVENT_USERNAME_REQUEST:
            case EVENT_USERNAME_SUBMIT:
                if(server->connections->users[sender].state == USER_NO_USERNAME)
                {
                    sanitized = allocate_sanitized_message(incoming_event->content);
                    server->connections->users[sender].username = sanitized;
                    // server->connections->users[sender].username[strlen(server->connections->users[sender].username)] = '\0';
                    send(server->connections->users[sender].pollfd.fd, &username_accepted_event, sizeof(event_t), 0);
                    send(server->connections->users[sender].pollfd.fd, &username_accepted_message, sizeof(username_accepted_message), 0);

                    event_t *user_join_event = malloc(sizeof(event_t) + strlen(sanitized) + 1);
                    if(user_join_event != NULL)
                    {
                        user_join_event->code = EVENT_USER_JOIN;
                        user_join_event->originator_id = sender;
                        user_join_event->content_length = strlen(sanitized) + 1;
                        strcpy(user_join_event->content, sanitized);

                        connections_relay_event_from(server->connections, user_join_event, sender);
                    }
                    free(user_join_event);

                    printf("Client %d set username as %s\n", sender, server->connections->users[sender].username);
                    server->connections->users[sender].state = USER_ACTIVE;
                }
                break;


            case EVENT_MESSAGE:
                if(server->connections->users[sender].state >= USER_ACTIVE)
                {
                    sanitized = allocate_sanitized_message(incoming_event->content);
                    printf("Got message from client %d:\n%s\n", sender, sanitized);
                    connections_relay_message_from(server->connections, sanitized, sender);
                }
                break;

            case EVENT_USERNAME_ACCEPTED:
            case EVENT_USERNAME_REJECTED:
            case EVENT_CONNECTION_FAILED:
            case EVENT_SERVER_SHUTDOWN:
            case EVENT_USER_LIST:
            case EVENT_USER_JOIN:
                // no op
        }

        free(sanitized);
        free(incoming_event);
    }
};



void server_check_new_users(server_t *server)
{
    if(server->connections->users[0].pollfd.revents & POLLIN)
    {
        struct sockaddr_in address;
        socklen_t addr_len = sizeof(address);
        int new_connection = accept(server->connections->users[0].pollfd.fd, (struct sockaddr *)&address, (socklen_t *)&addr_len);
        if(new_connection < 0)
        {
            if(server->error_output != NULL)
                fprintf(server->error_output, "New connection failed to accept;\n\t%s\n", strerror(errno));
            return;
        }

        int add_connection_result = connections_add_connection(server->connections, new_connection);
        if(add_connection_result <= 0)
        {
            server_send_server_event_with_content_to(server, &new_connection_fail_event, new_connection_fail_message, new_connection);
            close(new_connection);
            if(server->error_output != NULL)
                fprintf(server->error_output, "Unable to allocate memory for new connection\n");
            return;
        }

        if(server->standard_output != NULL)
            fprintf(server->standard_output, "New connection as client %d\n", add_connection_result);
    }
};