#include <stdint.h>
 
/* The audio back-end is initialized.
 * Return value is -1 if errors occur otherwise it's zero
 */
extern int initAudio();

/* Start a continuous beep.
 * Return value is -1 if errors occur otherwise it's zero
 */
extern int beepStart(void);

/* If started, end the continuous beep.
 * Return value is -1 if errors occur otherwise it's zero
 */
extern int beepStop(void);

/* The audio back-end is shutdown.
 * Return value is -1 if errors occur otherwise it's zero
 */
extern int closeAudio(void);
