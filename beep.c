#include "logError.h"

#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>

#define TAU	(M_PI * 2)
#define AMPLITUDE 3000
#define DSP_FREQUENCEY 48000
#define TONE (440 * 2)
	
SDL_AudioSpec beepDesired;
SDL_AudioSpec beepObtained;
SDL_AudioDeviceID dev;
static Uint8 silence;

/* This is the callback used by SDL Audio Device
 * for generating a sine wave */
void loadStream(void *userdata, Uint8 *stream, int len);

/* An SDL Audio Device is created and it's ID 
 * is set to _dev_ then it is "paused" meaning
 * it will not play any audio. */
int initAudio(){

	if( SDL_Init(SDL_INIT_AUDIO) < 0 ){
		setError("SDL_Init: %s", SDL_GetError() );
		return -1;
	}

	SDL_memset(&beepDesired, 0, sizeof(beepDesired) );

	beepDesired.freq = DSP_FREQUENCEY;
	beepDesired.format = AUDIO_S16SYS;
	beepDesired.channels = 1;
	beepDesired.samples = 4096;
	beepDesired.callback = loadStream;

	if( (dev = SDL_OpenAudioDevice(NULL, 0, &beepDesired, &beepObtained, 0) ) == 0){
		setError("SDL_OpenAudio: %s", SDL_GetError() );
		return -1;
	}

	silence = beepObtained.silence;

	SDL_PauseAudioDevice(dev, 1);
	return 0;
}

int cycleSamples = ceil(DSP_FREQUENCEY / TONE);
int cycleIndex = 0;

/* A simple _TONE_ Hz sine wave is generated
 * and written to __stream_ */
void loadStream(void *userdata, Uint8 *_stream, int len) {

	Sint16 *stream = (Sint16*) _stream;

	SDL_memset(stream, silence, 4096 * 2);

	int samplesToWrite = len / sizeof(Sint16);

	for(int sampleIndex = 0; 
		sampleIndex < samplesToWrite; 
		sampleIndex++)
	{
		stream[sampleIndex] = 10000 * sin( (TAU * cycleIndex * TONE) / (float)DSP_FREQUENCEY );
		cycleIndex = (cycleIndex + 1) % cycleSamples;
	}
}

/* The SDL Audio Device _dev_ is "unpaused" so it 
 * will start calling _loadStream_ to generate a 
 * sine wave then play it. */
int beepStart(void) {
	SDL_PauseAudioDevice(dev, 0);
}

/* The SDL Audio Device _dev_ is "paused" so it 
 * will stop calling _loadStream_ to generate audio
 * and stop playing. */
int beepStop(void){
	SDL_PauseAudioDevice(dev, 1);
}

/* SDL Audio is closed */
int closeAudio(void){
	SDL_CloseAudio();
}
