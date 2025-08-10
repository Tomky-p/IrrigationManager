#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define LOG_PATH "../logs/"

#define IO_ERR -1

#define INFO "[INFO]"
#define ACTION "[ACTION]"
#define ERROR "[ERROR]"

int initLogger();

void log_(const char *level, const char *msg_format, ...);

void logConfigChange();