#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "event.h"

#define BUFFER_SIZE 129
typedef struct {
    char buffer[BUFFER_SIZE];
    unsigned int position;
    bool done;
} input_buffer_t;

void print_event_to_window(event_t *event, WINDOW *window)
{
    wprintw(window, "===========================\n");
    wprintw(window, "Event code: %02X\n", event->code);
    wprintw(window, "Originator id: %02X\n", event->originator_id);
    wprintw(window, "Content length: %08lX\n", event->content_length);
    wprintw(window, "Content: ");

    for(size_t i = 0; i < event->content_length; i++)
        wprintw(window, "%02X | ", event->content[i]);
    wprintw(window, "\n===========================\n");
}

int main(int argc, const char **argv)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0)
    {
        fprintf(stderr, "Unable to create socket;\n\t%s\n", strerror(errno));
        return 1;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);

    if(inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) < 0)
    {
        fprintf(stderr, "Unable to use address;\n\t%s\n", strerror(errno));
        return 1;
    }

    if(connect(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        fprintf(stderr, "Unable to connect to server;\n\t%s\n", strerror(errno));
        return 1;
    }

    struct pollfd server_connection = {.fd = server_fd, .events = POLLIN | POLLOUT | POLLHUP, .revents = 0};


    initscr();
    start_color();
    init_color(8, 650, 650, 650);
    init_pair(1, 8, COLOR_BLACK);
    raw();
    noecho();
    keypad(stdscr, TRUE);
    nl();

    int max_x, max_y;
    getmaxyx(stdscr, max_y, max_x);

    WINDOW *history_window = newwin(max_y - 2, 0, 0, 0);
    scrollok(history_window, true);
    WINDOW *input_window = newwin(1, 0, max_y - 2, 0);
    nodelay(input_window, true);

    input_buffer_t input_buffer;
    memset(input_buffer.buffer, 0, sizeof(input_buffer.buffer));
    input_buffer.position = 0;
    input_buffer.done = false;

    bool exit = false;

    while(!exit)
    {
        poll(&server_connection, 1, 0);

        if(server_connection.revents & POLLIN)
        {
            event_t *incoming_event = malloc(sizeof(event_t));
            if(incoming_event == NULL)
                continue;
            read(server_connection.fd, incoming_event, sizeof(event_t));

            incoming_event = realloc(incoming_event, sizeof(event_t) + incoming_event->content_length);
            if(incoming_event == NULL)
                continue;
            read(server_connection.fd, incoming_event->content, incoming_event->content_length);

            if(incoming_event->content[0] == '\0')
            {
                printf("Lost connection to server\n");
                exit = true;
                free(incoming_event);
                continue;
                // break out of the loop; might work with POLLHUP;
                wprintw(history_window, "# Lost connection to server\n");
            }
            
            // print_event_to_window(incoming_event, history_window);
            switch(incoming_event->code)
            {
                case EVENT_MESSAGE:
                    wprintw(history_window, "%s\n", incoming_event->content);
                    break;

                case EVENT_USERNAME_REQUEST:
                case EVENT_USERNAME_ACCEPTED:
                    wattr_on(history_window, A_ITALIC, NULL);
                    wcolor_set(history_window, 1, NULL);
                    wprintw(history_window, "# %s\n", incoming_event->content);
                    wattr_off(history_window, A_ITALIC, NULL);
                    wcolor_set(history_window, 0, NULL);
                    break;
                
                case EVENT_USER_JOIN:
                    wattr_on(history_window, A_ITALIC, NULL);
                    wcolor_set(history_window, 1, NULL);
                    wprintw(history_window, "# %s joined\n", incoming_event->content);
                    wattr_off(history_window, A_ITALIC, NULL);
                    wcolor_set(history_window, 0, NULL);
                    break;

                case EVENT_USER_LEAVE:
                    wattr_on(history_window, A_ITALIC, NULL);
                    wcolor_set(history_window, 1, NULL);
                    wprintw(history_window, "# %s left\n", incoming_event->content);
                    wattr_off(history_window, A_ITALIC, NULL);
                    wcolor_set(history_window, 0, NULL);
                    break;
                
                default:
                    break;
            }
            wrefresh(history_window);
            free(incoming_event);
        }

        if(server_connection.revents & POLLOUT && input_buffer.done)
        {
            write(server_connection.fd, input_buffer.buffer, strlen(input_buffer.buffer));
            memset(input_buffer.buffer, 0, sizeof(input_buffer.buffer));
            input_buffer.position = 0;
            input_buffer.done = false;
            wclear(input_window);
            wrefresh(input_window);
        }

        int input = wgetch(input_window);
        if(isprint(input) || isspace(input))
        {
            if(input != '\n' && input_buffer.position < BUFFER_SIZE - 1 && !input_buffer.done)
            {
                input_buffer.buffer[input_buffer.position++] = input;
                wprintw(input_window, "%c", input);
                wrefresh(input_window);
            }
            else if(input_buffer.position > 0)
            {
                input_buffer.done = true;
                wprintw(history_window, "> %s\n", input_buffer.buffer);
                wrefresh(history_window);
            }
        }
        else
        {
            switch(input)
            {
                // TODO add command keys here
                case 127:
                case KEY_BACKSPACE:
                    if(input_buffer.position > 0)
                    {
                        input_buffer.buffer[input_buffer.position--] = '\0';
                        wprintw(input_window, "\b \b");
                        wrefresh(input_window);
                    }
                    break;
                    
                case ('c' & 0x1f): // ^c
                    exit = true;
                    break;

                case ERR:
                default:
                    break;
            };
        }
    }

    endwin();
    close(server_fd);
    return 0;
};