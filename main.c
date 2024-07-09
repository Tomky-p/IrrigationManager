#include <stdio.h>
#include <pthread.h>
#include <utils.h>
#include <string.h>

int main(void){
    pthread_t irrigation;

    char *command;
    
    while (readCmd())
    {
        if(strcmp(command, "run")){

        }
    }
    
    return 0;
}