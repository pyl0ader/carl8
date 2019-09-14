#include <stdint.h>

int initializeInterpreter(const char* file);
int interpreterProcess(void);
int disassemble();

extern uint8_t screen[64 * 32];
