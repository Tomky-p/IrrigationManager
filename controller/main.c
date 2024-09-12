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
    if(!args_ok) return EXIT_FAILURE;
    config.times_per_day = atoi(argv[2]);
    config.amount = atoi(argv[3]);

    //get times of day
    printf("Please enter %d times of day at which irrigation should commence.\n", config.times_per_day);

    config.time_routine = NULL;
    if(getTimeValues(config.times_per_day) == ALLOCATION_ERR){
        fprintf(stderr, "%s", ALLOC_ERR_MSG);
        return ALLOCATION_ERR;
    }
    //set everything going
    config.running = true;
    config.dispensing = false;
    config.amount_immidiate = 0;
    config.time_last_ran = TIME_ERR;
    
    //print config
    printf("Starting with following configuration:\n");
    printConfig();

    int *IC_thread_result;
    int *CMD_thread_result;

    //command line monitoring and handling thread
    pthread_t cmdMonitor;
    //irrigation executor thread
    pthread_t irrigationControl;

    /*req_params_t request_params;
    request_params.days = 0;
    request_params.api_key, request_params.coords = NULL;
    if((getRequestData(&request_params)) != EXIT_SUCCESS){
        config.running = false;
        return EXIT_FAILURE;
    }*/

    //create threads
    if(pthread_create(&cmdMonitor, NULL, cmdManager, NULL) != 0){
        fprintf(stderr, "FATAL ERR! Failed to create command line monitor thread.\n");
        pthread_mutex_destroy(&config_mutex);
        return EXIT_FAILURE;
    }
    if(pthread_create(&irrigationControl, NULL, irrigationController, NULL) != 0){
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
    free(config.time_routine);

    if(IC_ret != EXIT_SUCCESS) return IC_ret;
    if(CMD_ret != EXIT_SUCCESS) return CMD_ret;
    return EXIT_SUCCESS;
}


void* irrigationController(){  
    int *ret = malloc(sizeof(int));
    *ret = EXIT_SUCCESS;
    printf("Launching automatic filtration thread.\n");
    
    //initialize gpio pin interface
    /*if(initGpioPinControl() == ALLOCATION_ERR){
        config.running = false;
        *ret = GPIO_ERR;
        return (void*)ret;
    }*/
    //initialize api request parameters
    req_params_t request_params;
    request_params.api_key = NULL;
    request_params.coords = NULL;
    if((*ret = getRequestData(&request_params)) != EXIT_SUCCESS){
        pthread_mutex_lock(&config_mutex); 
        config.running = false;
        pthread_mutex_unlock(&config_mutex);
        return (void*)ret;
    }

    printf("%s\n", request_params.api_key);
    printf("%s\n", request_params.coords);
    pthread_mutex_lock(&config_mutex);
    while (config.running)
    {    
        int curtime = getCurrentTime();
        if(curtime == TIME_ERR){
            *ret = TIME_ERR;
            fprintf(stderr, "%s", TIME_ERR_MSG);
            break;
        }
        if(curtime == MIDNIGHT) config.amount_dispensed = 0;
        //Implement weather checks
        if(config.mode == AUTO && isDispensingTime(curtime) && !config.dispensing && config.time_last_ran != curtime){
            config.time_last_ran = curtime;
            float duration = getDispenseTime(config.amount);
            pthread_mutex_unlock(&config_mutex);
            //getWeather
            //if(evaluateWeather)
            runIrrigation(duration);
        }
        else if(config.mode == MANUAL && config.amount_immidiate > 0){
            float duration = getDispenseTime(config.amount_immidiate);
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
    free(request_params.api_key);
    free(request_params.coords);
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
