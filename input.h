#include <stdint.h>

extern void initializeInput(void);
extern void inputProcess();

extern struct Action {
    uint8_t quit;
    uint16_t interpreterInput;
} action;
