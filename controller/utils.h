#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define STARTING_CAPACITY 4
#define MAX_LENGHT 15
#define ALLOCATION_ERR -1
#define READING_SUCCESS 0
#define MAX_DIGITS 5
#define LENGHT_ERR 1

#define AUTO 1
#define MANUAL 0

typedef struct{
    uint8_t mode;
    uint32_t amount;
    uint32_t interval;
}config_t;

//reads the user input commands from stdin
int readCmd(char *command);

//check whether an string argument is an interger
bool checkArgument(char *arg);
