#include <stdio.h>

#include "logError.h"
#include "interpreter.h"

/* opcodes enum */

/* static function declaration */
static int loadRom(FILE *rom);

/* variables */
static uint8_t memory[4096] = {};
static uint8_t v[16] = {};
static uint16_t i;

static uint16_t pc = 0x200;
static uint16_t instruction;


/* array initialization */
uint8_t screen[64 * 32] = {};

/* function definitions */
int initializeInterpreter(const char* file)
{ 
    FILE* load;

    load = fopen(file, "r");

    if(load == NULL){
        logError("fopen","could not open file");
        return -1;
    }

    if(loadRom(load) < 0){
        logError("loadRom", getError());
        return -1;
    }
    return 0; 
}

int
interpreterProcess(void){}

int
getInstruction(uint16_t addr)
{
    return 0;
}

#include <string.h>
#define STRING_SIZE 80

#define SET_LOGIC(ADDR) /* set logic */
#define IS_LOGIC(ADDR) /* is logic */
int
disassemble()
{ 
    /*
    int addr = 0x200;
    int branchStack[0x1000 - 0x200];
    int* branchPointer = &branchStack[1];

    memset(branchStack, 0, sizeof branchStack);
    //step one: differentiate logic from data
    while(addr < 0x1000 && !IS_LOGIC(addr) || addr = *(--branchPointer))
    {
        SET_LOGIC(addr);
        if( is branch ){
            *branchPointer++ = addr+2;
            addr+=4;
        }
        else if( is jump ){
            addr = addr of jump;
        }
        else{
            addr+=2
        }
    }
    //step two: step through logic printing instructions
    */
    return 0;
} 

int
loadRom(FILE *rom)
{
    if(fread(&memory[0x200], sizeof(uint8_t), 0x1000 - 0x200, rom) % 2){
        logError("","Invalid Opcode");
        return -1;
    }
    return 0;
}
