#include <stdint.h>

#include "input.h"

/* variables */
struct Action action = {0, 0};
static uint8_t map[256];

#include <SDL2/SDL_events.h>
#include <string.h>

static SDL_Event e;

/* function definitions */

/* map is set to all ones, integeral SDL Keysym indices in map are set
 * to a corrosponding key value. */
void initializeInput(void)
{
	memset(map, 0xff, sizeof map);

	map[(uint8_t)SDLK_1] = 0x1;
	map[(uint8_t)SDLK_2] = 0x2;
	map[(uint8_t)SDLK_3] = 0x3;
	map[(uint8_t)SDLK_4] = 0xc;
	
	map[(uint8_t)SDLK_q] = 0x4;
	map[(uint8_t)SDLK_w] = 0x5;
	map[(uint8_t)SDLK_e] = 0x6;
	map[(uint8_t)SDLK_r] = 0xd;
	
	map[(uint8_t)SDLK_a] = 0x7;
	map[(uint8_t)SDLK_s] = 0x8;
	map[(uint8_t)SDLK_d] = 0x9;
	map[(uint8_t)SDLK_f] = 0xe;
	
	map[(uint8_t)SDLK_z] = 0xa;
	map[(uint8_t)SDLK_x] = 0x0;
	map[(uint8_t)SDLK_c] = 0xb;
	map[(uint8_t)SDLK_v] = 0xf;
}

/* For every event in the SDL event queue, bits corrosponding to a mapped key
 * in the action.interpreterInput sequence are 
 * updated upon press and release events. */
void inputProcess()
{
	uint8_t key;

	while(SDL_PollEvent(&e)){
		if(e.type == SDL_QUIT){
			action.quit = 1;
		}
		else if(e.type == SDL_KEYDOWN || e.type == SDL_KEYUP){
			key = map[(uint8_t)e.key.keysym.sym];
			if(key != 0xff){
				key &= 0xf;

				/* update bit if the corrosponding key is pressed or released */
				if( !(action.interpreterInput & (0b1 << key)) && e.type == SDL_KEYDOWN
				||	action.interpreterInput & (0b1 << key)  && e.type == SDL_KEYUP){
					action.interpreterInput ^= 0b1 << key;
				}
			}
		}
	}

}
