#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "logger.h"
#include "utils.h"

static char CURRENT_PATH[100];

void log_(const char *level, const char *msg_format, ...) {
    FILE *log = fopen(CURRENT_PATH, "a");
    if(!log) {
        fprintf(stderr, "Failed to log!\n");
        return;
    }
    char msg_buf[512]; //msg
    char final_buf[600]; // time level msg

    va_list args;
    va_start(args, msg_format);
    vsnprintf(msg_buf, sizeof(msg_buf), msg_format, args);
    va_end(args);

    snprintf(final_buf, sizeof(final_buf), "%s %s %s\n",
             getCurrentDateTime(), level, msg_buf);

    fwrite(final_buf, 1, strlen(final_buf), log);
    fclose(log);

}


void logConfigChange(){
    //TO DO: implement this
    return;
}

int createSessionLogFile(const char *date_time){
    FILE *log = fopen(CURRENT_PATH, "w");
    if(!log) return IO_ERR;
    
    char init_msg[100];
    snprintf(init_msg, sizeof(init_msg), "%s %s Logger initialized!\n", date_time, INFO);

    size_t written = fwrite(init_msg, sizeof(char), strlen(init_msg), log);
    if(written != sizeof(char) * strlen(init_msg)) { 
        fclose(log);
        return IO_ERR;
    }
    fclose(log);
    return EXIT_SUCCESS;
}

int initLogger() {
    const char* date_time = getCurrentDateTime();
    if(!date_time) return TIME_ERR;

    snprintf(CURRENT_PATH, sizeof(CURRENT_PATH), "%slog%s.txt", LOG_PATH, date_time);
    return createSessionLogFile(date_time);
}
