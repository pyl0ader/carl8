#include "input.h"

struct Action action = {0, 0};
static unsigned char map[256];

#include <SDL2/SDL_events.h>
#include <string.h>

static SDL_Event e;

void initializeInput(void)
{
    memset(map, 0xff, sizeof map);

    map[(unsigned char)SDLK_1] = 0x1;
    map[(unsigned char)SDLK_2] = 0x2;
    map[(unsigned char)SDLK_3] = 0x3;
    map[(unsigned char)SDLK_4] = 0xc;
    
    map[(unsigned char)SDLK_q] = 0x4;
    map[(unsigned char)SDLK_w] = 0x5;
    map[(unsigned char)SDLK_e] = 0x6;
    map[(unsigned char)SDLK_r] = 0xd;
    
    map[(unsigned char)SDLK_a] = 0x7;
    map[(unsigned char)SDLK_s] = 0x8;
    map[(unsigned char)SDLK_d] = 0x9;
    map[(unsigned char)SDLK_f] = 0xe;
    
    map[(unsigned char)SDLK_z] = 0xa;
    map[(unsigned char)SDLK_x] = 0x0;
    map[(unsigned char)SDLK_c] = 0xb;
    map[(unsigned char)SDLK_v] = 0xf;
}

void inputProcess()
{
    unsigned char key;

    while(SDL_PollEvent(&e)){
        if(e.type == SDL_QUIT){
            action.quit = 1;
        }
        else if(e.type == SDL_KEYDOWN || e.type == SDL_KEYUP){
            key = map[(unsigned char)e.key.keysym.sym];
            if(key != 0xff){
                key &= 0xf;
                if( !(action.hexKey & (0b1 << key)) && e.type == SDL_KEYDOWN 
                ||    action.hexKey & (0b1 << key)  && e.type == SDL_KEYUP){
                    action.hexKey ^= 0b1 << key;
                }
            }
        }
    }
}
