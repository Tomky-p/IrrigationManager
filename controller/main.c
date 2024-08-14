#include <stdio.h>
#include <pthread.h>
#include "utils.h"
#include <string.h>
#include "weather_utils.h"
#include "gpio_utils.h"


//thread function manages the irrigation process in automatic mode
void* irrigationController(void *configuration);

//USAGE: ./irrigationManager [Mode][Amount (l)][Interval (min)]
int main(int argc, char *argv[]){
    //argument check
    if(argc < 4){
        fprintf(stderr, "Not enough arguments, USAGE: ./irrigationManager [Mode of(-a/-m)] [Amount of water per cycle in liters] [Time between cycles in minutes]\n");
        return EXIT_FAILURE;
    }
    //process variables 
    int arg_value = 0;

    //argument parsing
    if(strncmp(argv[1], "-m", 2) == 0) config.mode = MANUAL;
    else if (strncmp(argv[1], "-a", 2) == 0) config.mode = AUTO;
    else{
        fprintf(stderr, "Invalid argument, USAGE: -a for Automatic irrigation, -m for manual irrigation.\n");
        return EXIT_FAILURE;
    }

    if(checkArgument(argv[2]) == false){
        fprintf(stderr, "Invalid argument, provided amount is not an interger or is too large.\n");
        return EXIT_FAILURE;
    }
    arg_value = atoi(argv[2]);
    if(arg_value <= 0 || arg_value > MAX_AMOUNT){
        fprintf(stderr, "Invalid argument, selected amount higher that the maximum amount.\n");
        return EXIT_FAILURE;
    }
    config.amount = arg_value;

    if(checkArgument(argv[3]) == false){
        fprintf(stderr, "Invalid argument, provided interval is not an interger or is too large.\n");
        return EXIT_FAILURE;
    }
    arg_value = atoi(argv[3]);
    if(arg_value < MIN_INTERVAL){
        fprintf(stderr, "Invalid argument, selected an interval shorter that the minimum lenght.\n");
        return EXIT_FAILURE;
    }
    config.interval = arg_value;
    config.running = true;
    config.dispensing = false;
    config.amount_immidiate = MAX_TIME + 1;

    const char *start_string = (config.interval <) < 10 ? "Starting with following configuration:\nmode: %d\namount: %.0f l\nin an interval of: %d minutes\n" : 
                                                          "Starting with following configuration:\nmode: %d\namount: %.0f l\nin an interval of: %d minutes\n";
    printf(start_string, config.mode, (config.duration*60), (config.time/100), (config.time % 100));

    printf("mode: %d\n", config.mode);
    printf("amount: %d\n", config.amount);
    printf("interval: %d\n", config.interval);

    int *IC_thread_result;
    int *CMD_thread_result;

    //command line monitoring and handling thread
    pthread_t cmdMonitor;
    //irrigation executor thread
    pthread_t irrigationControl;

    if(pthread_mutex_init(&config_mutex, NULL) != 0){
        fprintf(stderr, "FATAL ERR! Failed to initialize configuration mutex.");
        return EXIT_FAILURE;
    }
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
            config.running = false;
            return (void*)ret;
        }
        //printf("Mode: %d\n", config.mode);
        //printf("Running: %s\n", config.filtration_running ? "true" : "false");
        //printf("curtime: %d\n", curtime);
        if(config.mode == AUTO && curtime == config.time && !config.dispensing){        
            float duration = config.duration;
            pthread_mutex_unlock(&config_mutex);
            runIrrigation(duration);
        }
        else if(config.mode == MANUAL && config.amount_immidiate < (MAX_TIME + 1)){
            float duration = (float)(config.amount_immidiate - curtime)/(float)60;
            //invalidate the run until time
            config.amount_immidiate = MAX_TIME + 1; 
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
            fprintf(stderr, "FATAL ERR! Memory allocation failure.\n");
            break;
        }
        if(*ret == TIME_ERR){
            fprintf(stderr, "FATAL ERR! Failed to get current time.");
            break;
        }
        pthread_mutex_lock(&config_mutex);
    }
    pthread_mutex_unlock(&config_mutex);
    config.running = false;
    free(command);
    return (void*)ret;
}
