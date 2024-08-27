#include <stdio.h>
#include <pthread.h>
#include "utils.h"
#include <string.h>
#include "weather_utils.h"
#include "gpio_utils.h"


//thread routine function manages the irrigation process in automatic mode
void* irrigationController();

//thread routine function that reads and processes the commands the user inputs
void* cmdManager();

//USAGE: ./irrigationManager [Mode] [Number of times per day] [Amount (l)]
int main(int argc, char *argv[]){
    //argument check
    if(argc < 4){
        fprintf(stderr, "Not enough arguments, USAGE: ./irrigationManager [Mode of(-a/-m)] [Number of times per day] [Amount of water per cycle in liters]\n");
        return EXIT_FAILURE;
    }
    //initialize the mutex
    if(pthread_mutex_init(&config_mutex, NULL) != 0){
        fprintf(stderr, "FATAL ERR! Failed to initialize configuration mutex.");
        return EXIT_FAILURE;
    }
    //argument parsing
    if(strncmp(argv[1], "-m", 2) == 0) config.mode = MANUAL;
    else if (strncmp(argv[1], "-a", 2) == 0) config.mode = AUTO;
    else{
        fprintf(stderr, "Invalid argument, USAGE: -a for Automatic irrigation, -m for manual irrigation.\n");
        return EXIT_FAILURE;
    }
    bool args_ok = true;
    verifyArguments(&args_ok, argv[2], argv[3], 2);
    /*
    int arg_value = 0;
    if(checkArgument(argv[2]) == false){
        fprintf(stderr, "Invalid argument, provided times_per_day is not an interger or is too large.\n");
        return EXIT_FAILURE;
    }
    arg_value = atoi(argv[2]);
    if(arg_value > MAX_TIMES_PER_DAY){
        fprintf(stderr, "Invalid argument, selected an times_per_day shorter that the minimum lenght.\n");
        return EXIT_FAILURE;
    }
    config.times_per_day = arg_value;

    if(checkArgument(argv[3]) == false){
        fprintf(stderr, "Invalid argument, provided amount is not an interger or is too large.\n");
        return EXIT_FAILURE;
    }
    arg_value = atoi(argv[3]);
    if(arg_value <= 0 || arg_value > MAX_AMOUNT_PER_DAY){
        fprintf(stderr, "Invalid argument, selected amount higher that the maximum amount.\n");
        return EXIT_FAILURE;
    }
    config.amount = arg_value;*/

    printf("Please enter %d times of day at which irrigation should commence.\n", config.times_per_day);

    char *command = (char*)malloc(STARTING_CAPACITY);
    config.time_routine = (uint16_t*)malloc(sizeof(uint16_t) * config.times_per_day);
    if(getTimeValues(command, config.times_per_day) == ALLOCATION_ERR){
        return ALLOCATION_ERR;
    }
    /*
    //malloc check
    if(config.time_routine == NULL || command == NULL){
        fprintf(stderr, "FATAL ERR! Memory allocation failed.\n");
        return ALLOCATION_ERR;
    }
    //get a time values and put the in the array
    int index = 0;
    while(true){
        int time = readTimeFromUser(&command);
        if(time == ALLOCATION_ERR){
            fprintf(stderr, "FATAL ERR! Memory allocation failed.\n");
            return ALLOCATION_ERR;
        }
        if(!isIntTime(time)){
            fprintf(stderr, INVALID_TIME_INPUT);
            continue;
        }
        if(!checkIntervals(time)){
            fprintf(stderr, "Times of day must have bigger intervals between each other.\n");
            continue;
        }

        printf("Added %d:");
        config.time_routine[index] = time;
        index++;
        if(index >= config.times_per_day) break;
    }*/
    free(command);
    config.running = true;
    config.dispensing = false;
    config.amount_immidiate = 0;
    
    //print config
    printf("Starting with following configuration:\n");
    printConfig();
    /*
    const char *start_string = (config.mode == AUTO) ? "Starting with following configuration:\nMode: AUTOMATIC\nAmount of: %d l\nDispensing %d times per day at these times: " :
                                                        "Starting with following configuration:\nMode: MANUAL\nAmount of: %d l\nDispensing %d times per day at these times: ";
    printf("%s", start_string, config.amount, config.times_per_day);
    for (size_t i = 0; i < config.times_per_day; i++)
    {
        const char *time_string = ((config.time_routine[i] % 100) < 10) ? "%d:0%d " : "%d:%d ";
        printf("%s", time_string, config.time_routine[i]/100, config.time_routine[i] % 100);
    }
    putchar('\n');*/

    int *IC_thread_result;
    int *CMD_thread_result;

    //command line monitoring and handling thread
    pthread_t cmdMonitor;
    //irrigation executor thread
    pthread_t irrigationControl;

    //create threads
    if(pthread_create(&cmdMonitor, NULL, cmdManager, NULL) != 0){
        fprintf(stderr, "FATAL ERR! Failed to create command line monitor thread.\n");
        pthread_mutex_destroy(&config_mutex);
        return EXIT_FAILURE;
    }
    if(pthread_create(&irrigationControl, NULL, automaticController, NULL) != 0){
        fprintf(stderr, "FATAL ERR! Failed to create automatic filtration thread.\n");
        pthread_mutex_destroy(&config_mutex);
        return EXIT_FAILURE;
    }
    //join threads
    if(pthread_join(irrigationControl, (void**)&IC_thread_result) != 0 ||
    pthread_join(cmdMonitor, (void**)&CMD_thread_result) != 0){
        fprintf(stderr, "FATAL ERR! Failed to join threads.");
        pthread_mutex_destroy(&config_mutex);
        return EXIT_FAILURE;
    }
    //shutdown if not already shutdown
    if(checkDeviceState()){
        shutdownWaterPump();
    }
    pthread_mutex_destroy(&config_mutex);

    int IC_ret = *IC_thread_result;
    int CMD_ret = *CMD_thread_result;
    free(IC_thread_result);
    free(CMD_thread_result);

    if(IC_ret != EXIT_SUCCESS) return IC_ret;
    if(CMD_ret != EXIT_SUCCESS) return CMD_ret;
    return EXIT_SUCCESS;
}


void* automaticController(){   
    int *ret = malloc(sizeof(int));
    *ret = EXIT_SUCCESS;
    printf("Launching automatic filtration thread.\n");
    
    //initialize gpio pin interface
    if(initGpioPinControl() == ALLOCATION_ERR){
        config.running = false;
        *ret = GPIO_ERR;
        return (void*)ret;
    }
    pthread_mutex_lock(&config_mutex);
    while (config.running)
    {    
        int curtime = getCurrentTime();
        if(curtime == TIME_ERR){
            *ret = TIME_ERR;
            fprintf(stderr, "%s", TIME_ERR_MSG);
            break;
        }
        //printf("Mode: %d\n", config.mode);
        //printf("Running: %s\n", config.filtration_running ? "true" : "false");
        //printf("curtime: %d\n", curtime);
        
        if(config.mode == AUTO && isDispensingTime(curtime) && !config.dispensing){        
            int duration = getDispenseTime(config.amount);
            pthread_mutex_unlock(&config_mutex);
            runIrrigation(duration);
        }
        else if(config.mode == MANUAL && config.amount_immidiate > 0){
            int duration = getDispenseTime(config.amount_immidiate);
            //invalidate the run until time
            config.amount_immidiate = 0; 
            pthread_mutex_unlock(&config_mutex);
            runIrrigation(duration);
        }
        else{
            pthread_mutex_unlock(&config_mutex);
        }
        //wait 100ms NOTE: cannot be over a minute
        delay(100);
        pthread_mutex_lock(&config_mutex);
    }
    pthread_mutex_unlock(&config_mutex);
    config.running = false;
    return (void*)ret;
}

void* cmdManager(){
    int *ret = malloc(sizeof(int));
    char *command = (char*)malloc(STARTING_CAPACITY);
    if (command == NULL){
        *ret = ALLOCATION_ERR;
        config.running = false;
        return (void*)ret;
    } 
    printf("Launching command line listener thread.\n");

    //command line control thread
    pthread_mutex_lock(&config_mutex);
    while (config.running)
    {
        pthread_mutex_unlock(&config_mutex);
        *ret = readCmd(&command);
        if(*ret == READING_SUCCESS){
            //printf("Executing: %s\n", command);
            *ret = processCommand(command);
        }
        if(*ret == ALLOCATION_ERR){
            fprintf(stderr, "%s", ALLOC_ERR_MSG);
            break;
        }
        if(*ret == TIME_ERR){
            fprintf(stderr, "%s", TIME_ERR_MSG);
            break;
        }
        pthread_mutex_lock(&config_mutex);
    }
    pthread_mutex_unlock(&config_mutex);
    config.running = false;
    free(command);
    return (void*)ret;
}
