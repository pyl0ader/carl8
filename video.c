#include <SDL2/SDL.h>

#include "video.h"
#include "error.h"

const unsigned short WIDTH = 640;
const unsigned short HEIGHT = 480;

static SDL_Window *win;
static SDL_Renderer *ren;

int initializeVideo(const char* name)
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        logError("SDL_Init", SDL_GetError());
        return -1;
    }
    win = SDL_CreateWindow(name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, 0);
    if(win == NULL){
        logError("SDL_CreateWindow", SDL_GetError());
        return -1;
    }
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(ren == NULL){
        logError("SDL_CreateRenderer", SDL_GetError());
        return -1;
    }

    if(clear()  < 0){
        logError("clear", getError());
        return -1;
    }

    return 0;
}

int videoProcess(unsigned char screen[64 * 32])
{
    int x, y, i;
    SDL_Rect pixel;

    pixel.w = WIDTH / 64;
    pixel.h = HEIGHT / 32;

    if(clear()  < 0){
        logError("clear", getError());
        return -1;
    }

    if(SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE) < 0){
        logError("SDL_SetRenderDrawColor", SDL_GetError());
        return -1;
    }
    for(y = 0; y < 32; y++){
        for(x = 0; x < 64; x++){
            if(screen[y * 64 + x]){
                pixel.x = x * pixel.w;
                pixel.y = y * pixel.h;
                if(SDL_RenderFillRect(ren, &pixel) < 0){
                    logError("SDL_RenderFillRect", SDL_GetError());
                    return -1;
                }
            }
        }
    }
    SDL_RenderPresent(ren);
    return 0;
}

int clear(void)
{
    if(SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE) < 0){
        logError("SDL_SetRenderDrawColor", SDL_GetError());
        return -1;
    }
    if(SDL_RenderClear(ren) < 0){
        logError("SDL_RenderClear", SDL_GetError());
        return -1;
    }
    return 0;
}

void closeVideo(void)
{
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}
