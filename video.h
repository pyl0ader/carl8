#include <stdint.h>

/* initializes video, sets window title to _name_ */
extern int initializeVideo(const char* name);

/* set window size */
extern void video_set_window_size(uint16_t width, uint16_t height);

/* updates the window if _scrn_ has changed sense last call */
extern int draw(const uint8_t back[64 * 32]);

/* wipes the screen black */
extern int clear(void);

/* closes video */
extern void closeVideo(void);
