#include "video.h"
#include "input.h"
#include "interpreter.h"
#include "error.h"

#define TITLE "chipper"

void die(const char* fmt, ...);

#include <stdio.h>

int main(int argc, char** argv)
{
    FILE *load;

    if(argc != 2){
        die("usage: chip8 ROM");
    } 

    if(initializeInterpreter(*++argv) < 0){
        die("initializeInterpreter: %s", getError());
    }

    disassemble();

    if(initializeVideo(TITLE) < 0){
        die("InitializeVideo: %s", getError());
    }

    initializeInput();

    while(!action.quit){
        inputProcess();
        for(int i=0; i < 16; i++){
            screen[i] = (action.hexKey & (0b1 << i)) > 0; 
        }
        if(videoProcess(screen) < 0){
            die("video: %s", getError());
        }
    }

    closeVideo();
}

#include <stdarg.h>
#include <stdlib.h>

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
