#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "logError.h"
#include "interpreter.h"
#include "input.h"
#include "video.h"

#include "interpreter_codec.h"

#define WORD(ADDR) ( interp_memory[ADDR] << 8 | interp_memory[(ADDR)+1] )
#define V_REGISTER_LEN 16
#define SCREEN_LEN 64 * 32
#define STRING_SIZE 80

enum instructionDataFlag {
    INSTRUCTIONDATA_ADDR    = 1,
    INSTRUCTIONDATA_BYTE    = 2,
    INSTRUCTIONDATA_VX  = 4,
    INSTRUCTIONDATA_VY  = 8,
    INSTRUCTIONDATA_NIBBLE  = 16,
};

/* variables */
static uint8_t interp_memory[INTERP_MEMORY_LEN];
static int programSize;

static uint8_t v_register[V_REGISTER_LEN];
static uint16_t i_register = 0;
static uint16_t pc_register = INTERP_PROGRAM_START;

static uint8_t interp_screen[SCREEN_LEN];

static const uint16_t opcodes[INTERP_INSTRUCTIONSET_LEN] = {
    0x00E0,
    0x00EE,
    0x0000,
    0x1000,
    0x2000,
    0x3000,
    0x4000,
    0x5000,
    0x6000,
    0x7000,
    0x8000,
    0x8001,
    0x8002,
    0x8003,
    0x8004,
    0x8005,
    0x8006,
    0x8007,
    0x800E,
    0x9000,
    0xA000,
    0xB000,
    0xC000,
    0xD000,
    0xE09E,
    0xE0A1,
    0xF007,
    0xF00A,
    0xF015,
    0xF018,
    0xF01E,
    0xF029,
    0xF033,
    0xF055,
    0xF065
};

static const uint8_t instructionDataMask[INTERP_INSTRUCTIONSET_LEN] = {
    0,
    0,
    INSTRUCTIONDATA_ADDR,
    INSTRUCTIONDATA_ADDR,
    INSTRUCTIONDATA_ADDR,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_BYTE,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_BYTE,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_VY,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_BYTE,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_BYTE,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_VY,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_VY,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_VY,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_VY,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_VY,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_VY,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_VY,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_VY,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_VY,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_VY,
    INSTRUCTIONDATA_ADDR,
    INSTRUCTIONDATA_ADDR,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_BYTE,
    INSTRUCTIONDATA_VX | INSTRUCTIONDATA_VY | INSTRUCTIONDATA_NIBBLE,
    INSTRUCTIONDATA_VX,
    INSTRUCTIONDATA_VX,
    INSTRUCTIONDATA_VX,
    INSTRUCTIONDATA_VX,
    INSTRUCTIONDATA_VX,
    INSTRUCTIONDATA_VX,
    INSTRUCTIONDATA_VX,
    INSTRUCTIONDATA_VX,
    INSTRUCTIONDATA_VX,
    INSTRUCTIONDATA_VX,
    INSTRUCTIONDATA_VX
};

/* function definitions */

/* Starting at index 512, the content of _rom_ is copied to _memory_.
 * Return value is -1 if errors occur, otherwise it is 0.
 */
int interp_loadRom(const char* rom)
{
    FILE* load;

    load = fopen(rom, "r");

    if(load == NULL){
        setError("fopen: %s", strerror(errno));
        return -1;
    }

    programSize = fread(interp_memory + INTERP_PROGRAM_START, 
            sizeof(uint8_t), 
            INTERP_MEMORY_LEN - INTERP_PROGRAM_START, 
            load);

    if( ferror(load) || !feof(load) ){
        setError("fread: %s", strerror(errno));
        return -1;
    }

    while(!interp_memory[INTERP_PROGRAM_START + programSize - 1]){
        if(!--programSize){
            setError("rom file is blank; not a valid program");
            return -1;
        }
    }
    return 0;
}

/* _interp_memory_ is set to all zeros.
 * No error handling at the moment, return value is 0.
 */
int interp_initialize(void){
    memset(interp_memory, 0, INTERP_MEMORY_LEN);
    return 0; 
}

int interp_step(void){

    for(int i = 0; i < 16; i++){
        interp_screen[i] = (action.interpreterInput >> i) & 1;
    }

    draw(interp_screen);
}

/* The encoded intruction in _interp_memory_ at index _addr_ is stored in
 * the _instruction_ pointed by _ins_.
 * Return value is -1 if errors occur, otherwise it is 0.
 */
int interp_decode(uint16_t addr, interp_instruction *ins)
{
    uint16_t word = WORD(addr);
    uint8_t topNibble = word >> 12;

    enum interp_instructionId id = INTERP_INSTRUCTION_UNKNOWN;
   
    switch(topNibble){
        case 0x0:
            switch(word & 0xfff){
                case 0x0e0:
                    id = INTERP_CLS;
                    break;
                case 0x0ee:
                    id = INTERP_RET;
                    break;
                default:
                    id = INTERP_SYS_ADDR;
                    break;
            }
            break;
        case 0x8:
            if((word & 0xf) == 0xe){
                id = INTERP_SHL_VX_VY;
                break;
            }
            else if((word & 0xf) < 8){
                id = (INTERP_LD_VX_VY + (word & 0xf)); //INTERP_LD_VX_VY - INTERP_SUBN_VX_VY
                break;
            }
        case 0xe:
            switch(word & 0xff){
                case 0x9e:
                    id = INTERP_SKP_VX;
                    break;
                case 0xa1:
                    id = INTERP_SKNP_VX;
                    break;
            }
            break;
        case 0xf:
            switch(word & 0xff){
                case 0x07:
                    id = INTERP_LD_VX_DT;
                    break;
                case 0x0a:
                    id = INTERP_LD_VX_K;
                    break;
                case 0x15:
                    id = INTERP_LD_DT_VX;
                    break;
                case 0x18:
                    id = INTERP_LD_ST_VX;
                    break;
                case 0x1e:
                    id = INTERP_ADD_I_VX;
                    break;
                case 0x29:
                    id = INTERP_LD_F_VX;
                    break;
                case 0x33:
                    id = INTERP_LD_B_VX;
                    break;
                case 0x55:
                    id = INTERP_LD_MEMINDEX_VX;
                    break;
                case 0x65:
                    id = INTERP_LD_VX_MEMINDEX;
                    break;
            }
            break;
        case 0x5:
            if( (word & 0xf) == 0)
                id = INTERP_SE_VX_VY;
            break;
        case 0x9:
            if( (word & 0xf) == 0)
                id = INTERP_SNE_VX_VY;
            break;
        default:
            if(topNibble > 9)
                id = topNibble + INTERP_LD_VX_VY;
            else
                id = topNibble + INTERP_SYS_ADDR;
            break;
    }

    if(id == INTERP_INSTRUCTION_UNKNOWN){
        setError("unknown opcode at address 0x%02X", addr);
        return -1;
    }

    ins->id = id;

    if(instructionDataMask[id] & INSTRUCTIONDATA_ADDR)
        ins->nnn = WORD(addr) & 0xfff;
    if(instructionDataMask[id] & INSTRUCTIONDATA_BYTE)
        ins->kk = WORD(addr) & 0xff;
    if(instructionDataMask[id] & INSTRUCTIONDATA_VX)
        ins->x = (WORD(addr) & 0xf00) >> 8;
    if(instructionDataMask[id] & INSTRUCTIONDATA_VY)
        ins->y = (WORD(addr) & 0xf0) >> 4;
    if(instructionDataMask[id] & INSTRUCTIONDATA_NIBBLE)
        ins->n = WORD(addr) & 0xf;

    return 0;
}

int interp_encode(uint16_t addr, interp_instruction ins){
    uint16_t word = 0;

    if(ins.id == INTERP_INSTRUCTION_UNKNOWN) {
        setError("instruction unknown, cannot encode");
        return -1;
    }

    word |= opcodes[ins.id];

    if(instructionDataMask[ins.id] & INSTRUCTIONDATA_ADDR)
        word |= ins.nnn;
    if(instructionDataMask[ins.id] & INSTRUCTIONDATA_BYTE)
        word |= ins.kk;
    if(instructionDataMask[ins.id] & INSTRUCTIONDATA_VX)
        word |= ins.x << 8;
    if(instructionDataMask[ins.id] & INSTRUCTIONDATA_VY)
        word |= ins.y << 4;
    if(instructionDataMask[ins.id] & INSTRUCTIONDATA_NIBBLE)
        word |= ins.n;

    interp_memory[addr]   = word >> 8;
    interp_memory[addr+1] = word & 0xff;

    if( (addr - INTERP_PROGRAM_START) + 1 > programSize )
        programSize = (addr - INTERP_PROGRAM_START) + 1;

    return 0;
}

int interp_writeMemory(uint16_t addr, uint8_t byte){

    if(addr >= INTERP_MEMORY_LEN || addr < 0){
        setError("invalid address 0x%02X", addr);
        return -1;
    }
    else if(addr < INTERP_PROGRAM_START){
        setError("prohibited address 0x%02X", addr);
        return -1;
    }
    else 
        interp_memory[addr] = byte;

    if( (addr - INTERP_PROGRAM_START) + 1 > programSize )
        programSize = (addr - INTERP_PROGRAM_START) + 1;

    return 0;
}

int interp_readMemory(uint16_t addr){
    return interp_memory[addr];
}

int interp_getProgramSize(void){
    return programSize;
}
