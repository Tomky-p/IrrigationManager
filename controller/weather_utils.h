#include <stdio.h> 
#include <curl/curl.h> 
#include <json-c/json.h>

#ifndef API_KEY_FILENAME
//file where the api key is stored 
#define API_KEY_FILENAME "api_key.txt"
#endif

#ifndef LOCATION
//Geographic location name used to pull weather data
#define LOCATION "Poděbrady"
#endif

bool getCurrentWeather(struct json_object *weather_data, char *url);

bool getWeatherForecast(struct json_object *weather_data, uint8_t days, char *url);

bool evaluateWeatherData(struct json_object *weather_data);
