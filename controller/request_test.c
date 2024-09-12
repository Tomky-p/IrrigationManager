#include "weather_utils.h"
#include <stdio.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "utils.h"

int main(){
    struct json_object *container;
    req_params_t request_params;
    request_params.api_key, request_params.coords = NULL;
    if((getRequestData(&request_params)) != EXIT_SUCCESS){
        fprintf(stderr, "Did not get api key or coords from file.\n");
        return EXIT_FAILURE;
    }
    getCurrentWeather(container, &request_params);
    
}