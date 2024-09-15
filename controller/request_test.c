#include "weather_utils.h"
#include <stdio.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "utils.h"

int main(){
    req_params_t request_params;
    request_params.api_key = NULL;
    request_params.coords = NULL;
    int curtime = getCurrentTime();

    config.amount = 50;
    config.mode = AUTO;
    config.times_per_day = 2;
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
    config.amount_dispensed = 0;
    config.time_routine[0] = (uint16_t)curtime;

    if((getRequestData(&request_params)) != EXIT_SUCCESS){
        fprintf(stderr, "Did not get api key or coords from file.\n");
        return EXIT_FAILURE;
    }
    //int next_hour = getNextTime(curtime);
    //printf("Next cycle: %d\n", next_hour);
    //printf("Current hour: %d\n", curtime);
    int ret = evaluateWeatherData(&request_params, curtime, true);
    printf("Request response code: %d\n", ret);

    free(request_params.api_key);
    free(request_params.coords);
    free(config.time_routine);
    return 0;
}
