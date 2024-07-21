#include <stdio.h>
#include <pthread.h>
#include "utils.h"
#include <string.h>

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
    config_t config;
    char *command = NULL;
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

    printf("mode: %d\n", config.mode);
    printf("amount: %d\n", config.amount);
    printf("interval: %d\n", config.interval);

    //irrigation executor thread
    pthread_t irrigationControl;
    pthread_create(&irrigationControl, NULL, irrigationController, (void*)&config);

    //command line control thread
    while (config.running)
    {
        //in the future to be replace by command recieving from the socket from the web interface
        int ret = readCmd(&command);
        if(ret == READING_SUCCESS){
            printf("%s", command);
            processCommand(command, &config);
        }
        if(ret == ALLOCATION_ERR){
            pthread_join(irrigationControl, NULL);
            return ALLOCATION_ERR;
        }
    }

    pthread_join(irrigationControl, NULL);
    if(command != NULL) free(command);
    return EXIT_SUCCESS;
}


void* irrigationController(void *configuration){
    config_t *config = (config_t*)configuration;
    while (true)
    {
        
    }
    
    
    if(config->mode == AUTO){

    }

    return NULL;
}
