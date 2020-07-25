#include "video.h"
#include "input.h"
#include "interpreter.h"
#include "assembly.h"
#include "logError.h"

#define TITLE "carl8"
#define USAGE "usage: carl8 [-d] ROM"

void die(void);

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define CARL8R8 (1 / 60)

int main(int argc, char** argv)
{
	FILE *load;

	if(argc != 2 && argc != 3){
		setError(USAGE);
		presentErrorLog();
		die();
	} 

	interp_initialize();

	if(argc == 3 && strcmp(argv[1], "-d") == 0 ){
		if( interp_loadRom(argv[2]) < 0 ){
			setError("could not load \"%s\": %s", argv[2], getError());
			presentErrorLog();
			die();
		}
		if( assm_disassemble() < 0 ){
			setError("could not disassemble \"%s\": %s", argv[2], getError());
			presentErrorLog();
			die();
		}
		
	}
	else if (argc == 3 && strcmp(argv[1], "-a") == 0 ){
		if( assm_assemble(argv[2]) < 0 ){
			setError("could not assemble \"%s\": %s", argv[2], getError());
			presentErrorLog();
			die();
		}
	}
	else if (argc == 3 && strcmp(argv[1], "-r") == 0){
		if( assm_assemble(argv[2]) < 0 ){
			setError("could not assemble \"%s\": %s", argv[2], getError());
			presentErrorLog();
			die();
		}
		if( assm_disassemble() < 0 ){
			setError("could not re-disassemble \"%s\": %s", argv[2], getError());
			presentErrorLog();
			die();
		}
	}
	else if (argc == 2){

		if(interp_loadRom(argv[1]) < 0){
			setError("could not load %s: %s",
					argv[1],
					getError() );
		}
		
	}
	if(argc == 2 || (argc == 3 && strcmp(argv[1], "-a") == 0) ){

		if(initializeVideo(TITLE) < 0){
			setError("initializeVideo: %s", getError());
			presentErrorLog();
			die();
		}

		initializeInput();

		struct timespec lastTime  = {0, 0};
		struct timespec deltaTime = {0, 0};

		while(!action.quit){

			inputProcess();

			clock_gettime(CLOCK_MONOTONIC, &deltaTime);
			
			if( !(lastTime.tv_sec || lastTime.tv_nsec) ) {
				lastTime  = deltaTime;
				deltaTime.tv_sec = deltaTime.tv_nsec = 0;
			}
			else {
				deltaTime.tv_sec  -= lastTime.tv_sec;
				deltaTime.tv_nsec -= lastTime.tv_nsec;

				lastTime.tv_sec  += deltaTime.tv_sec;
				lastTime.tv_nsec += deltaTime.tv_nsec;

				/* carry over seconds as interpreter only measures nanos */
				deltaTime.tv_nsec += 1000000000 * deltaTime.tv_sec;
			}

			if(interp_step(deltaTime.tv_nsec) < 0){
				setError("interpreter failure: %s", getError() );
				die();
			}
			
		}
		closeVideo();
	}
}

void die(void)
{
	closeVideo();
	exit(1);
}
