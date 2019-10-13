#include <stdint.h>

#include "input.h"

struct Action action = {0, 0};
static uint8_t map[256];

#include <SDL2/SDL_events.h>
#include <string.h>

static SDL_Event e;

int initializeInput(void)
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

    return 0;
}

int inputProcess()
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
                if( !(action.hexKey & (0b1 << key)) && e.type == SDL_KEYDOWN
                ||    action.hexKey & (0b1 << key)  && e.type == SDL_KEYUP){
                    action.hexKey ^= 0b1 << key;
                }
            }
        }
    }

    return 0;
}
