#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "utils.h"


/*
//LIST OF ALL COMMANDS
- mode [mode]
    changes mode of the system
    parameters:
    (mode) -a automatic operation -m manual
- run [amount]
    run in watering cycle in manual mode
    parameters:
    (int) amount in liters to be dispensed
- config [amount] [interval]
    change the config for automatic mode
    parameters:
    (int) new amount per cycle in liters
    (int) new interval per cycle in minutes
- weather (date)
    print the relevant weather data
    parameters:
    (date) date for which you want the weather data [optional]
    without parameter prints weather for the last 5 days and upcoming 5 days
*/
void processCommand(char *input, config_t *config){
    char *cmd_buffer = (char*)malloc(STARTING_CAPACITY * sizeof(char));
    char *param_buffer_first = (char*)malloc(STARTING_CAPACITY * sizeof(char));
    char *param_buffer_second = (char*)malloc(STARTING_CAPACITY * sizeof(char));
    int i = 0;
    while (input[i] != ' ' && input[i] != '\0')
    {
        if(i >= STARTING_CAPACITY) break;
    }
}

int readCmd(char **command)
{
    unsigned capacity = STARTING_CAPACITY;
    unsigned curLength = 0;
    char c;

    *command = (char*)malloc(capacity);
    if (*command == NULL) return ALLOCATION_ERR;

    while ((c = getchar()) != EOF && c != '\n' && c != '\0') {
        if(curLength >= MAX_LENGHT) return LENGHT_ERR;
        //increase capacity if length of message reaches capacity
        if (curLength >= capacity) {
            capacity = capacity * 2;
            char *tmp = realloc(*command, capacity);
            if (tmp == NULL){
                free(*command);
                return ALLOCATION_ERR;
            } 
            *command = tmp;
        }
        //write char into message and increase length
        (*command)[curLength] = c;
        curLength += 1;
    }
    if(curLength >= capacity){
        capacity = capacity * 2;
        char *tmp = realloc(*command, capacity);
        if (tmp == NULL){
            free(*command);
            return ALLOCATION_ERR;
        }
        *command = tmp;
    }
    (*command)[curLength] = '\0';

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
