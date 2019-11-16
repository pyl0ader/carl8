#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef struct instructionType {
    char opSymbol[5];
    uint16_t opcode;
    uint8_t parameterSegments[3];
    uint8_t parameters[3];
    uint8_t isBranch;
    uint8_t isJump;
} InstructionType;

typedef struct instruction {
    const InstructionType *type;
    uint16_t nnn;
    uint8_t kk;
    uint8_t x;
    uint8_t y;
    uint8_t n;
} Instruction;

/* static function declaration */
static int loadRom(FILE *rom);

/* variables */
static int programSize;

static uint8_t memory[4096];
static uint8_t v_register[16];
static uint16_t i_register = 0;

static uint16_t pc_register = 0x200;

/* array */
uint8_t screen[64 * 32];

static const InstructionType instructionTypes[] = {
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

/* Starting at index 512, write contents of file rom to memory.
 * TODO: error handling. */
int loadRom(FILE *rom)
{
    programSize = fread(&memory[0x200], sizeof(uint8_t), 0x1000 - 0x200, rom);
    return 0;
}

/* Set memory to all zeros, then load file into memory.
 * Return 0 on success, or return -1 if errors occur. */
int initializeInterpreter(const char* file)
{ 
    FILE* load;

    load = fopen(file, "r");

    memset(memory, 0, 4096);
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

int step(void){}

/* Set instruction to the instruction 
 * decoded from memory at addr.
 * Return 0 on success, or return -1 if errors occur */
int decode(Instruction* instruction, uint16_t addr)
{
    uint16_t word = WORD(addr);
    uint8_t nibble = word >> 12;

    switch(nibble){
        case 0x0:
            switch(word & 0xfff){
                case 0x0e0:
                    instruction->type = instructionTypes;    //00e0 CLS
                    break;
                case 0x0ee:
                    instruction->type = instructionTypes + 1;      //00ee RET
                    break;
                default:
                    instruction->type = instructionTypes + 3;      //0nnn SYS
                    break;
            }
            break;
        case 0x8:
            if((word & 0xf) == 0xe){
                instruction->type = instructionTypes + 18;         //8xyE SHL
                break;
            }
            else if((word & 0xf) < 8){
                instruction->type = instructionTypes + (10 + (word & 0xf)); //8xy(0-7) 
                break;
            }
            logError("", "unknown opcode");
            return -1;
        case 0xe:
            switch(word & 0xff){
                case 0x9e:
                    instruction->type = instructionTypes + 24;     //Ex9E SKP
                    break;
                case 0xa1:
                    instruction->type = instructionTypes + 25;     //ExA1 SKNP
                    break;
                default:
                    logError("", "unknown opcode");
                    return -1;
            }
            break;
        case 0xf:
            switch(word & 0xff){
                case 0x07:
                    instruction->type = instructionTypes + 26;     //Fx07 LD
                    break;
                case 0x0a:
                    instruction->type = instructionTypes + 27;     //Fx0A LD
                    break;
                case 0x15:
                    instruction->type = instructionTypes + 28;     //Fx15 LD
                    break;
                case 0x18:
                    instruction->type = instructionTypes + 29;     //Fx18 LD
                    break;
                case 0x1e:
                    instruction->type = instructionTypes + 30;     //Fx1E ADD
                    break;
                case 0x29:
                    instruction->type = instructionTypes + 31;     //Fx29 LD
                    break;
                case 0x33:
                    instruction->type = instructionTypes + 32;     //Fx33 LD
                    break;
                case 0x55:
                    instruction->type = instructionTypes + 33;     //Fx55 LD
                    break;
                case 0x65:
                    instruction->type = instructionTypes + 34;     //Fx65 LD
                    break;
                default:
                    logError("", "unknown opcode");
                    return -1;
            }
            break;
        default:
            if(nibble > 8)
                instruction->type = instructionTypes + (nibble + 10);
            else
                instruction->type = instructionTypes + (nibble + 2);
            break;
    }

    for(int i=0; i < 3 && instruction->type->parameterSegments[i]; i++){
        switch(instruction->type->parameterSegments[i]){
            case NNN:
                instruction->nnn = word & 0xfff;
                break;
            case KK:
                instruction->kk = word & 0xff;
                break;
            case X:
                instruction->x = (word & 0xf00) >> 8;
                break;
            case Y:
                instruction->y = (word & 0xf0) >> 4;
                break;
            case N:
                instruction->n = word & 0xf;
                break;
        }
    }

    return 0;
}

#define SET_LOGIC(ADDR) (logic[ADDR >> 3] |= 1 << (ADDR & 7) )
#define IS_LOGIC(ADDR) (logic[ADDR >> 3] & (1 << (ADDR & 7) ) )

/* If a program is loaded it is translated to an Assembly Language.
 * The Assembly text is then written to stdout.
 * Returns 0 on success, or returns -1 if errors occur. */
int disassemble()
{ 
    uint8_t logic[4096 / 8];

    int addr = 0x200;
    int branchStack[0x1000 - 0x200];
    int* branchPointer = &branchStack[1];

    Instruction instruction;

    memset(branchStack, 0, sizeof branchStack);
    memset(logic, 0, sizeof logic);

    while(addr < 0x1000 && !IS_LOGIC(addr) && WORD(addr) || ( addr = *(--branchPointer) ))
    {
        SET_LOGIC(addr);
        decode(&instruction, addr);
        if( instruction.type->isBranch ){
            *(branchPointer++) = addr+2;
            addr+=4;
        }
        else if( instruction.type->isJump ){
            addr = instruction.nnn;
        }
        else if(instruction.type->opcode == 0x2000){ // CALL
            *(branchPointer++) = addr+2;
            addr = instruction.nnn;
        }
        else if(instruction.type->opcode == 0x00ee){ // RET
            continue;
        }
        else{
            addr+=2;
        }
    }

    char op[STRING_SIZE] = "";
    char arguments[STRING_SIZE] = "";
    char token[STRING_SIZE] = "";
    uint8_t spriteData[STRING_SIZE];
    uint8_t spriteDataCount = 0;
    addr = 0x200;

    memset(spriteData, 0, STRING_SIZE);
    while(addr < 0x200 + programSize){
        if(IS_LOGIC(addr)){
            decode(&instruction, addr);
            snprintf(op, STRING_SIZE, "0x%x:\t%-5s", addr, instruction.type->opSymbol);

            for(int i=0; i < 3 && instruction.type->parameters[i]; i++){
                switch(instruction.type->parameters[i]){
                    case ADDR:
                        snprintf(token, STRING_SIZE, "0x%03x", instruction.nnn);
                        break;
                    case BYTE:
                        snprintf(token, STRING_SIZE, "%02X", instruction.kk);
                        break;
                    case NIBBLE:
                        snprintf(token, STRING_SIZE, "#%02d", instruction.n);
                        break;
                    case V0:
                        snprintf(token, STRING_SIZE, "V0");
                        break;
                    case VX:
                        snprintf(token, STRING_SIZE, "V%X", instruction.x);
                        break;
                    case VY:
                        snprintf(token, STRING_SIZE, "V%X", instruction.y);
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
                if(instruction.type->parameters[i+1]){     /* Add comma if more argument tokens follow */
                    strncat(token, ", ", STRING_SIZE);
                }
                strncat(arguments, token, STRING_SIZE);
            }
            printf("%s\t%-20s;%X\n", op, arguments, WORD(addr));

            addr += 2;
        }
        else {
            spriteData[spriteDataCount++] = memory[addr];

            addr++;
            if(IS_LOGIC(addr) || addr >= 0x200 + programSize){

                int i = 0;
                while( i < spriteDataCount ){
                    snprintf(op, STRING_SIZE, "0x%x:\t%-5s", (addr - spriteDataCount) + i, "db");

                    for(int j=0; i < spriteDataCount && j < 4; j++, i++){
                        snprintf(token, STRING_SIZE, "#%X", spriteData[i]);

                        if(j+1 < 4 && i+1 < spriteDataCount){      /* Add comma if more byte tokens follow */
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
