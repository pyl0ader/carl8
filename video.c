#include <SDL2/SDL.h>

#include "video.h"
#include "logError.h"

/* variables */
const unsigned short WIDTH = 640;
const unsigned short HEIGHT = 480;

static SDL_Window *window;
static SDL_Renderer *renderer;

/* function definitions */

/* SDL video is initialized. An SDL_Window is created with the title name
 * and window is set to it. An SDL_Renderer is created and renderer is set to it.
 * Return value is -1 if errors occur, otherwise it is 0. */
int initializeVideo(const char* name)
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        setError("SDL_Init: %s", SDL_GetError());
        return -1;
    }
    window = SDL_CreateWindow(name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, 0);
    if(window == NULL){
        setError("SDL_CreateWindow: %s", SDL_GetError());
        return -1;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
    if(renderer == NULL){
        setError("SDL_CreateRenderer: %s", SDL_GetError());
        return -1;
    }

    if(clear()  < 0){
        setError("clear: %s", getError());
        return -1;
    }

    return 0;
}

/* For every nonzero element of screen, using renderer, a white filled Rectangle 
 * is rendered at that element's index in a 64 x 32 representation of screen. 
 * Rectangles are stretched/scaled with respect to the dimensions of window.
 * Return value is -1 if errors occur, otherwise it is 0. */
int videoProcess(unsigned char back[64 * 32])
{
    int x, y, i;
    SDL_Rect pixel;

    pixel.w = WIDTH / 64;
    pixel.h = HEIGHT / 32;

    static unsigned char front[64 * 32] = {};
    unsigned char changed = 0;

    for(y = 0; y < 32; y++)
    {
        for(x = 0; x < 64; x++)
        {
            unsigned char coordinates = y * 64 + x;
            if(back[coordinates] ^ front[coordinates])
            {
                if(!changed){
                    changed = 1;
                    if(clear()  < 0){
                        setError("clear: %s", getError());
                        return -1;
                    }

                    if(SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE) < 0){
                        setError("SDL_SetRenderDrawColor: %s", SDL_GetError());
                        return -1;
                    }
                }
                if(back[coordinates]){
                    pixel.x = x * pixel.w;
                    pixel.y = y * pixel.h;
                    if(SDL_RenderFillRect(renderer, &pixel) < 0){
                        setError("SDL_RenderFillRect: %s", SDL_GetError());
                        return -1;
                    }
                }
                front[coordinates] = back[coordinates];
            }
        }
    }

    if(changed) SDL_RenderPresent(renderer);
    return 0;
}

/* renderer is wiped black.
 * Return value is -1 if errors occur, otherwise it is 0. */
int clear(void)
{
    if(SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE) < 0){
        setError("SDL_SetRenderDrawColor: %s", SDL_GetError());
        return -1;
    }
    if(SDL_RenderClear(renderer) < 0){
        setError("SDL_RenderClear: %s", SDL_GetError());
        return -1;
    }
    return 0;
}

/* renderer and window are destroyed. SDL is closed.
 * Errors are ignored. */
void closeVideo(void)
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
