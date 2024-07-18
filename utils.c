#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "utils.h"


/*
//LIST OF ALL COMMANDS
- mode
    changes mode of the system
    -a automatic operation -m manual
- run 
    run in 



*/

int readCmd(char *command)
{
    unsigned capacity = STARTING_CAPACITY;
    unsigned curLength = 0;
    char c;

    command = (char*)malloc(capacity);
    if (command == NULL) return ALLOCATION_ERR;

    while ((c = getchar()) != EOF && c != '\n' && c != '\0') {
        if(curLength > MAX_LENGHT) return LENGHT_ERR;
        //increase capacity if length of message reaches capacity
        if (curLength == capacity) {
            capacity = capacity * 2;
            char *tmp = realloc(command, capacity);
            if (tmp == NULL) return ALLOCATION_ERR;
            command = tmp;
        }
        //write char into message and increase length
        command[curLength] = c;
        curLength += 1;
    }
    return READING_SUCCESS;
}
 
bool checkArgument(char *arg){
    if(arg == NULL) return false;
    int i = 0;
    while ((arg[i] != '\0' && arg[i] != '\n') && i < MAX_DIGITS)
    {
        if(arg[i] < '0' || arg[i] > '9') return false; 
        i++;
    }
    if(arg[i] != '\0' && arg[i] != '\n'){
        return false;
    } 
    return true;
}
