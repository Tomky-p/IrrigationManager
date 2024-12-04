#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include "utils.h"
#include "gpio_utils.h"

//initialize global (thread shared) variables 
config_t config = {0};
pthread_mutex_t config_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
LIST OF ALL COMMANDS
- mode [mode]
    changes mode of the system
    parameters:
    (mode) -a automatic operation -m manual
    - without parameters: show the current mode
- run [amount] -(confirm)
    run in watering cycle in manual mode
    parameters:
    (int) amount in liters to be dispensed
- config [times per day] [amount] -(confirm)
    change the config for automatic mode
    parameters:
    (int) new number of cycles per day
    (int) new amount to be dispensed per cycle
    - without paramater: prints current config
- stop -(confirm)
    stops the current irrigation cycle
- kill -(confirm)
    stops and quits the entire program
- state
    show whether the system is dispensing or not
*/
int processCommand(char *input){
    char *cmd_buffer, arg1_buffer, arg2_buffer, tmp_ptr;

    cmd_buffer = strtok_r(input, " \r\n", &tmp_ptr);
    arg1_buffer = strtok_r(NULL, " \r\n", &tmp_ptr);
    arg2_buffer = strtok_r(NULL, " \r\n", &tmp_ptr);

    bool args_ok = true;
    if(strncmp(cmd_buffer, "mode", 5) == 0)
    {
        pthread_mutex_lock(&config_mutex);
        if(config.dispensing){
            fprintf(stderr, "Cannot switch the operating mode while the system is running.\n");
        }
        else if (strncmp(arg2_buffer, "", MAX_LENGHT) != 0){
            fprintf(stderr, "Too many parameters, USAGE: -m for manual or -a for automatic mode.\n");
        }
        else if (strncmp(arg1_buffer, "", MAX_LENGHT) == 0){
            const char *response = config.mode == AUTO ? "Currently running in automatic mode.\n" : "Currently running in manual mode.\n";
            printf("%s",response);        
        }     
        else if (strncmp(arg1_buffer, "-a", 3) == 0 && config.mode == AUTO){
            printf("Already running in automatic mode.\n");
        }
        else if (strncmp(arg1_buffer, "-a", 3) == 0 && config.mode != AUTO){
            printf("Switching to automatic mode...\n");
            config.mode = AUTO;           
        }    
        else if (strncmp(arg1_buffer, "-m", 3) == 0 && config.mode != MANUAL){
            printf("Switching to manual mode...\n");
            config.mode = MANUAL;
        }
        else if (strncmp(arg1_buffer, "-m", 3) == 0 && config.mode == MANUAL){
            printf("Already running in manual mode.\n");
        }
        else{
            fprintf(stderr, "Unrecognized parameter, USAGE: -m for manual or -a for automatic mode.\n");
        }
        pthread_mutex_unlock(&config_mutex);
    }
    else if (strncmp(cmd_buffer, "run", 4) == 0)
    {   
        pthread_mutex_lock(&config_mutex);
        if(config.mode == AUTO){
            fprintf(stderr, "Currently running in automatic mode, to use run command switch to manual mode.\n");
            args_ok = false;
        }
        pthread_mutex_unlock(&config_mutex);
        verifyState(&args_ok, false);
        countArguments(1, &args_ok, arg1_buffer, arg2_buffer);
        verifyArguments(&args_ok, arg1_buffer, arg2_buffer, 1);
        if(args_ok){
            int amount = atoi(arg1_buffer);
            printf("The irrigation system will dispense %d liters of water.\nAre you sure you want proceed?\n[y/n]", amount);
            int ret = recieveConfirmation();
            if(ret == ALLOCATION_ERR) return ALLOCATION_ERR;
            if(ret == YES){
                if(getDispenseTime(amount) < 1) printf("Proceeding...\nLaunching irrigation for %0.f seconds.\n", getDispenseTime(amount)*60);
                else printf("Proceeding...\nLaunching irrigation for %0.f minutes.\n", getDispenseTime(amount));
                pthread_mutex_lock(&config_mutex);
                config.amount_immidiate = amount;
                pthread_mutex_unlock(&config_mutex);
            }
            else{
                printf("Aborted.\n");
            }   
        }   
    }
    else if (strncmp(cmd_buffer, "config", 7) == 0)
    {
        if(strncmp(arg1_buffer, "", MAX_LENGHT) == 0 && strncmp(arg2_buffer, "" , MAX_LENGHT) == 0){
            args_ok = false;
            printConfig();
        }
        verifyState(&args_ok, false);
        countArguments(2, &args_ok, arg1_buffer, arg2_buffer);
        verifyArguments(&args_ok, arg1_buffer, arg2_buffer, 2);

        if(args_ok){
            uint8_t new_times_per_day = atoi(arg1_buffer);
            uint16_t new_amount = atoi(arg2_buffer);
            printf("Set new configuration?\nCycles per day: %d\nAmount: %d l\nAre you sure you want proceed?\n[y/n]", new_times_per_day, new_amount);
            int ret = recieveConfirmation();
            if(ret == ALLOCATION_ERR) return ALLOCATION_ERR;
            if(ret == YES){
                printf("Please enter %d times of day at which irrigation should commence.\n", new_times_per_day);
                if(getTimeValues(new_times_per_day) == ALLOCATION_ERR) return ALLOCATION_ERR;
                pthread_mutex_lock(&config_mutex);
                config.amount = new_amount;
                pthread_mutex_unlock(&config_mutex);
                printf("Configuration set to:\n");
                printConfig();
            }
            else{
                printf("Aborted.\n");
            }
        }
    }
    else if (strncmp(cmd_buffer, "kill", 5) == 0)
    {
        countArguments(0, &args_ok, arg1_buffer, arg2_buffer);
        if(args_ok){
            printf("WARNING! The irrigation system controler will be terminated.\nAre you sure you want proceed?\n[y/n]");
            int ret = recieveConfirmation();
            if(ret == ALLOCATION_ERR) return ALLOCATION_ERR;
            if(ret == YES){
                pthread_mutex_lock(&config_mutex);
                printf("Quitting program...\n");
                config.running = false;
                pthread_mutex_unlock(&config_mutex);
                if(checkDeviceState()){
                    shutdownWaterPump();
                }
            }   
            else{
                printf("Aborted.\n");
            }
        }
    }
    else if (strncmp(cmd_buffer, "stop", 5) == 0)
    {
        countArguments(0, &args_ok, arg1_buffer, arg2_buffer);
        verifyState(&args_ok, true);
        if(args_ok && !checkDeviceState()){
            fprintf(stderr, "The system is currently not dispensing.\n");
            args_ok = false;
        }
        if(args_ok){
            printf("The system is currently dispensing.\nAre you sure you want stop irrigating?\n[y/n]");
            int ret = recieveConfirmation();
            if(ret == ALLOCATION_ERR) return ALLOCATION_ERR;
            if(ret == YES){
                printf("Stopping filtration...\n");
                pthread_mutex_lock(&config_mutex);
                config.dispensing = false;
                pthread_mutex_unlock(&config_mutex);
            }
            else{
                printf("Aborted.\n");
            }
        }   
    }
    else if (strncmp(cmd_buffer, "help", 5) == 0)
    {
        printf("%s", HELP_MESSAGE);
    }
    else if (strncmp(cmd_buffer, "state", 6) == 0)
    {
        countArguments(0, &args_ok, arg1_buffer, arg2_buffer);
        if(args_ok){
            char *state_string = checkDeviceState() ? "Currently dispensing...\n" : "System is not dispensing.\n";
            printf("%s", state_string);
        }   
    }  
    else{
        fprintf(stderr, "Command not recognized.\n");
    }
    return EXIT_SUCCESS;
}

int readCmd(char **command)
{
    unsigned capacity = STARTING_CAPACITY;
    unsigned curLength = 0;
    char c;

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
    int index = 0;
    while ((arg[index] != '\0' && arg[index] != '\n') && index < MAX_DIGITS)
    {
        if(arg[index] < '0' || arg[index] > '9') return false; 
        index++;
    }
    if(arg[index] != '\0' && arg[index] != '\n') return false; 
    return true;
}

int recieveConfirmation(){
    char *buffer = (char*)malloc(STARTING_CAPACITY);
    if(buffer == NULL) return ALLOCATION_ERR;
    int ret = readCmd(&buffer);
    if(ret == ALLOCATION_ERR) return ALLOCATION_ERR;
    if(ret != READING_SUCCESS || strncmp(buffer, "y", 2) != 0) {
        free(buffer);
        return NO;
    }
    else {
        free(buffer);
        return YES;
    }
}

int getCurrentTime(){
    time_t now = time(NULL);
    struct tm *currentTime = localtime(&now);
    if (currentTime == NULL) return TIME_ERR;
    return (currentTime->tm_hour*100 + currentTime->tm_min);
}

bool isIntTime(int time_val){
    if(time_val > MAX_TIME || time_val < 0) return false;
    int minutes = time_val % 100;
    if(minutes > 59 || minutes < 0) return false;
    return true;
}

int readTimeFromUser(){
    char *buffer = (char*)malloc(STARTING_CAPACITY);
    if(buffer == NULL) return ALLOCATION_ERR;
    int ret = readCmd(&buffer);
    if(!checkArgument(buffer)) ret = LENGHT_ERR;
    else ret = atoi(buffer);
    free(buffer);
    return ret;
}

float getDispenseTime(uint16_t amount){
    return ((float)amount*(float)60)/(float)LITERS_PER_HOUR;
}

bool isDispensingTime(int curtime){
    for (int i = 0; i < config.times_per_day; i++)
    {
        if (config.time_routine[i] == curtime) return true;
    }
    return false;
}

void printConfig(){
    pthread_mutex_lock(&config_mutex);
    const char *conf_string = (config.mode == AUTO) ? "Mode: AUTOMATIC\nAmount of: %d l\nDispensing %d times per day at these times:\n":
                                                        "Mode: MANUAL\nAmount of: %d l\nDispensing %d times per day at these times:\n";
    printf(conf_string, config.amount, config.times_per_day);
    for (int i = 0; i < config.times_per_day; i++)
    {
        const char *time_string = ((config.time_routine[i] % 100) < 10) ? "%d:0%d " : "%d:%d ";
        printf(time_string, config.time_routine[i]/100, config.time_routine[i] % 100);
    }
    putchar('\n');
    pthread_mutex_unlock(&config_mutex);
}

void verifyState(bool *args_ok, bool desiredOn){
    if((*args_ok) == false) return;
    if(checkDeviceState()){
        if(desiredOn) return;
        *args_ok = false;
        fprintf(stderr, SYSTEM_RUNNING_MSG);
    }
    else{
        if(!desiredOn) return;
        *args_ok = false;
        fprintf(stderr, SYSTEM_NOT_RUNNING_MSG); 
    } 
}

void countArguments(int desired_count, bool *args_ok, char *first_param, char *second_param){
    if((*args_ok) == false) return;
    switch (desired_count)
    {
    case 0:
        if(strncmp(first_param, "", MAX_LENGHT) != 0 || strncmp(second_param, "" , MAX_LENGHT) != 0){
            fprintf(stderr, "This command does not take any parameters.\n");
            *args_ok = false;
        }
        break;
    case 1:
        if(strncmp(second_param, "" , MAX_LENGHT) != 0){
            fprintf(stderr, "This command takes only one parameter\n");
            *args_ok = false;
        }
        if(strncmp(first_param, "", MAX_LENGHT) == 0){
            fprintf(stderr, "Too few parameters.\n");
            *args_ok = false;
        }
        break;
    case 2:
        if(strncmp(first_param, "", MAX_LENGHT) == 0 || strncmp(second_param, "" , MAX_LENGHT) == 0){
            fprintf(stderr, "Too few parameters.\n");
            *args_ok = false;
        }
        break;
    }
}

int getTimeValues(int times_per_day){
    uint16_t *vals = (uint16_t*)malloc(sizeof(uint16_t) * times_per_day);
    if(vals == NULL) return ALLOCATION_ERR;
    //get a time values and put the in the array
    int index = 0;
    while(true){
        int time = readTimeFromUser();
        if(time == ALLOCATION_ERR) return ALLOCATION_ERR;
        if(!isIntTime(time)){
            fprintf(stderr, INVALID_TIME_INPUT);
            continue;
        }
        if(!checkIntervals(time, index, vals)) {
            fprintf(stderr, INVALID_INTERVAL_MSG);
            continue;
        }
        char *time_string = (time % 100 >= 10) ? "Added %d:%d\n" : "Added %d:0%d\n";
        printf(time_string, time/100, time%100);
        vals[index] = time;
        index++;
        if(index >= times_per_day) break;
    }
    pthread_mutex_lock(&config_mutex);
    if(config.time_routine != NULL) free(config.time_routine);
    config.time_routine = vals;
    config.times_per_day = times_per_day;
    pthread_mutex_unlock(&config_mutex);
    return EXIT_SUCCESS;
}

void verifyArguments(bool *args_ok, char *first_arg, char *second_arg, int desired_count){
    if((*args_ok) == false) return;
    switch (desired_count){
    case 1:
        if(!checkArgument(first_arg)){
            *args_ok = false;
            fprintf(stderr, INVALID_NUMBER_AMOUNT_MSG);
            break;
        }
        if(atoi(first_arg) > MAX_AMOUNT_PER_DAY || atoi(first_arg) <= 0){
            *args_ok = false;
            fprintf(stderr, INVALID_ONE_TIME_AMOUNT_MSG, MAX_AMOUNT_PER_DAY);
        }
        break;
    
    case 2:
        if(!checkArgument(first_arg) || !checkArgument(second_arg)){
            *args_ok = false;
            fprintf(stderr, INVALID_ARGS_MSG);
            break;    
        } 
        if((atoi(first_arg) * atoi(second_arg)) > MAX_AMOUNT_PER_DAY){
            *args_ok = false;
            fprintf(stderr, INVALID_AMOUNT_MSG, MAX_AMOUNT_PER_DAY);
            break;
        }
        if(atoi(first_arg) > MAX_TIMES_PER_DAY){
            *args_ok = false;
            fprintf(stderr, INVALID_CYCLES_NUMBER_MSG, MAX_TIMES_PER_DAY);
        }
        break;
    }
}
bool checkIntervals(int time, int time_count, uint16_t *time_vals){
    for (int i = 0; i < time_count; i++)
    {
        int minutes_apart = getMinutesBetweenTimes(time_vals[i], time);
        if(minutes_apart < MIN_INTERVAL || minutes_apart > REVERSE_INTERVAL) return false;
        
    }
    return true;
}

int getMinutesBetweenTimes(int time_1, int time_2){
    int minutes_1 = time_1 % 100;
    int hours_1 = time_1 / 100;
    int minutes_2 = (time_2 % 100)*(-1);
    int hours_2 = (time_2 / 100)*(-1);
    return abs(((hours_1 + hours_2)*60) +  minutes_1 + minutes_2);   
}

int getNextTime(uint16_t time_val, int prevOrNext){
    pthread_mutex_lock(&config_mutex);
    int time = (prevOrNext < 0) ? TIME_ERR : MAX_TIME + 1;
    for (size_t i = 0; i < config.times_per_day; i++)
    {
        if(config.time_routine[i] < time_val && config.time_routine[i] > time && prevOrNext < 0){
            time = config.time_routine[i];
        }
        else if(config.time_routine[i] > time_val && config.time_routine[i] < time && prevOrNext > 0){
            time = config.time_routine[i];
        }
    }
    if(time == MAX_TIME + 1 || time == TIME_ERR){
        for (size_t i = 0; i < config.times_per_day; i++)
        {
            if(config.time_routine[i] < time && prevOrNext > 0) time = config.time_routine[i];
            if(config.time_routine[i] > time && prevOrNext < 0) time = config.time_routine[i];
        }    
    }
    pthread_mutex_unlock(&config_mutex);
    return time;
}
