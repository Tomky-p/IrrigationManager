#include <stdio.h>
#include <stdbool.h> 
#include <curl/curl.h> 
#include <json-c/json.h>
#include <time.h>
#include "weather_utils.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

size_t writeToBuffer(void *data, size_t size, size_t nmemb, void *userdata){
    size_t real_size = size * nmemb;
    Response_data *raw_data = (Response_data*)userdata;
    char *tmp = realloc(raw_data->data, raw_data->size + real_size + 1);
    if(tmp == NULL){
        fprintf(stderr, "%s", ALLOC_ERR_MSG);
        return CURL_WRITEFUNC_ERROR;
    }
    raw_data->data = tmp;
    memcpy(&(raw_data->data[raw_data->size]), data, real_size);
    raw_data->size += real_size;
    raw_data->data[raw_data->size] = '\0';
    return real_size;
}

int sendAPIRequest(char *url, struct json_object **weather_data){
    CURL *curl;
    CURLcode res;
    Response_data raw_data;
    raw_data.data = malloc(1);
    raw_data.size = 0;

    //init CURL
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(curl == NULL){
        fprintf(stderr, HTTP_INIT_ERR_MSG);
        curl_easy_cleanup(curl);
        free(raw_data.data);
        return HTTP_INIT_ERR;
    }
    //format URL
    printf("API call URL: %s\n", url);
    //set options
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToBuffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&raw_data);
    //perform request
    res = curl_easy_perform(curl);
    if(res != CURLE_OK){
        fprintf(stderr, "CURL API request returned ERR: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        free(raw_data.data);
        return CURL_REQUEST_ERR;
    }
    //parse and store data into json object
    *weather_data = json_tokener_parse(raw_data.data);
    //print_json_object(*weather_data);
    curl_easy_cleanup(curl);
    free(raw_data.data);
    return EXIT_SUCCESS;
}

int getCurrentWeather(struct json_object **weather_data, req_params_t *params){
    //build the URL
    char url[200];
    uint16_t lenght = snprintf(url, sizeof(url),"%s%s%s%s%s", BASE_URL_PRESENT, params->api_key, URL_QUERY, params->coords, URL_NO_AQ);
    if(lenght > sizeof(url)){
        fprintf(stderr, "%s", URL_COPY_ERR);
        return HTTP_INIT_ERR;
    }
    return sendAPIRequest(url, weather_data);
}

int getWeatherForecast(struct json_object **weather_data, uint8_t days, req_params_t *params){
    char number[2];
    uint16_t num_lenght = snprintf(number, sizeof(number),"%d", days);
    char url[250];
    uint16_t lenght = snprintf(url, sizeof(url),"%s%s%s%s%s%s%s%s", BASE_URL_FORECAST, params->api_key, URL_QUERY, params->coords, URL_NO_OF_DAYS, number, URL_NO_AQ, URL_NO_ALERTS);
    if(lenght > sizeof(url) || num_lenght > sizeof(number)){
        fprintf(stderr, "%s", URL_COPY_ERR);
        return HTTP_INIT_ERR;
    }
    return sendAPIRequest(url, weather_data);
}

int evaluateWeatherData(req_params_t *params, int curtime, bool warn){
    //get relevant time values
    int next_hour = getNextTime(curtime);
    next_hour = (next_hour % 100 > 30) ? ((next_hour/100) + 1) % 24 : (next_hour/100);
    curtime = (curtime % 100 > 30) ? ((curtime/100) + 1) % 24 : (curtime/100);
    if(next_hour < curtime) next_hour = 24 + next_hour;

    //get weather
    struct json_object *weather_data;
    int ret = getWeatherForecast(&weather_data, DAYS_TO_FORECAST, params);
    if(ret != EXIT_SUCCESS) return ret;

    //check for API err
    struct json_object *error;
    if(json_object_object_get_ex(weather_data, "error", &error)){
        struct json_object *code;
        json_object_object_get_ex(error, "code", &code);
        int err_code = json_object_get_int(code);
        fprintf(stderr, API_ERR_RESP_MSG, err_code, getErrMessage(err_code));
        json_object_put(weather_data);
        return err_code;
    }
    //disect data
    struct json_object *forecast = NULL;
    struct json_object *forecast_days = NULL;
    struct json_object *forecast_day = NULL;
    struct json_object *totalprecip = NULL;
    struct json_object *hours = NULL;
    struct json_object *hour = NULL;
    struct json_object *time_date = NULL;
    struct json_object *chance = NULL;
    
    //unpack forecast data
    if(json_object_object_get_ex(weather_data, "forecast", &forecast) != true) {
        json_object_put(weather_data);
        return JSON_READ_ERR;
    }
    if(json_object_object_get_ex(forecast, "forecastday", &forecast_days) != true) {
        json_object_put(weather_data);
        return JSON_READ_ERR;
    }
    //get precipation and hours of rain in between cycles
    double totalprecip_mm = 0;
    int rain_hours = 0;
    int interval_hours = 0;
    for (size_t i = 0; i < json_object_array_length(forecast_days); i++)
    {
        forecast_day = json_object_array_get_idx(forecast_days, i);
        if(forecast_day == NULL || json_object_object_get_ex(forecast_day, "hour", &hours) != true){
            json_object_put(weather_data);
            return JSON_READ_ERR;
        }
        int begin_at = (i == 0)? curtime : 0;  
        for (size_t p = begin_at; p < json_object_array_length(hours); p++)
        {
            if(interval_hours > next_hour - curtime) break;
            hour = json_object_array_get_idx(hours, p);
            interval_hours++;

            if(hour == NULL || json_object_object_get_ex(hour, "time", &time_date) != true){
                json_object_put(weather_data);
                return JSON_READ_ERR;
            }
            if(json_object_object_get_ex(hour, "precip_mm", &totalprecip) != true){
                json_object_put(weather_data);
                return JSON_READ_ERR;
            }
            if(json_object_object_get_ex(hour, "chance_of_rain", &chance) != true){
                json_object_put(weather_data);
                return JSON_READ_ERR;
            }
            printf("Date and time: %s\n       Precip in mm: %.2f, Chance of rain: %d\n", json_object_get_string(time_date), json_object_get_double(totalprecip), json_object_get_int(chance));
            if(json_object_get_int(chance) >= RAIN_CHANCE_THRESHOLD){
                rain_hours++;
            }
            totalprecip_mm += json_object_get_double(totalprecip);
        }        
    }
    printf("Total precip: %f Interval: %d\n", totalprecip_mm, interval_hours);

    //check if it will rain 60% of the time between irrigation cycles and weather the amount of rainfall and
        // the previously dispensed water has surpassed the recommended amount
    if(checkIrrigationLevel(totalprecip_mm) || (float)rain_hours > ((float)interval_hours*0.6)){
        printf("Postponed irrigation due to suspected rainfall.\n");
        json_object_put(weather_data);
        return NO;
    }
    printf("Weather is dry beginning dispensing...\n");
    
    //print_json_object(weather_data);
    
    //destroy json objects
    json_object_put(weather_data);
    return YES;
}

int getRequestData(req_params_t *req_params){
    int api_file = readDataFromFile(API_KEY_FILENAME, &req_params->api_key);
    int coords_file = readDataFromFile(LOCATION_FILENAME, &req_params->coords);

    if(api_file == ALLOCATION_ERR || coords_file == ALLOCATION_ERR) return ALLOCATION_ERR;
    if(api_file == FILE_NOT_FOUND_ERR || coords_file == FILE_NOT_FOUND_ERR) return FILE_NOT_FOUND_ERR;

    CURL *curl = curl_easy_init();
    char *formated_url = curl_easy_escape(curl, req_params->coords, 0);
    if(formated_url == NULL){
        fprintf(stderr, "%s", ALLOC_ERR_MSG);
        curl_easy_cleanup(curl);
        return ALLOCATION_ERR;
    }
    free(req_params->coords);
    req_params->coords = formated_url;
    curl_easy_cleanup(curl);
    return EXIT_SUCCESS;
}


int readDataFromFile(char *filename, char **buffer){
    FILE *data_file = fopen(filename, "r");
    if(data_file == NULL){
        fprintf(stderr, FILE_NOT_FOUND_ERR_MSG ,filename);
        return FILE_NOT_FOUND_ERR;
    }
    int buf_capacity = API_KEY_LENGHT;
    *buffer = (char*)malloc((buf_capacity + 1) * sizeof(char));
    if(buffer == NULL){
        fprintf(stderr, ALLOC_ERR_MSG);
        return ALLOCATION_ERR;
    }
    char c;
    int i = 0;
    while ((c = fgetc(data_file)) != '\n' && c != EOF)
    {
        if(i >= buf_capacity){
            buf_capacity = buf_capacity*2;
            char *tmp = (char*)realloc(*buffer, buf_capacity+1);
            if(tmp == NULL){
                fprintf(stderr, ALLOC_ERR_MSG);
                fclose(data_file);
                return ALLOCATION_ERR;
            }
            *buffer = tmp;
        }
        (*buffer)[i] = c;
        i++;
    }
    (*buffer)[i] = '\0';
    fclose(data_file);
    return EXIT_SUCCESS;
}

void print_json_object(struct json_object *json) {
    const char *json_str = json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY);
    printf("%s\n", json_str);
}

int checkIrrigationLevel(double precip){
    pthread_mutex_lock(&config_mutex);
    //NOTE: It is assumed that the recommended amount per day is equal to the needed precipation per day (1 mm of precipation = 1 liter of water per square meter)
    float till_suffient_amount = RECOMMENDED_AMOUNT - config.amount_dispensed - ((precip * RECOMMENDED_AMOUNT) / NEEDED_PRECIP_PER_DAY); //this is the conversion serves as conversion from mm of precip to liters
    pthread_mutex_unlock(&config_mutex);
    if(till_suffient_amount <= 0){
        return YES;
    }
    return NO;
}

const char *getErrMessage(int err_code){
    switch (err_code)
    {
    case NO_API_KEY_ERR:
        return "API key not provided.";
    case NO_QUERY_ERR:
        return "Parameter 'q' not provided.";
    case URL_INVALID:
        return "API request url is invalid";
    case LOCATION_NOT_FOUND:
        return "No location found matching parameter 'q'";
    case INVALID_API_KEY:
        return "API key provided is invalid";
    case EXCEEDED_QUOTA:
        return "API key has exceeded calls per month quota.";
    case API_KEY_DISABLED:
        return "API key has been disabled.";
    case SERVICE_NOT_AVAILABLE:
        return "API key does not have access to the resource. Please check pricing page for what is allowed in your API subscription plan.";
    case ENCODING_INVALID:
        return "Json body passed in bulk request is invalid. Please make sure it is valid json with utf-8 encoding.";
    case INTERNAL_ERR:
        return "Internal application error.";
    default:
        return "Unknown API ERR.";
    }

}


