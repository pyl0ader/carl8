#include "video.h"
#include "input.h"
#include "interpreter.h"
#include "logError.h"

#define TITLE "carl8"
#define USAGE "usage: carl8 [-d] ROM"

void die(void);

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
    FILE *load;

    if(argc != 2 && argc != 3){
        setError(USAGE);
        presentErrorLog();
        die();
    } 

    if(argc == 3 ? initializeInterpreter(argv[2]) < 0 :  initializeInterpreter(argv[1]) < 0){
        setError("initializeInterpreter: %s", getError());
        presentErrorLog();
        die();
    }

    if( argc == 3 ){
        if(strcmp(argv[1], "-d") != 0){ 
            setError(USAGE);
            presentErrorLog();
            die();
        }

        if(disassemble() < 0){
            setError("disassemble: %s", getError());
            presentErrorLog();
            die();
        }
        exit(0);
    }
    
    if(initializeVideo(TITLE) < 0){
        setError("initializeVideo: %s", getError());
        presentErrorLog();
        die();
    }

    initializeInput();

    while(!action.quit){
        inputProcess();
        /*
        for(int i=0; i < 16; i++){
            screen[i] = (action.interpreterInput & (1 << i)) > 0; 
        }
        */
        step();
        if(videoProcess(screen) < 0){
            setError("video: %s", getError());
            die();
        }
    }

    closeVideo();
}

#include <stdarg.h>

void die(void)
{
    closeVideo();
    exit(1);
}
