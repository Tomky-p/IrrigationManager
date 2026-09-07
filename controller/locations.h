#include <stdio.h>

#ifndef API_KEY_FILENAME
//file where the api key is stored 
#define API_KEY_FILENAME "api_key.txt"
#endif

#define API_KEY_LENGHT 31

#ifndef LOCATION_FILENAME
//Geographic location coordinates used to pull weather data
#define LOCATION_FILENAME "coords.txt"
#endif

#define INVALID_LOCATION -1


enum Locations {API_KEY, COORDS, NUM_LOCATIONS};

struct Location {
    const char *env_var_name;
    const char *filepath;
};

static const struct Location LOCATIONS[] = {
    {"IRRIGATION_API_KEY", "api_key.txt"},
    {"IRRIGATION_COORDS", "coords.txt"}
};