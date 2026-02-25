#include <SDL2/SDL.h>

#include "video.h"
#include "logError.h"

static int update(void);

/* variables */
const unsigned short WIDTH = 640;
const unsigned short HEIGHT = 480;

static SDL_Window *window;
static SDL_Texture *interpreterScreen;
static SDL_Renderer *renderer;

/* function definitions */

/* SDL video is initialized. An _SDL_Window_ is created with the title _name_
 * and _window_ is set to it. An _SDL_Renderer_ is created and _renderer_ is set to it.
 * Return value is -1 if errors occur, otherwise it is 0. */
int initializeVideo(const char* name)
{
    SDL_Color white;
    SDL_Color black;

    white.r = 0xff;
    white.g = 0xff;
    white.b = 0xff;
    white.a = 0xff;
   
    black.r = 0x00;
    black.g = 0x00;
    black.b = 0x00;
    black.a = 0xff;

	// SDL Video is initialized
	if(SDL_Init(SDL_INIT_VIDEO) != 0){
		setError("SDL_Init: %s", SDL_GetError());
		return -1;
	}

	// _window_ and _renderer_ are created
	window = SDL_CreateWindow(name, 
							SDL_WINDOWPOS_UNDEFINED, 
							SDL_WINDOWPOS_UNDEFINED, 
							WIDTH, 
							HEIGHT, 
							0);
	if(window == NULL){
		setError("SDL_CreateWindow: %s", SDL_GetError());
		return -1;
	}

	renderer = SDL_CreateRenderer(window, 
								-1, 
								SDL_RENDERER_ACCELERATED);
	if(renderer == NULL){
		setError("SDL_CreateRenderer: %s", SDL_GetError());
		return -1;
	}

	// _interpreterScreen_ is created
	interpreterScreen = SDL_CreateTexture(renderer, 
										SDL_PIXELFORMAT_RGBA8888, 
										SDL_TEXTUREACCESS_TARGET, 
										64,
										32);

	// _renderer_ is cleared
	if(clear()  < 0){
		setError("clear: %s", getError());
		return -1;
	}
   
	if( SDL_SetRenderTarget(renderer, interpreterScreen) < 0){
		setError("SDL_SetRenderTarget: %s", SDL_GetError() );
		return -1;
	}

    // _interpreterScreen_ is cleared
	if(clear()  < 0){
		setError("clear: %s", getError());
		return -1;
	}

	return 0;
}

/* _renderer_ and _window_ are destroyed. SDL is closed.
 * Errors are ignored. */
void closeVideo(void)
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

static uint8_t cleared = 0;

/* For every nonzero element of _back_, using _renderer_, a white _SDL_Point_
 * is rendered at that element's index in a 64 x 32 representation of _back_. 
 * Return value is -1 if errors occur, otherwise it is 0. */
extern int draw(const uint8_t back[64 * 32])
{
	int x, y, i;

	static unsigned char front[64 * 32] = {0};
	unsigned char changed = 0;

	if( SDL_SetRenderTarget(renderer, interpreterScreen) < 0){
		setError("SDL_SetRenderTarget: %s", SDL_GetError() );
		return -1;
	}

	for(y = 0; y < 32; y++)
	{
		for(x = 0; x < 64; x++)
		{
			int coordinates = y * 64 + x;
			if((back[coordinates] ^ front[coordinates]) || (cleared && back[coordinates]) )
			{
				if(!changed)
					changed = 1;

				// prepare to draw
				if( (back[coordinates] && !front[coordinates]) || (cleared && back[coordinates]) ) {
                    // might make "0xff, 0xff, 0xff" a mcaro named "WHITE"
					if(SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE) < 0){
						setError("SDL_SetRenderDrawColor: %s", SDL_GetError());
						return -1;
					}
				}

				// prepare to erase
				else if(!back[coordinates] && front[coordinates]) {
                    // and do the same for black
					if(SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, SDL_ALPHA_OPAQUE) < 0){
						setError("SDL_SetRenderDrawColor: %s", SDL_GetError());
						return -1;
					}
				}

				// draw/erase
				if(SDL_RenderDrawPoint(renderer, x, y) < 0){
					setError("SDL_RenderDrawPoint: %s", SDL_GetError());
					return -1;
				}
				front[coordinates] = back[coordinates];
			}
		}
	}

	if(cleared)
    {
        cleared = 0;
    }
	if(changed) 
    {
        if( update() < 0) {
            setError("update: %s", getError() );
            return -1;
        }
    }

	return 0;
}

/* _renderer_ is wiped black.
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

	cleared = 1;

	return 0;
}

/* then interpreter's screen is drawn,
 * then the toolbar is drawn above it. */
int update(void)
{
	const SDL_Rect screenArea = {0, 0, WIDTH, HEIGHT};

	if( SDL_SetRenderTarget(renderer, NULL) < 0 ) {
        setError("SDL_SetRenderTarget: %s", SDL_GetError() );
        return -1;
    }
	if( SDL_RenderCopy(renderer, interpreterScreen,
				       NULL,
				       &screenArea ) < 0 ) 
    {
        setError("SDL_RenderCopy: %s", SDL_GetError() );
        return -1;
    }

	SDL_RenderPresent(renderer);	
}
