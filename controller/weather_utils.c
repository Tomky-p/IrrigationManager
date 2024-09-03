#include <stdio.h>
#include <stdbool.h> 
#include <curl/curl.h> 
#include <json-c/json.h>
#include <time.h>
#include "weather_utils.h"
#include "utils.h"

int getCurrentWeather(struct json_object *weather_data, request_data_t *req_data){
    return true;
}

int getWeatherForecast(struct json_object *weather_data, uint8_t days, request_data_t *req_data){
    return true;

}

int evaluateWeatherData(struct json_object *weather_data, request_data_t *req_data){
    return true;
}

int getAPIkey(char buffer[]){
    FILE *api_key_file = fopen("r", API_KEY_FILENAME);
    if(api_key_file == NULL){
        fprintf(stderr, FILE_NOT_FOUND_ERR_MSG ,API_KEY_FILENAME);
        return FILE_NOT_FOUND_ERR;
    }
    char c;
    char buffer[API_KEY_LENGHT] = {0};
    for (size_t i = 0; i < API_KEY_LENGHT; i++)
    {
        c = fgetc(api_key_file);
        if(c == '\n' || c == '\0' || c == EOF){
            fprintf(stderr, NO_API_KEY_ERR_MSG);
            return NO_API_KEY_ERR;
        }
        buffer[i] = c;
    }
    return EXIT_SUCCESS;
}

int getURL(){

}
