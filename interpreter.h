#include <stdint.h>

/* function declarations */
int initializeInterpreter(const char* file);
int interpreterProcess(void);
int disassemble();

/* array declaration */
extern uint8_t screen[64 * 32];
