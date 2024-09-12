#include <stdio.h> 
#include <curl/curl.h> 
#include <json-c/json.h>

#ifndef API_KEY_FILENAME
//file where the api key is stored 
#define API_KEY_FILENAME "api_key.txt"
#endif

#define API_KEY_LENGHT 31

#ifndef LOCATION_FILENAME
//Geographic location coordinates used to pull weather data
#define LOCATION_FILENAME "coords.txt"
#endif

#ifndef BASE_URL_FORECAST 
#define BASE_URL_FORECAST "http://api.weatherapi.com/v1/forecast.json?key="
#endif

#ifndef BASE_URL_PRESENT 
#define BASE_URL_PRESENT "http://api.weatherapi.com/v1/current.json?key="
#endif

#define URL_QUERY "&q="
#define URL_NO_AQ "&aqi=no"
#define URL_NO_OF_DAYS "&days="
#define URL_NO_ALERTS "&alerts=no"

#ifndef DAYS_TO_FORECAST
#define DAYS_TO_FORECAST 1
#endif

#define FILE_NOT_FOUND_ERR -1
#define HTTP_INIT_ERR -2
#define CURL_WRITEFUNC_ERROR 0
#define NO_API_KEY_ERR 401	//1002	//API key not provided.
#define NO_QUERY_ERR 400	//1003	//Parameter 'q' not provided.
#define URL_INVALID 400	//1005	//API request url is invalid
#define LOCATION_NOT_FOUND 400	//1006	//No location found matching parameter 'q'
#define INVALID_API_KEY 401	//2006	//API key provided is invalid
#define EXCEEDED_QUOTA 403	//2007	//API key has exceeded calls per month quota.
#define API_KEY_DISABLED 403	//2008	//API key has been disabled.
#define SERVICE_NOT_AVAILABLE 403	//2009	//API key does not have access to the resource. Please check pricing page for what is allowed in your API subscription plan.
#define ENCODING_INVALID 400	//9000	//Json body passed in bulk request is invalid. Please make sure it is valid json with utf-8 encoding.
#define INTERNAL_ERR 400	//9999	//Internal application error.

#define NO_API_KEY_ERR_MSG "ERR! No API key provided.\n"
#define HTTP_INIT_ERR_MSG "CRITICAL ERR! Failed to initialize HTTP requests.\n"
#define URL_COPY_ERR "ERR! Failed to parse URL.\n"

typedef struct{
    char *api_key;
    char *coords;
}req_params_t;

typedef struct {
    char *data;
    size_t size;
}Response_data;

//makes an API request to get weather data for today
int getCurrentWeather(struct json_object *weather_data, req_params_t *req_data);

//makes an API request to get forecast for a specified number of days
int getWeatherForecast(struct json_object *weather_data, uint8_t days, req_params_t *req_data);

//process a json that is the result of the weather API call
int evaluateWeatherData(struct json_object *weather_data, req_params_t *req_data);

//loads API key and coordinates from file
int getRequestData(req_params_t *req_params);

//reads data from file and stores inside buffer
int readDataFromFile(char *filename, char **buffer);

//makes API request based on the provided URL
int sendAPIRequest(char *url, struct json_object *weather_data);

