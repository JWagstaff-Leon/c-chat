#include "server.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include "connections.h"

server_init_result server_initialize(server_t *out_server)
{
    int master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(master_socket < 0)
        return SINIT_SOCKET_FAILED;

    int opt = 1;
    if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        return SINIT_OPTIONS_FAILED;

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
    if(bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
        return SINIT_BIND_FAILED;
    
    if(listen(master_socket, 16) < 0)
        return SINIT_LISTEN_FAILED;

    connections_t *connections = malloc(sizeof(connections_t));
    if(connections == NULL)
        return SINIT_ALLOCATION_FAILED;

    connections->count = 1;
    connections->size = 8;
    connections->users = calloc(connections.size, sizeof(user_t));

    if(connections->users == NULL)
        return SINIT_ALLOCATION_FAILED;

    connections->users[0].username = "Server";
    connections->users[0].pollfd.fd = master_socket;
    connections->users[0].pollfd.events = POLLIN;
    connections->users[0].pollfd.revents = 0;

    for(int i = 1; i < connections->size; i++)
        connections->users[i] = blank_user;

    out_server->connections = connections;
    return SINIT_SUCCESS;
};



void server_cleanup(server_t *server)
{
    connections_shutdown(&connections);
    free(server->connecitons);
};
// FIXME find out where to put these if anywhere
// unsigned char buffer[1025];
// memset(buffer, '\0', sizeof(buffer));
// 
// int addr_len = sizeof(address);