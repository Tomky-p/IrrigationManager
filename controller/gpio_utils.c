#include "utils.h"
#include "gpio_utils.h"
#include <time.h>
#include <wiringPi.h>
#include <stdio.h>

int initGpioPinControl(){
    int ret = wiringPiSetupGpio();
    pinMode(DEVICE_PIN_NUMBER, OUTPUT);
    return ret;
}

bool checkDeviceState(){
    if(digitalRead(DEVICE_PIN_NUMBER) == HIGH) return true;
    else return false;
}

void runIrrigation(int duration){
    int in_miliseconds = ((duration*60)*60)*1000;
    launchWaterPump();
    pthread_mutex_lock(&config_mutex);
    while (config.dispensing && in_miliseconds > 0)
    {
        pthread_mutex_unlock(&config_mutex);
        delay(1000);
        in_miliseconds = in_miliseconds - 1000;
        pthread_mutex_lock(&config_mutex);
    }
    pthread_mutex_unlock(&config_mutex);
    if(checkDeviceState()){
        shutdownWaterPump();
    }
}

void shutdownWaterPump(){
    pthread_mutex_lock(&config_mutex);
    digitalWrite(DEVICE_PIN_NUMBER, LOW);
    printf("Turned off.\n");
    config.dispensing = false;
    pthread_mutex_unlock(&config_mutex);
}

void launchWaterPump(){
    pthread_mutex_lock(&config_mutex);
    digitalWrite(DEVICE_PIN_NUMBER, HIGH);
    printf("Turned on.\n");
    config.dispensing = true;
    pthread_mutex_unlock(&config_mutex);
}
