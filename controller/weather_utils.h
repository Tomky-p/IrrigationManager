#include <stdio.h> 
#include <curl/curl.h> 
#include <json-c/json.h>
#include <stdbool.h>

//HTTP request constants and parameters
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

#ifndef BASE_URL_PAST 
#define BASE_URL_PAST "http://api.weatherapi.com/v1/history.json?key="
#endif

#define URL_QUERY "&q="
#define URL_NO_AQ "&aqi=no"
#define URL_NO_OF_DAYS "&days="
#define URL_NO_ALERTS "&alerts=no"
#define URL_DATE "&dt="

//rainfall constants
#ifndef DAYS_TO_FORECAST
#define DAYS_TO_FORECAST 2
#endif

#ifndef MAX_PRECIP_TODAY_MM
#define MAX_PRECIP_TODAY_MM 10.0
#endif

#ifndef MAX_PRECIP_NEXT_DAYS_MM
#define MAX_PRECIP_NEXT_DAYS_MM 40.0
#endif

#ifndef NEEDED_PRECIP_PER_DAY
#define NEEDED_PRECIP_PER_DAY 10.0
#endif

#ifndef RAIN_CHANCE_THRESHOLD
#define RAIN_CHANCE_THRESHOLD 75
#endif

//Fatal JSON and HTTP request err codes
#define FILE_NOT_FOUND_ERR -1
#define HTTP_INIT_ERR -2
#define CURL_WRITEFUNC_ERR 0
#define JSON_READ_ERR -4
#define CURL_REQUEST_ERR -5

//API err codes
#define NO_API_KEY_ERR 1002	//API key not provided.
#define NO_QUERY_ERR 1003	//Parameter 'q' not provided.
#define URL_INVALID 1005	//API request url is invalid
#define LOCATION_NOT_FOUND 1006	//No location found matching parameter 'q'
#define INVALID_API_KEY 2006	//API key provided is invalid
#define EXCEEDED_QUOTA 2007	//API key has exceeded calls per month quota.
#define API_KEY_DISABLED 2008	//API key has been disabled.
#define SERVICE_NOT_AVAILABLE 2009	//API key does not have access to the resource. Please check pricing page for what is allowed in your API subscription plan.
#define ENCODING_INVALID 9000	//Json body passed in bulk request is invalid. Please make sure it is valid json with utf-8 encoding.
#define INTERNAL_ERR 9999	//Internal application error.

//HTTP request err messages
#define HTTP_INIT_ERR_MSG "CRITICAL ERR! Failed to initialize HTTP requests.\n"
#define URL_COPY_ERR "ERR! Failed to parse URL.\n"
#define API_ERR_RESP_MSG "API returned code: %d, with message: %s\n"

//Used to hold the sensitive API request data
typedef struct{
    char *api_key;
    char *coords;
}req_params_t;

//Used to hold the raw response data
typedef struct {
    char *data;
    size_t size;
}Response_data;

//makes an API request to get weather data for today
int getPreviousWeather(struct json_object **weather_data, req_params_t *params);

//makes an API request to get forecast for a specified number of days
int getWeatherForecast(struct json_object **weather_data, uint8_t days, req_params_t *req_data);

//process a json that is the result of the weather API call
//returns negative values when a fatal err happens, returns 0 and 1 for NO and YES (if irrigation should occur), returns >1000 for api values err
int evaluateWeatherData(req_params_t *req_data, int curtime, int manual_duration);

//loads API key and coordinates from file
int getRequestData(req_params_t *req_params);

//reads data from file and stores inside buffer
int readDataFromFile(char *filename, char **buffer);

//makes API request based on the provided URL
int sendAPIRequest(char *url, struct json_object **weather_data);

//prints the contents of a JSON object
void print_json_object(struct json_object *json);

//returns the proper err message for each API err
const char *getErrMessage(int err_code);

//estimates soil irrigation level based on the amount already dispensed and upcoming rainfall
int checkIrrigationLevel(double precip);

int getPrevDate(char date[]);

int checkAPIErrs(struct json_object **weather_data);

int unpackJson(struct json_object **weather_data, struct json_object **forecast, struct json_object **forecast_days);

int getRainfallData(struct json_object **weather_data, struct json_object **hour, struct json_object **time_date, struct json_object **chance, struct json_object **totalprecip);
