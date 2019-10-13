#include "video.h"
#include "input.h"
#include "interpreter.h"
#include "logError.h"

#define TITLE "chipper"
#define USAGE "usage: chipper [-d] ROM"

void die(const char* fmt, ...);

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
    FILE *load;

    if(argc != 2 && argc != 3){
        die(USAGE);
    } 

    if(argc == 3 ? initializeInterpreter(argv[2]) :  initializeInterpreter(argv[1]) < 0){
        die("initializeInterpreter: %s", getError());
    }

    if( argc == 3 ){
        if(strcmp(argv[1], "-d") != 0) die(USAGE);

        if(disassemble() < 0){
            die("disassemble: ", getError());
        }
        exit(0);
    }
    
    if(initializeVideo(TITLE) < 0){
        die("initializeVideo: %s", getError());
    }

    if(initializeInput() < 0){ //currently no error handling for input
        die("initializeInput: %s", getError());
    }

    while(!action.quit){
        if(inputProcess() < 0){
            die("input: %s", getError());
        }
        for(int i=0; i < 16; i++){
            screen[i] = (action.hexKey & (1 << i)) > 0; 
        }
        if(videoProcess(screen) < 0){
            die("video: %s", getError());
        }
    }

    closeVideo();
}

#include <stdarg.h>

void die(const char *fmt, ...)
{
    closeVideo();

	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

    fputc('\n',stderr);

    exit(1);
}
