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
//LIST OF ALL COMMANDS
- mode [mode] -(confirm)
    changes mode of the system
    parameters:
    (mode) -a automatic operation -m manual
- run [duration] -(confirm)
    run in watering cycle in manual mode
    parameters:
    (int) duration in liters to be dispensed
- config [duration] [time] -(confirm)
    change the config for automatic mode
    parameters:
    (int) new duration per cycle in liters
    (int) new time per cycle in minutes
- stop -(confirm)
    stops the current filtration cycle
- kill -(confirm)
    stops and quits the entire program

*/
int processCommand(char *input){
    char *cmd_buffer = (char*)calloc(MAX_LENGHT * sizeof(char), sizeof(char));
    char *param_buffer_first = (char*)calloc(MAX_LENGHT * sizeof(char), sizeof(char));
    char *param_buffer_second = (char*)calloc(MAX_LENGHT * sizeof(char), sizeof(char));
    //allocation check
    if(cmd_buffer == NULL || param_buffer_first == NULL || param_buffer_second == NULL){
        if(cmd_buffer != NULL) free(cmd_buffer);
        if(param_buffer_first != NULL) free(param_buffer_first);
        if(param_buffer_second !=  NULL) free(param_buffer_second);
        return ALLOCATION_ERR;
    }
    if(!splitToBuffers(input, cmd_buffer, param_buffer_first, param_buffer_second)){
        free(cmd_buffer);
        free(param_buffer_first);
        free(param_buffer_second);
        return EXIT_SUCCESS;
    }
    bool args_ok = true;
    //printf("command: %s\nfirst parameter: %s\nsecond parameter %s\n", cmd_buffer, param_buffer_first, param_buffer_second);
    if(strncmp(cmd_buffer, "mode", 5) == 0)
    {
        pthread_mutex_lock(&config_mutex);
        if(config.dispensing){
            fprintf(stderr, "Cannot switch the operating mode while the system is running.\n");
        }
        else if (strncmp(param_buffer_second, "", MAX_LENGHT) != 0){
            fprintf(stderr, "Too many parameters, USAGE: -m for manual or -a for automatic mode.\n");
        }
        else if (strncmp(param_buffer_first, "", MAX_LENGHT) == 0){
            const char *response = config.mode == AUTO ? "Currently running in automatic mode.\n" : "Currently running in manual mode.\n";
            printf("%s",response);        
        }     
        else if (strncmp(param_buffer_first, "-a", 3) == 0 && config.mode == AUTO){
            printf("Already running in automatic mode.\n");
        }
        else if (strncmp(param_buffer_first, "-a", 3) == 0 && config.mode != AUTO){
            printf("Switching to automatic mode...\n");
            config.mode = AUTO;           
        }    
        else if (strncmp(param_buffer_first, "-m", 3) == 0 && config.mode != MANUAL){
            printf("Switching to manual mode...\n");
            config.mode = MANUAL;
        }
        else if (strncmp(param_buffer_first, "-m", 3) == 0 && config.mode == MANUAL){
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
        /*if(config.dispensing){
            fprintf(stderr, "The system is already dispensing, wait for the cycle to finish or terminate it to run again.\n");
            args_ok = false;
        }*/
        pthread_mutex_unlock(&config_mutex);
        countArguments(1, &args_ok, param_buffer_first, param_buffer_second);
        verifyArguments(&args_ok, param_buffer_first, param_buffer_second, 1);
        /*if((args_ok && !checkArgument(param_buffer_first)) || (atoi(param_buffer_first) <= 0 && args_ok) || (atoi(param_buffer_first) > MAX_AMOUNT_PER_DAY  && args_ok)){
            args_ok = false;
            fprintf(stderr, "Invalid argument, provide a sufficient amount to be dispensed.\n");
        }
        if(args_ok && strncmp(param_buffer_second, "" , MAX_LENGHT) != 0){
            args_ok = false;
            fprintf(stderr, "Invalid arguments, provide only one argument.\n");
        }*/
        if(args_ok){
            int amount = atoi(param_buffer_first);
            printf("The irrigation system will dispense %d liters of water.\nAre you sure you want proceed?\n[y/n]", amount);
            int ret = recieveConfirmation(input);
            if(ret == ALLOCATION_ERR) return ALLOCATION_ERR;
            if(ret == YES){
                printf("Proceeding...\nLaunching irrigation for %d minutes.\n", getDispenseTime(amount));
                pthread_mutex_lock(&config_mutex);
                config.amount_immidiate = amount;
                pthread_mutex_unlock(&config_mutex);
            }
            else{
                printf("Aborted.\n");
            }   
        }   
    }
    //TO DO: Rework and adapt the config command
    else if (strncmp(cmd_buffer, "config", 7) == 0)
    {
        if(strncmp(param_buffer_first, "", MAX_LENGHT) == 0 && strncmp(param_buffer_second, "" , MAX_LENGHT) == 0){
            args_ok = false;
            printConfig();
        }
        /*pthread_mutex_lock(&config_mutex);
        if(args_ok && config.dispensing){
            fprintf(stderr, "Filtration is currently running. Cannot change config now.\n");
            args_ok = false;
        }
        pthread_mutex_unlock(&config_mutex);*/
        verifyState(&args_ok, false);
        //if(!checkArgument(param_buffer_first) || !checkArgument(param_buffer_second)) args_ok = false;

        /*if((args_ok && atoi(param_buffer_first) <= 0) || (args_ok && atoi(param_buffer_second) <= 0)
        || (args_ok && atoi(param_buffer_first) > MAX_DURATION) || (args_ok && atoi(param_buffer_second) > MAX_TIME)){
            fprintf(stderr, "Provided parameters are invalid! USAGE: config [times per day] [amount](in liters)\n");
            args_ok = false;
        } */
        countArguments(2, &args_ok, param_buffer_first, param_buffer_second);
        verifyArguments(&args_ok, param_buffer_first, param_buffer_second, 2);
        if(args_ok){
            uint8_t new_times_per_day = atoi(param_buffer_first);
            uint16_t new_amount = atoi(param_buffer_second);
            printf("Set new configuration?\nCycles per day: %d\nAmount: %d l\nAre you sure you want proceed?\n[y/n]", new_times_per_day, new_amount);
            int ret = recieveConfirmation(input);
            if(ret == ALLOCATION_ERR) return ALLOCATION_ERR;
            if(ret == YES){
                pthread_mutex_lock(&config_mutex);
                config.amount = new_amount;
                config.times_per_day = new_times_per_day;
                pthread_mutex_unlock(&config_mutex);
                if(getTimeValues(input, new_times_per_day) == ALLOCATION_ERR) return ALLOCATION_ERR;
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
        /*if(strncmp(param_buffer_first, "", MAX_LENGHT) != 0 || strncmp(param_buffer_second, "" , MAX_LENGHT) != 0){
            fprintf(stderr, "This command does not take any parameters.\n");
            args_ok = false;
        }*/
        countArguments(0, &args_ok, param_buffer_first, param_buffer_second);
        if(args_ok){
            printf("WARNING! The irrigation system controler will be terminated.\nAre you sure you want proceed?\n[y/n]");
            int ret = recieveConfirmation(input);
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
        /*if(strncmp(param_buffer_first, "", MAX_LENGHT) != 0 || strncmp(param_buffer_second, "" , MAX_LENGHT) != 0){
            fprintf(stderr, "This command does not take any parameters.\n");
            args_ok = false;
        }*/
        countArguments(0, &args_ok, param_buffer_first, param_buffer_second);
        /*pthread_mutex_lock(&config_mutex);
        if(!config.dispensing){
            fprintf(stderr, "The system is currently not dispensing.\n");
            args_ok = false;
        }
        pthread_mutex_unlock(&config_mutex);*/
        verifyState(&args_ok, true);
        if(args_ok && !checkDeviceState()){
            fprintf(stderr, "The system is currently not dispensing.\n");
            args_ok = false;
        }
        if(args_ok){
            printf("The system is currently dispensing.\nAre you sure you want stop irrigating?\n[y/n]");
            int ret = recieveConfirmation(input);
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
     //TO DO implement the help and state command
    else if (strncmp(cmd_buffer, "help", 5) == 0)
    {
        printf("%s", HELP_MESSAGE);
    }
    else if (strncmp(cmd_buffer, "state", 6) == 0)
    {
        countArguments(0, &args_ok, param_buffer_first, param_buffer_second);
        if(args_ok){
            char *state_string = checkDeviceState() ? "Currently dispensing...\n" : "System is not dispensing.\n";
            printf("%s", state_string);
        }   
    }  
    else{
        fprintf(stderr, "Command not recognized.\n");
    }
    free(cmd_buffer);
    free(param_buffer_first);
    free(param_buffer_second);
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

bool splitToBuffers(char *input, char *cmd_buffer, char *param_buffer_first, char *param_buffer_second){
    int index = 0;
    int buffer_index = 0;
    int offset = 0;
    int current_buffer = 1;
    while (input[index] != '\0')
    {
        switch (current_buffer)
        {
        case 1:
            while (input[index] != ' ' && input[index] != '\0')
            {        
                cmd_buffer[index] = input[index];
                index++;
                if(index >= MAX_LENGHT){
                    fprintf(stderr, "Input parameter too long and not recongized.\n");
                    return false;
                }
            }
            cmd_buffer[index] = '\0';
            if(input[index] != '\0'){
                current_buffer++;
                index++;
            }
            break;
        case 2:
            buffer_index = 0;
            offset = index;
            while (input[index] != ' ' && input[index] != '\0')
            {        
                param_buffer_first[buffer_index] = input[index];
                index++;
                buffer_index++;
                if(buffer_index >= MAX_LENGHT - offset){
                    fprintf(stderr, "Input parameter too long and not recongized.\n");
                    return false;
                }
            }
            param_buffer_first[buffer_index] = '\0';
            if(input[index] != '\0'){
                current_buffer++;
                index++;
            }
            break;
        case 3:
            buffer_index = 0;
            offset = index;
            while (input[index] != '\0' && input[index] != ' ')
            {        
                param_buffer_second[buffer_index] = input[index];
                index++;
                buffer_index++;
                if(buffer_index >= MAX_LENGHT - offset){
                    fprintf(stderr, "Input parameter too long and not recongized.\n");
                    return false;
                }
            }
            if(input[index] != '\0'){
                fprintf(stderr, "Too many parameters, there are commands that accept 2 parameters at most.\n");
                return false;
            }
            param_buffer_second[buffer_index] = '\0';
            break;   
        default:
            fprintf(stderr, "Failed to parse command!\n");
            return false;
        }
        if(index >= MAX_LENGHT){
            fprintf(stderr, "Input command too long and not recongized.\n");
            return false;
        }
    }
    return true;
}

int recieveConfirmation(char *command){
    int ret = readCmd(&command);
    if(ret == ALLOCATION_ERR) return ALLOCATION_ERR;
    if(ret != READING_SUCCESS || strncmp(command, "y", 2) != 0) return NO;
    else return YES;
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

int readTimeFromUser(char **buffer){
    int ret = readCmd(buffer);
    if(ret < 0) return ret;
    if(!checkArgument(buffer)) return LENGHT_ERR;
    return atoi(*buffer);
}

int getDispenseTime(uint16_t amount){
    return (amount*60)/LITERS_PER_HOUR;
}

bool isDispensingTime(int curtime){
    for (size_t i = 0; i < config.times_per_day; i++)
    {
        if (config.time_routine[i] == curtime) return true;
    }
    return false;
}

void printConfig(){
    pthread_mutex_lock(&config_mutex);
    const char *conf_string = (config.mode == AUTO) ? "Mode: AUTOMATIC\nAmount of: %d l\nDispensing %d times per day at these times:\n":
                                                        "Mode: MANUAL\nAmount of: %d l\nDispensing %d times per day at these times:\n";
    printf("%s", conf_string, config.amount, config.times_per_day);
    for (size_t i = 0; i < config.times_per_day; i++)
    {
        const char *time_string = ((config.time_routine[i] % 100) < 10) ? "%d:0%d " : "%d:%d ";
        printf("%s", time_string, config.time_routine[i]/100, config.time_routine[i] % 100);
    }
    putchar('\n');
    pthread_mutex_unlock(&config_mutex);
}

void verifyState(bool *args_ok, bool desiredOn){
    if(!args_ok) return;
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
    if(!(*args_ok)) return;
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

//TO DO: Implement the checkInterval() function
int getTimeValues(char *buffer, int times_per_day){
    pthread_mutex_lock(&config_mutex);
    uint16_t *tmp = (uint16_t*)realloc(config.time_routine, sizeof(uint16_t) * config.times_per_day);
    //malloc check
    if(tmp == NULL || buffer == NULL){
        fprintf(stderr, ALLOC_ERR_MSG);
        return ALLOCATION_ERR;
    }
    config.time_routine = tmp;
    //get a time values and put the in the array
    int index = 0;
    while(true){
        int time = readTimeFromUser(&buffer);
        if(time == ALLOCATION_ERR){
            fprintf(stderr, ALLOC_ERR_MSG);
            return ALLOCATION_ERR;
        }
        if(!isIntTime(time)){
            fprintf(stderr, INVALID_TIME_INPUT);
            continue;
        }
        if(!checkIntervals(time)){
            fprintf(stderr, INVALID_INTERVAL_MSG);
            continue;
        }
        char *time_string = (time % 100 > 10) ? "Added %d:%d" : "Added %d:0%d";
        printf("%s", time_string, time/100, time%100);
        config.time_routine[index] = time;
        index++;
        if(index >= config.times_per_day) break;
    }
    pthread_mutex_unlock(&config_mutex);
    return EXIT_SUCCESS;
}

void verifyArguments(bool *args_ok, char *first_arg, char *second_arg, int desired_count){
    switch (desired_count){
    case 1:
        if(!checkArgument(first_arg)){
            *args_ok = false;
            fprintf(stderr, "Invalid argument! Please provide a number of liters to be dispensed.\n");
            break;
        }
        if(atoi(first_arg) > MAX_AMOUNT_PER_DAY || atoi(first_arg) <= 0){
            *args_ok = false;
            fprintf(stderr, "Invalid amount! Please provide a number of liters to be dispensed, %d liters at most.\n", MAX_AMOUNT_PER_DAY);
        }
        break;
    
    case 2:
        if(!checkArgument(first_arg) || !checkArgument(second_arg)){
            *args_ok = false;
            fprintf(stderr, "Provided parameters are invalid! PROVIDE: [times per day] [amount](in liters)\n");
            break;    
        } 
        if((atoi(first_arg) * atoi(second_arg)) > MAX_AMOUNT_PER_DAY){
            *args_ok = false;
            fprintf(stderr, "The system can dispense at most %d liters of water per day. Choose a lower amount or set less cycles per day.\n", MAX_AMOUNT_PER_DAY);
            break;
        }
        if(atoi(first_arg) > MAX_TIMES_PER_DAY){
            *args_ok = false;
            fprintf(stderr, "The system can dispense at most %d liters of water per day. Choose a lower amount or set less cycles per day.\n", MAX_AMOUNT_PER_DAY);
        }
        break;
    }
}