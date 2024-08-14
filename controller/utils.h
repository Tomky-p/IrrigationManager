#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

//Conversion constants
#define DAY_IN_MINUTES 1440

#ifndef LITERS_PER_HOUR
//the amount of water flowing into the system per hour (based on the water pump)
#define LITERS_PER_HOUR 240
#endif

//String constants
#define MAX_LENGHT 25
#define MAX_DIGITS 5
#define STARTING_CAPACITY 4

//ERR and SUCCESS Codes
#define ALLOCATION_ERR -1
#define LENGHT_ERR 1
#define TIME_ERR -2
#define READING_SUCCESS 0

#define AUTO 1
#define MANUAL 0


#ifndef MAX_AMOUNT
//The maximum amount of water to be realeased in a cycle (in liters)
#define MAX_AMOUNT 170
#endif

#ifndef MIN_INTERVAL
//Time in between each release of water into the system (in minutes)
#define MIN_INTERVAL 360
#endif

#ifndef LOCATION
//Geographic location name used to pull weather data
#define LOCATION "Poděbrady"
#endif

#define YES 1
#define NO 0


typedef struct{
    uint8_t mode;
    uint32_t amount;
    uint16_t interval;
    uint16_t amount_immidiate;
    bool running;
    bool dispensing;
}config_t;

extern config_t config;
extern pthread_mutex_t config_mutex;

//reads the user input commands from stdin
int readCmd(char **command);

//check whether an string argument is an interger
bool checkArgument(char *arg);

//proccess the commands from user, updates the config for irrigation thread and runs irrigation in manual mode
int processCommand(char *input);

bool checkArgumentFloat(char *arg);

bool splitToBuffers(char *input, char *cmd_buffer, char *param_buffer_first, char *param_buffer_second);

int recieveConfirmation(char *command);

int getCurrentTime();

int sendRunSignal(float duration);

int timeArithmeticAdd(int time, float duration);

bool isIntTime(int time_val);
