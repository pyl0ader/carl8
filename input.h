#include <stdint.h>

void initializeInput(void);
void inputProcess();

extern struct Action {
    uint8_t quit;
    uint16_t interpreterInput;
} action;
