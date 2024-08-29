#include <wiringPi.h>

#define RELAY_1_PIN_NUMBER 22
#define RELAY_2_PIN_NUMBER 5

#define GPIO_ERR 101

#ifndef DEVICE_PIN_NUMBER
#define DEVICE_PIN_NUMBER 5
#endif

//initializes the GPIO pin interface
int initGpioPinControl();

//returns true if the irrigation system is dispensing
bool checkDeviceState();

//dispenses water for a certain amount of time
void runIrrigation(float duration);

//turns off the water pump
void shutdownWaterPump();

//turns on the water pump
void launchWaterPump();
