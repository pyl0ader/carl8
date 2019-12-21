#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "logError.h"
#include "interpreter.h"

#define WORD(ADDR) ( memory[ADDR] << 8 | memory[(ADDR)+1] )
#define STRING_SIZE 80

/* structs */
enum parameterSegments {
    NNN = 1,
    KK,
    X,
    Y,
    N
};

enum parameters {
    ADDR = 1,
    BYTE,
    NIBBLE,
    V0,
    VX,
    VY,
    DT,
    ST,
    I,
    K,
    F,
    B
};

typedef struct instruction {
    char opSymbol[5];
    uint16_t opcode;
    uint8_t parameterSegments[3];
    uint8_t parameters[3];
    uint8_t isBranch;
    uint8_t isJump;
} Instruction;

/* static function declaration */
static int loadRom(FILE *rom);

/* variables */
static int programSize;

static uint8_t memory[4096];
static uint8_t v_register[16];
static uint16_t i_register = 0;

static uint16_t pc_register = 0x200;

static uint16_t nnn;
static uint8_t kk;
static uint8_t x;
static uint8_t y;
static uint8_t n;

/* arrays */
uint8_t screen[64 * 32];

static const Instruction instructionSet[] = {
/*  opsymbol opcode  parameterSegments  parameters        isBranch  isJump */
    {"CLS",  0x00E0, {0},               {0},              0,        0},
    {"RET",  0x00EE, {0},               {0},              0,        0},
    {"SYS",  0x0000, {0},               {ADDR},           0,        0},
    {"JP",   0x1000, {NNN},             {ADDR},           0,        1},
    {"CALL", 0x2000, {NNN},             {ADDR},           0,        0},
    {"SE",   0x3000, {X, KK},           {VX, BYTE},       1,        0},
    {"SNE",  0x4000, {X, KK},           {VX, BYTE},       1,        0},
    {"SE",   0x5000, {X, Y},            {VX, VY},         1,        0},
    {"LD",   0x6000, {X, KK},           {VX, BYTE},       0,        0},
    {"ADD",  0x7000, {X, KK},           {VX, BYTE},       0,        0},
    {"LD",   0x8000, {X, Y},            {VX, VY},         0,        0},
    {"OR",   0x8001, {X, Y},            {VX, VY},         0,        0},
    {"AND",  0x8002, {X, Y},            {VX, VY},         0,        0},
    {"XOR",  0x8003, {X, Y},            {VX, VY},         0,        0},
    {"ADD",  0x8004, {X, Y},            {VX, VY},         0,        0},
    {"SUB",  0x8005, {X, Y},            {VX, VY},         0,        0},
    {"SHR",  0x8006, {X, Y},            {VX},             0,        0},
    {"SUBN", 0x8007, {X, Y},            {VX, VY},         0,        0},
    {"SHL",  0x800E, {X, Y},            {VX},             0,        0},
    {"SNE",  0x9000, {X, Y},            {VX, VY},         1,        0},
    {"LD",   0xA000, {NNN},             {I, ADDR},        0,        0},
    {"JP",   0xB000, {NNN},             {V0, ADDR},       0,        1},
    {"RND",  0xC000, {X, KK},           {VX, BYTE},       0,        0},
    {"DRW",  0xD000, {X, Y, N},         {VX, VY, NIBBLE}, 0,        0},
    {"SKP",  0xE09E, {X},               {VX},             1,        0},
    {"SKNP", 0xE0A1, {X},               {VX},             1,        0},
    {"LD",   0xF007, {X},               {VX, DT},         0,        0},
    {"LD",   0xF00A, {X},               {VX, K},          0,        0},
    {"LD",   0xF015, {X},               {DT, VX},         0,        0},
    {"LD",   0xF018, {X},               {ST, VX},         0,        0},
    {"ADD",  0xF01E, {X},               {I, VX},          0,        0},
    {"LD",   0xF029, {X},               {F, VX},          0,        0},
    {"LD",   0xF033, {X},               {B, VX},          0,        0},
    {"LD",   0xF055, {X},               {I, VX},          0,        0},
    {"LD",   0xF065, {X},               {VX, I},          0,        0}
};

/* function definitions */

/* Starting at index 512, the content of _rom_ is copied to _memory_.
 * Return value is -1 if errors occur, otherwise it is 0.
 */
int loadRom(FILE *rom)
{
    programSize = fread(&memory[0x200], sizeof(uint8_t), 0x1000 - 0x200, rom);
    if( ferror(rom) || !feof(rom) ){
        setError("fread: %s", strerror(errno));
        return -1;
    }
    while(!memory[0x200 + programSize - 1])
        --programSize;
    return 0;
}

/* _memory_ is set to all zeros, then the content of _file_ is copied to _memory_.
 * Return value is -1 if errors occur, otherwise it is 0.
 */
int initializeInterpreter(const char* file)
{ 
    FILE* load;

    load = fopen(file, "r");

    memset(memory, 0, 4096);
    if(load == NULL){
        setError("fopen: %s", strerror(errno));
        return -1;
    }

    if(loadRom(load) < 0){
        setError("loadRom: %s", getError());
        return -1;
    }
    return 0; 
}

int step(void){}

/* The _Instruction_ pointer pointed by _ins_ is set to set to point to the element of _instructionSet_
 * which represents the encoded intruction in _memory_ at index _addr_.
 * Static variables _nnn_, _kk_, _x_, _y_, and _n_ are also set to the values
 * in said encoding (if they exist, otherwise they remain unchanged).
 * Return value is -1 if errors occur, otherwise it is 0.
 */
int decode(const Instruction **ins, uint16_t addr)
{
    uint16_t word = WORD(addr);
    uint8_t nibble = word >> 12;

    switch(nibble){
        case 0x0:
            switch(word & 0xfff){
                case 0x0e0:
                    *ins = instructionSet;    //00e0 CLS
                    break;
                case 0x0ee:
                    *ins = instructionSet + 1;      //00ee RET
                    break;
                default:
                    *ins = instructionSet + 3;      //0nnn SYS
                    break;
            }
            break;
        case 0x8:
            if((word & 0xf) == 0xe){
                *ins = instructionSet + 18;         //8xyE SHL
                break;
            }
            else if((word & 0xf) < 8){
                *ins = instructionSet + (10 + (word & 0xf)); //8xy(0-7) 
                break;
            }
            setError("unknown opcode");
            return -1;
        case 0xe:
            switch(word & 0xff){
                case 0x9e:
                    *ins = instructionSet + 24;     //Ex9E SKP
                    break;
                case 0xa1:
                    *ins = instructionSet + 25;     //ExA1 SKNP
                    break;
                default:
                    setError("unknown opcode");
                    return -1;
            }
            break;
        case 0xf:
            switch(word & 0xff){
                case 0x07:
                    *ins = instructionSet + 26;     //Fx07 LD
                    break;
                case 0x0a:
                    *ins = instructionSet + 27;     //Fx0A LD
                    break;
                case 0x15:
                    *ins = instructionSet + 28;     //Fx15 LD
                    break;
                case 0x18:
                    *ins = instructionSet + 29;     //Fx18 LD
                    break;
                case 0x1e:
                    *ins = instructionSet + 30;     //Fx1E ADD
                    break;
                case 0x29:
                    *ins = instructionSet + 31;     //Fx29 LD
                    break;
                case 0x33:
                    *ins = instructionSet + 32;     //Fx33 LD
                    break;
                case 0x55:
                    *ins = instructionSet + 33;     //Fx55 LD
                    break;
                case 0x65:
                    *ins = instructionSet + 34;     //Fx65 LD
                    break;
                default:
                    setError("unknown opcode");
                    return -1;
            }
            break;
        default:
            if(nibble > 8)
                *ins = instructionSet + (nibble + 10);
            else
                *ins = instructionSet + (nibble + 2);
            break;
    }

    for(int i=0; i < 3 && (*ins)->parameterSegments[i]; i++){
        switch( (*ins)->parameterSegments[i] ){
            case NNN:
                nnn = word & 0xfff;
                break;
            case KK:
                kk = word & 0xff;
                break;
            case X:
                x = (word & 0xf00) >> 8;
                break;
            case Y:
                y = (word & 0xf0) >> 4;
                break;
            case N:
                n = word & 0xf;
                break;
        }
    }

    return 0;
}

#define SET_LOGIC(ADDR) (addrFlags[ADDR >> 2] |= 1 << (ADDR & 3) * 2 + 0 )
#define IS_LOGIC(ADDR)  (addrFlags[ADDR >> 2] & (1 << (ADDR & 3) * 2 + 0 )  )

#define SET_LABEL(ADDR) (addrFlags[ADDR >> 2] |= 1 << (ADDR & 3) * 2 + 1)
#define IS_LABEL(ADDR)  (addrFlags[ADDR >> 2] & (1 << (ADDR & 3) * 2 + 1)  )

/* If a program is loaded it is translated to an Assembly Language.
 * The Assembly text is then written to _stdout_.
 * Return value is -1 if errors occur, otherwise it is 0.
 */
int disassemble()
{ 
    uint8_t addrFlags[4096 / 4];

    int addr = 0x200;
    int branchStack[(4096 - 0x200) / 2];
    int* branchPointer = &branchStack[1];

    const Instruction *instruction;

    memset(branchStack, 0, sizeof branchStack);
    memset(addrFlags, 0, sizeof addrFlags);

    /* The program's control flow is traced to discern program instructions
     * from sprite data.
     */
    while(addr < 0x1000 && !IS_LOGIC(addr) || ( addr = *(--branchPointer) ))
    {
        SET_LOGIC(addr);
        decode(&instruction, addr);
        if( instruction->isBranch ){
            *(branchPointer++) = addr+2;
            addr+=4;
        }
        else if( instruction->isJump ){
            addr = nnn;
            SET_LABEL(addr);
        }
        else if(instruction->opcode == 0x2000){ // CALL
            *(branchPointer++) = addr+2;
            addr = nnn;
            SET_LABEL(addr);
        }
        else if(instruction->opcode == 0x00ee){ // RET
            continue;
        }
        else{
            addr+=2;
        }
    }

    addr = 0x200;

    char token[STRING_SIZE] = "";

    char op[STRING_SIZE] = "";
    char arguments[STRING_SIZE] = "";

    uint8_t spriteData[0xe00];
    unsigned int spriteDataCount = 0;

    /* Encoded instructions are translated to symbolic instructions,
     * and sprite data to data directives. Then they are printed to 
     * _stdout_.
     */
    memset(spriteData, 0, STRING_SIZE);

    while(addr < 0x200 + programSize){
        if(IS_LOGIC(addr)){
            decode(&instruction, addr);
            strcpy(op, instruction->opSymbol);

            for(int i=0; i < 3 && instruction->parameters[i]; i++){
                switch(instruction->parameters[i]){
                    case ADDR:
                        snprintf(token, STRING_SIZE, "#%03X", nnn);
                        break;
                    case BYTE:
                        snprintf(token, STRING_SIZE, "#%02X", kk);
                        break;
                    case NIBBLE:
                        snprintf(token, STRING_SIZE, "#%X", n);
                        break;
                    case V0:
                        snprintf(token, STRING_SIZE, "V0");
                        break;
                    case VX:
                        snprintf(token, STRING_SIZE, "V%X", x);
                        break;
                    case VY:
                        snprintf(token, STRING_SIZE, "V%X", y);
                        break;
                    case DT:
                        snprintf(token, STRING_SIZE, "DT");
                        break;
                    case ST:
                        snprintf(token, STRING_SIZE, "ST");
                        break;
                    case I:
                        snprintf(token, STRING_SIZE, "I");
                        break;
                    case K:
                        snprintf(token, STRING_SIZE, "K");
                        break;
                    case F:
                        snprintf(token, STRING_SIZE, "F");
                        break;
                    case B:
                        snprintf(token, STRING_SIZE, "B");
                        break;
                }
                if(instruction->parameters[i+1]){ //Add comma if more argument tokens follow
                    strncat(token, ", ", STRING_SIZE);
                }
                strncat(arguments, token, STRING_SIZE);
            }
            if(IS_LABEL(addr)) printf("0x%03X:\t%s\t%-20s;%04X\n", addr, op, arguments, WORD(addr));
            else  printf("\t%s\t%-20s;%04X\n", op, arguments, WORD(addr));

            addr += 2;
        }
        else {

            /* When a segment of sprite data is encountered, it is collected in 
             * _spriteData_.
             */
            spriteData[spriteDataCount++] = memory[addr];

            ++addr;
            if(IS_LOGIC(addr) || addr >= 0x200 + programSize){

                /* When the end of that segment arrives it is printed, and
                 * _spriteData_ is set to be overwritten by the next segment.
                 */
                int i = 0;
                while( i < spriteDataCount ){
                    snprintf(op, STRING_SIZE, "0x%03X:\t%-5s", (addr - spriteDataCount) + i, "db");

                    for(int j=0; i < spriteDataCount && j < 4; j++, i++){
                        snprintf(token, STRING_SIZE, "#%02X", spriteData[i]);

                        if(j+1 < 4 && i+1 < spriteDataCount){ //Add comma if more byte tokens follow
                            strncat(token, ", ", STRING_SIZE);
                        }
                        strncat(arguments, token, STRING_SIZE);
                    }
                    printf("%s\t%-10s\n", op, arguments);
                    arguments[0] = '\0';
                }
                spriteDataCount = 0;

            }
        }
        arguments[0] = '\0';

    }

    return 0;
} 
