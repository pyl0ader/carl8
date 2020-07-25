#include <stdint.h>

/* initializes video, sets window title to _name_ */
extern int initializeVideo(const char* name);

/* updates the window if _scrn_ has changed sense last call */
extern int draw(const uint8_t back[64 * 32]);

/* wipes the screen black */
extern int clear(void);

/* closes video */
extern void closeVideo(void);

extern const unsigned short int WIDTH;
extern const unsigned short int HEIGHT;
