#include "weather_utils.h"
#include <stdio.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "utils.h"

int main(){
    struct json_object *container = NULL;
    req_params_t request_params;
    request_params.api_key = NULL;
    request_params.coords = NULL;
    if((getRequestData(&request_params)) != EXIT_SUCCESS){
        fprintf(stderr, "Did not get api key or coords from file.\n");
        return EXIT_FAILURE;
    }
    getCurrentWeather(&container, &request_params);
    if (container != NULL) {
        json_object_put(container);
    }
    container = NULL;
    getWeatherForecast(&container, 3, &request_params);
    free(request_params.api_key);
    free(request_params.coords);
    if (container != NULL) {
        json_object_put(container);
    }
    return 0;
}
