#include <wiringPi.h>

#define RELAY_1_PIN_NUMBER 22
#define RELAY_2_PIN_NUMBER 5

#define GPIO_ERR 101

#ifndef DEVICE_PIN_NUMBER
#define DEVICE_PIN_NUMBER 5
#endif

//initializes the GPIO pin interface
int initGpioPinControl();

//returns true if the filtration is on
bool checkDeviceState();

//runs the filtration for a certain amount of time
void runIrrigation(float duration);

//turns off the filtration
void shutdownWaterPump();

//turns on the filtration
void launchWaterPump();
