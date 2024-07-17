#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define STARTING_CAPACITY 4
#define MAX_CAPACITY 15
#define ALLOCATION_ERR -1
#define READING_SUCCESS 0
#define MAX_DIGITS 5

#define AUTO 1
#define MANUAL 0

/*
//LIST OF ALL COMMANDS
- mode
    changes mode of the system
    -a automatic operation -m manual
- run 
    run in 



*/
typedef struct config{
    uint8_t mode;
    uint32_t amount;
    uint32_t interval;
}config_t;

int readCmd(char *command)
{
    unsigned capacity = STARTING_CAPACITY;
    unsigned curLength = 0;
    command = (char*)malloc(capacity);
    char c;
    if (command == NULL) return ALLOCATION_ERR;

    while ((c = getchar()) != EOF && c != '\n') {
        //increase capacity if length of message reaches capacity
        if (curLength == capacity) {
            capacity = capacity * 2;
            char *message2 = realloc(command, capacity);
            if (message2 == NULL) return ALLOCATION_ERR;

            command = message2;
        }
        //write char into message and increase length
        command[curLength] = c;
        curLength += 1;
    }
    return READING_SUCCESS;
}

//check whether an string argument is an interger 
bool checkArgument(char *arg){
    if(arg == NULL) return false;
    int i = 0;
    while ((arg[i] != '\0' || arg[i] != '\n') && i < MAX_DIGITS)
    {
        if(arg[i] < '0' || arg[i] > '9') return false;
        i++;
    }
    if(arg[MAX_DIGITS + 1] != '\0') return false;
    return true;
}