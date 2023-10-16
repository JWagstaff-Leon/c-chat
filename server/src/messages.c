#include "messages.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

unsigned char *allocate_sanitized_message(unsigned char *input_message)
{
    int input_length = strlen(input_message);

    unsigned char *sanitized = malloc(input_length + 1);

    int i,s;
    for(i = s = 0; input_message[i] != '\0'; i++)
        if(isprint(input_message[i]))
            sanitized[s++] = input_message[i];
    
    sanitized[input_length] = '\0';
    size_t sanitized_length = strlen(sanitized) + 1;
    sanitized = realloc(sanitized, sanitized_length);
    sanitized[sanitized_length - 1] = '\0';

    return sanitized;
};