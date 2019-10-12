#include <stdio.h>

#include "logError.h"
#include "interpreter.h"

static int loadRom(FILE *rom);

static uint8_t memory[4096] = {};
static uint8_t v[16] = {};
static uint16_t i;

static uint16_t pc = 0x200;
static uint16_t instruction;

uint8_t screen[64 * 32] = {};

enum {
    NNN,
    X,
    Y,
    KK,
    N
};


int initializeInterpreter(const char* file)
{ 
    /*
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
    */
    return 0; 
}

int interpreterProcess(void){}

int getInstruction(uint16_t addr)
{
    /*
    const struct {
       uint16_t mask;
       uint16_t code;
       uint8_t parameterCount;
       uint8_t parameter[3];
       char name[4];
    } optab[35] = {
        {0xF000, 0x0000, 0, {}, "SYS" }, 
        {0xFFFF, 0x00E0, 0, {}, "CLS" },
        {0xFFFF, 0x00EE, 0, {}, "RET" },
        {0xF000, 0x1000, 1, {NNN}, "JP" },
        {0xF000, 0x2000, 1, {NNN}, "CALL" },
        {0xF000, 0x3000, 2, {X, KK}, "SE" },
        {0xF000, 0x4000, 2, {X, KK}, "SNE" },
        {0xF00F, 0x5000, 2, {X, Y}, "SE" },
        {0xF000, 0x6000, 2, {X, KK}, "LD" },
        {0xF000, 0x7000, 2, {X, KK}, "ADD" },
        {0xF00F, 0x8000, 2, {X, Y}, "LD" },
        {0xF00F, 0x8001, 2, {X, Y}, "OR" },
        {0xF00F, 0x8002, 2, {X, Y}, "AND" },
        {0xF00F, 0x8003, 2, {X, Y}, "XOR" },
        {0xF00F, 0x8004, 2, {X, Y}, "ADD" },
        {0xF00F, 0x8005, 2, {X, Y}, "SUB" },
        {0xF00F, 0x8006, 2, {X, Y}, "SHR" },
        {0xF00F, 0x8007, 2, {X, Y}, "SUBN" },
        {0xF00F, 0x800E, 2, {X, Y}, "SHL" },
        {0xF00F, 0x9000, 2, {X, Y}, "SNE" },
        {0xF000, 0xA000, 1, {NNN}, "LD" },
        {0xF000, 0xB000, 1, {NNN}, "JP" },
        {0xF000, 0xC000, 2, {X, KK}, "RND" },
        {0xF000, 0xD000, 3, {X, Y, N}, "DRW" },
        {0xF0FF, 0xE09E, 1, {X}, "SKP" },
        {0xF0FF, 0xE0A1, 1, {X}, "SKNP" },
        {0xF0FF, 0xF007, 1, {X}, "LD" },
        {0xF0FF, 0xF00a, 1, {X}, "LD" },
        {0xF0FF, 0xF015, 1, {X}, "LD" },
        {0xF0FF, 0xF018, 1, {X}, "LD" },
        {0xF0FF, 0xF01E, 1, {X}, "ADD" },
        {0xF0FF, 0xF029, 1, {X}, "LD" },
        {0xF0FF, 0xf033, 1, {X}, "LD" },
        {0xF0FF, 0xf055, 1, {X}, "LD" },
        {0xF0FF, 0xf065, 1, {X}, "LD" } 
    };

    uint16_t i = ((uint16_t)memory[addr] << 8) | memory[addr+1];
    for(int j = 0; j < 35 && !(i & optab[j].mask == optab[j].code); j++);
    */
    return 0;
}

#include <string.h>
#define STRING_SIZE 80

int disassemble()
{ 
    //step one, differentiate logic from data
    //step two, step through logic printing instructions
    return 0;
} 

int loadRom(FILE *rom)
{
    /*
    if(fread(&memory[0x200], sizeof(unsigned char), 0x1000 - 0x200, rom) % 2){
        logError("","Invalid Opcode");
        return -1;
    }
    return 0;
    */
}
