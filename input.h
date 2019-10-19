#include <stdint.h>

int initializeInput(void);
int inputProcess();

extern struct Action {
    uint8_t quit;
    uint16_t interpreterInput;
} action;
