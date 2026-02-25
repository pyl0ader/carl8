#include <stdint.h>

#include "input.h"

/* variables */
struct Action action = {0, 0};

#include <SDL2/SDL_events.h>
#include <string.h>

static SDL_Event e;

/* function definitions */

static uint8_t input_get_key(SDL_KeyboardEvent keyboard_event)
{
    switch(keyboard_event.keysym.sym){
        case SDLK_1:
            return 0x1;
            break;
        case SDLK_2:
            return 0x2;
            break;
        case SDLK_3:
            return 0x3;
            break;
        case SDLK_4:
            return 0xc;
            break;
        
        case SDLK_q:
            return 0x4;
            break;
        case SDLK_w:
            return 0x5;
            break;
        case SDLK_e:
            return 0x6;
            break;
        case SDLK_r:
            return 0xd;
            break;
        
        case SDLK_a:
            return 0x7;
            break;
        case SDLK_s:
            return 0x8;
            break;
        case SDLK_d:
            return 0x9;
            break;
        case SDLK_f:
            return 0xe;
            break;
        
        case SDLK_z:
            return 0xa;
            break;
        case SDLK_x:
            return 0x0;
            break;
        case SDLK_c:
            return 0xb;
            break;
        case SDLK_v:
            return 0xf;
            break;
        default:
            return 0;
    }
}

/* For every event in the SDL event queue, bits corrosponding to a mapped key
 * in the action.interpreterInput sequence are 
 * updated upon press and release events. */
void inputProcess()
{
	uint8_t k;

	while(SDL_PollEvent(&e)){
		if(e.type == SDL_QUIT){
			action.quit = 1;
            return;
		}

        if(e.type != SDL_KEYDOWN && e.type != SDL_KEYUP)
            return;

        k = input_get_key(e.key);
        if(k != 0)
            if(e.type == SDL_KEYDOWN)
                action.interpreterInput |= 0b1 << k;
            else if(e.type == SDL_KEYUP)
                action.interpreterInput ^= 0b1 << k;
        return;
	}
}
