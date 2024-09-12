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

int sendAPIRequest(char *url, struct json_object *weather_data){
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
        return HTTP_INIT_ERR;
    }
    //set options
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToBuffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&raw_data);
    //perform request
    res = curl_easy_perform(curl);
    if(res != CURLE_OK){
        fprintf(stderr, "API returned ERR: %s", curl_easy_strerror(res));
        return res;
    }
    //parse and store data into json object
    weather_data = json_tokener_parse(raw_data.data);
    curl_easy_cleanup(curl);
    free(raw_data.data);
    return EXIT_SUCCESS;
}

int getCurrentWeather(struct json_object *weather_data, req_params_t *params){
    //build the URL
    char url[200];
    uint16_t lenght = snprintf(url, sizeof(url),"%s%s%s%s%s", BASE_URL_PRESENT, params->api_key, URL_QUERY, params->coords, URL_NO_AQ);
    if(lenght > sizeof(url)){
        fprintf(stderr, "%s", URL_COPY_ERR);
        return HTTP_INIT_ERR;
    }
    printf("API call URL: %s\n", url);
    return sendAPIRequest(url, weather_data);
}

int getWeatherForecast(struct json_object *weather_data, uint8_t days, req_params_t *params){
    char number[2];
    uint16_t num_lenght = snprintf(number, sizeof(number),"%d", days);
    char url[250];
    uint16_t lenght = snprintf(url, sizeof(url),"%s%s%s%s%s%s%s%s", BASE_URL_FORECAST, params->api_key, URL_QUERY, params->coords, URL_NO_OF_DAYS, number, URL_NO_AQ, URL_NO_ALERTS);
    if(lenght > sizeof(url) || num_lenght > sizeof(number)){
        fprintf(stderr, "%s", URL_COPY_ERR);
        return HTTP_INIT_ERR;
    }
    printf("API call URL: %s\n", url);
    return sendAPIRequest(url, weather_data);
}

int evaluateWeatherData(struct json_object *weather_data, req_params_t *params){
    return true;
}

int getRequestData(req_params_t *req_params){
    int api_file = readDataFromFile(API_KEY_FILENAME, &req_params->api_key);
    int coords_file = readDataFromFile(LOCATION_FILENAME, &req_params->coords);

    if(api_file == ALLOCATION_ERR || coords_file == ALLOCATION_ERR) return ALLOCATION_ERR;
    if(api_file == FILE_NOT_FOUND_ERR || coords_file == FILE_NOT_FOUND_ERR) return FILE_NOT_FOUND_ERR;

    return EXIT_SUCCESS;
}


int readDataFromFile(char *filename, char **buffer){
    FILE *data_file = fopen(filename, "r");
    if(data_file == NULL){
        fprintf(stderr, FILE_NOT_FOUND_ERR_MSG ,filename);
        return FILE_NOT_FOUND_ERR;
    }
    int buf_capacity = API_KEY_LENGHT + 1;
    *buffer = (char*)malloc(buf_capacity * sizeof(char));
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


