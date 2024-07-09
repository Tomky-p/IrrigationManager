#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define STARTING_CAPACITY 4
#define MAX_CAPACITY 

int readCmd(char *command)
{
    unsigned capacity = STARTING_CAPACITY;
    command = malloc(capacity);
    char c;
    unsigned curLength = 0;
    if (command == NULL) {
        free(command);
        command = NULL;
        curLength = 0;
    }
    else {
        while ((c = getchar()) != EOF && c != '\n') {
            //increase capacity if length of message reaches capacity
            if (curLength == capacity) {
                capacity = capacity * 2;
                char *message2 = realloc(command, capacity);
                if (message2 == NULL) {
                    free(message2);
                    command = NULL;
                    curLength = 0;
                }
                command = message2;
            }
            //write char into message and increase length
            command[curLength] = c;
            curLength += 1;
        }
    }
    return command;
}