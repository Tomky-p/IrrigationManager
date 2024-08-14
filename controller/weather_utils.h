#include <stdio.h> 
#include <curl/curl.h> 
#include <json-c/json.h>

bool getCurrentWeather(struct json_object *weather_data);

bool getWeatherForecast(struct json_object *weather_data, uint8_t days);

bool evaluateWeatherData(struct json_object *weather_data);
