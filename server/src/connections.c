#include "connections.h"

#include <assert.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "event.h"
#include "user.h"

connections_t *connections_init(size_t size)
{
    connections_t *new_connections = malloc(sizeof(connections_t));
    assert(new_connections != NULL);
    
    new_connections->count = 0;
    new_connections->size = size;
    
    new_connections->users = calloc(size, sizeof(user_t));
    assert(new_connections->users != NULL);
    
    new_connections->pollfds = calloc(size, sizeof(struct pollfd));
    assert(new_connections->pollfds != NULL);

    return new_connections;
};



void connections_destroy(connections_t *connections)
{
    for(int i = 0; i < connections->size; i++)
        if(connections->users[i].state > USER_UNINITIALIZED)
            connections_close_connection(connections, i);

    free(connections->users);
    free(connections->pollfds);
    free(connections);
};



void connections_update(connections_t *connections)
{
    for(int i = 0; i < connections->size; i++)
    {
        connections->pollfds[i] = connections->users[i].pollfd;
    }

    poll(connections->pollfds, connections->size, 0);

    for(int i = 0; i < connections->size; i++)
    {
        connections->users[i].pollfd = connections->pollfds[i];
    }

    memset(connections->pollfds, 0, sizeof(struct pollfd) * connections->size);
};



bool connections_is_connection_readable(connections_t *connections, int connection)
{
    assert(connection > 0 && connection < connections->size);
    return connections->users[connection].pollfd.revents & POLLIN;
};



bool connections_is_connection_writeable(connections_t *connections, int connection)
{
    assert(connection > 0 && connection < connections->size);
    return connections->users[connection].pollfd.revents & POLLOUT;
};



enum user_state connections_get_user_state(connections_t *connections, int user)
{
    assert(user > 0 && user < connections->size);
    return connections->users[user].state;
};



void connections_set_user_state(connections_t *connections, int user, enum user_state state)
{
    assert(user > 0 && user < connections->size);
    connections->users[user].state = state;
};



void connections_send_event_to_fd(connections_t *connections, event_t *event, int reciever_fd)
{
    connections_send_event_with_content_to_fd(connections, event, event->content, reciever_fd);
};



void connections_send_event_with_content_to_fd(connections_t *connections, event_t *event, unsigned char event_content[], int reciever_fd)
{
    send(reciever_fd, event, sizeof(event_t), 0);
    send(reciever_fd, event_content, event->content_length, 0);
};



void connections_send_event_to(connections_t *connections, event_t *event, int reciever)
{
    connections_send_event_to_fd(connections, event, connections->users[reciever].pollfd.fd);
};



void connections_send_event_with_content_to(connections_t *connections, event_t *event, unsigned char event_content[], int reciever)
{
    connections_send_event_with_content_to_fd(connections, event, event_content, connections->users[reciever].pollfd.fd);
};



void connections_relay_event_from(connections_t *connections, event_t *event, int sender)
{
    for(int reciever = 0; reciever < connections->size; reciever++)
        if(connections->users[reciever].state > USER_UNINITIALIZED && sender != reciever && connections->users[reciever].pollfd.revents & POLLOUT)
            connections_send_event_to(connections, event, reciever);
};



void connections_relay_event_with_content_from(connections_t *connections, event_t *event, unsigned char event_content[], int sender)
{
    for(int reciever = 0; reciever < connections->size; reciever++)
        if(connections->users[reciever].state > USER_UNINITIALIZED && sender != reciever && connections->users[reciever].pollfd.revents & POLLOUT)
            connections_send_event_with_content_to(connections, event, event_content, reciever);
};



void connections_relay_message_from(connections_t *connections, unsigned char *message, int sender)
{
    size_t decorated_message_length = strlen(connections->users[sender].username) + 3 + strlen(message);
    event_t *message_event = malloc(sizeof(event_t) + decorated_message_length);
    if(message_event == NULL)
    {
        fprintf(stderr, "Unable to send message\n");
        return;
    }

    message_event->code = EVENT_MESSAGE;
    message_event->originator_id = sender;
    message_event->content_length = decorated_message_length;

    strcpy(message_event->content, connections->users[sender].username);
    strcat(message_event->content, ": ");
    strcat(message_event->content, message);
    message_event->content[decorated_message_length - 1] = '\0';

    connections_relay_event_from(connections, message_event, sender);
    free(message_event);
};



int connections_add_connection(connections_t *connections, int new_connection)
{
    if(connections->count >= connections->size)
    {
        size_t new_size = connections->size * GROW_FACTOR;
        user_t *new_users = reallocarray(connections->users, new_size, sizeof(user_t));
        struct pollfd *new_pollfds = reallocarray(connections->pollfds, new_size, sizeof(struct pollfd));
        if(new_users == NULL || new_pollfds == NULL)
            return 0;

        for(int i = connections->size; i < new_size; i++)
            new_users[i] = blank_user;

        memset(&new_pollfds[connections->size], 0, (new_size - connections->size) * sizeof(struct pollfd));

        connections->users = new_users;
        connections->pollfds = new_pollfds;
        connections->size = new_size;
    }

    user_t new_user = {.username = NULL, .pollfd = {.fd = new_connection, .events = POLLIN | POLLOUT, .revents = 0}, .state = USER_CONNECTED};
    int insert_position = 0;
    size_t usernames_size = 0;
    unsigned char *usernames = NULL;
    bool user_list_failed = false;
    for(int i = 0; i < connections->size; i++)
    {
        // TODO put this all into one spot by summing strlen preeemptively
        if(!user_list_failed && connections->users[i].state == USER_ACTIVE)
        {
            size_t next_username_size = strlen(connections->users[i].username) + 1;
            unsigned char *usernames = realloc(usernames, usernames_size + next_username_size);
            if(usernames == NULL)
            {
                user_list_failed = true;
                continue;
            }

            memcpy(&usernames[usernames_size], connections->users[i].username, next_username_size);
            usernames_size += next_username_size;
        }
        if(insert_position <= 0 && connections->users[i].state == USER_UNINITIALIZED)
            insert_position = i;
    }

    event_t *user_list_event = malloc(sizeof(event_t) + usernames_size);
    if(user_list_event != NULL && usernames != NULL)
    {
        user_list_event->code = EVENT_USER_LIST;
        user_list_event->originator_id = 0;
        user_list_event->content_length = usernames_size;
        memcpy(user_list_event->content, usernames, usernames_size);

        connections_send_event_to_fd(connections, user_list_event, new_user.pollfd.fd);
    }
    free(user_list_event);

    connections->users[insert_position] = new_user;
    connections->count++;

    free(usernames);
    return insert_position;
};



void connections_close_connection(connections_t *connections, unsigned int index)
{
    if(index < 0 || index > connections->size)
        return;

    shutdown(connections->users[index].pollfd.fd, SHUT_RDWR);
    close(connections->users[index].pollfd.fd);
    free(connections->users[index].username);
    connections->users[index] = blank_user;
    connections->count--;
};