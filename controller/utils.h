#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

//debug guard
#ifndef DEBUG_NON_PI
#define DEBUG_NON_PI false
#endif

//Conversion constants
#define DAY_IN_MINUTES 1440
#define MAX_TIME 2359
#define MIDNIGHT 0

#ifndef LITERS_PER_HOUR
//the amount of water flowing into the system per hour (based on the water pump)
#define LITERS_PER_HOUR 100
#endif

//String constants
#define MAX_LENGHT 25
#define MAX_DIGITS 5
#define STARTING_CAPACITY 4

//ERR and SUCCESS Codes
#define ALLOCATION_ERR -1
#define LENGHT_ERR -3
#define TIME_ERR -2
#define READING_SUCCESS 0

//ERR messages
#define ALLOC_ERR_MSG "FATAL ERR! Memory allocation failed.\n"
#define TIME_ERR_MSG "FATAL ERR! Failed to get current time.\n"
#define GPIO_ERR_MSG "FATAL ERR! Failed to initialize access to GPIO periferies.\n"
#define WEATHER_ERR_MSG "CRITICAL ERR! Failed to get weather. Connection failed or API not responding.\n"
#define CONNECTION_ERR_MSG "CRITICAL ERR! Connection lost.\n"
#define FILE_NOT_FOUND_ERR_MSG "CRITICAL ERR! Failed to read file %s.\n"

//USER ERR messages
#define INVALID_TIME_INPUT "Provided time is invalid. Provide a number corresponding to a time of day in format: 1135 = 11:35\n"
#define SYSTEM_RUNNING_MSG "Cannot execute command the system is currently dispensing.\n"
#define SYSTEM_NOT_RUNNING_MSG "Cannot execute command the system is currently not dispensing.\n"
#define INVALID_INTERVAL_MSG "Times of day must have bigger intervals between each other, atleast 4 hours.\n"
#define INVALID_AMOUNT_MSG "The system can dispense at most %d liters of water per day. Choose a lower amount or set less cycles per day.\n"
#define INVALID_CYCLES_NUMBER_MSG "The system can dispense at most %d times a day.\n"
#define INVALID_ONE_TIME_AMOUNT_MSG "Invalid amount! Please provide a number of liters to be dispensed, %d liters at most.\n"
#define INVALID_NUMBER_AMOUNT_MSG "Invalid argument! Please provide a number of liters to be dispensed.\n"
#define INVALID_ARGS_MSG "Provided parameters are invalid! PROVIDE: [times per day] [amount](in liters)\n"

//INFO messages
#define HELP_MESSAGE "LIST OF ALL COMMANDS\n- mode [mode]\n  changes mode of the system\n    parameters:\n    (mode) -a automatic operation -m manual\n   - without parameters: show the current mode\n- run [amount] -(confirm)\n    run in watering cycle in manual mode\n    parameters:\n    (int) amount in liters to be dispensed\n- config [times per day] [amount] -(confirm)\n    change the configuration for automatic mode\n    parameters:\n    (int) new number of cycles per day\n    (int) new amount to be dispensed per cycle\n    - without paramater: shows current configuration\n- stop -(confirm)\n    stops the water pump\n- kill -(confirm)\n    stops and quits the entire program\n- state\n    shows whether the system is dispensing or not\n"

//mode values
#define AUTO 1
#define MANUAL 0


#ifndef MAX_AMOUNT_PER_DAY
//The maximum amount of water to be realeased in a cycle (in liters)
#define MAX_AMOUNT_PER_DAY 230
#endif

#ifndef MIN_INTERVAL
//Time in between each release of water into the system (in minutes)
#define MIN_INTERVAL 239
#endif

#ifndef VOLUME
//Total volume of source water body when full
#define VOLUME 250
#endif

//amount of water in liters that should be supplied to square meter of garden every day
#define LITER_PER_SQR_M_PER_DAY 2.7

#ifndef IRRIGATED_AREA
//the total square are of the garden in square meters
#define IRRIGATED_AREA 56
#endif

//recommended amount of water to be dispensed per day based on the irrigated area
#define RECOMMENDED_AMOUNT (float)IRRIGATED_AREA * LITER_PER_SQR_M_PER_DAY

#define REVERSE_INTERVAL DAY_IN_MINUTES - MIN_INTERVAL

#ifndef MAX_TIMES_PER_DAY
//maximum number of times per the system runs per day
#define MAX_TIMES_PER_DAY 4
#endif

#define YES 1
#define NO 0


typedef struct{
    uint8_t mode;
    //amount to be dispensed in liters
    uint16_t amount;
    //number of days in between routines
    uint8_t times_per_day;
    //specific times of day when the system runs
    uint16_t *time_routine;
    //used in manual mode to pass the amount
    uint16_t amount_immidiate;
    //amount dispensed today
    float amount_dispensed;
    //time at which
    uint16_t time_last_ran;
    //indicates the state of the program
    bool running;
    //indicates whether the system is dispensing water currently
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

bool splitToBuffers(char *input, char *cmd_buffer, char *param_buffer_first, char *param_buffer_second);

int recieveConfirmation();

int getCurrentTime();

const char* getCurrentDateTime();

bool isIntTime(int time_val);

//returns how many minutes it takes to dispense a certain amount of water
float getDispenseTime(uint16_t amount);

//read a time value from user
int readTimeFromUser();

//returns true the its time to dispence water according to config
bool isDispensingTime(int curtime);

//prints the current configuration
void printConfig();

//reads and saves time values from user to config
int getTimeValues(int times_per_day);

//verifies wether the arguments are valid
void verifyArguments(bool *args_ok, char *first_arg, char *second_arg, int desired_count);

//verifies wether the correct amount of arguments was provided
void countArguments(int desired_count, bool *args_ok, char *first_param, char *second_param);

void verifyState(bool *args_ok, bool desiredOn);

bool checkIntervals(int time, int time_count, uint16_t *time_vals);

int getMinutesBetweenTimes(int time_1, int time_2);

int getNextTime(uint16_t time_val, int prevOrNext);
