#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include "logError.h"
#include "interpreter.h"
#include "input.h"
#include "video.h"

#include <unistd.h>

#define FETCH(ADDR) ( interp_memory[ADDR] << 8 | interp_memory[(ADDR)+1] )
#define CLOCKRATE ( (float)1 / 60 )
#define V_REGISTER_LEN 16
#define SCREEN_LEN 64 * 32
#define STRING_SIZE 80
#define SECOND 1000000000

enum instructionDataFlag {
	INSTRUCTIONDATA_ADDR	= 1,
	INSTRUCTIONDATA_BYTE	= 2,
	INSTRUCTIONDATA_VX  = 4,
	INSTRUCTIONDATA_VY  = 8,
	INSTRUCTIONDATA_NIBBLE  = 16,
};

/* variables */
static uint8_t interp_memory[INTERP_MEMORY_LEN];
static int programSize;

static uint16_t stack[(INTERP_MEMORY_LEN - INTERP_PROGRAM_START) / 4]; 
static uint16_t *stackPointer = stack - 1;

static uint8_t v_register[V_REGISTER_LEN];
static uint16_t i_register = 0;
static uint16_t pc_register = INTERP_PROGRAM_START;
uint8_t dt_register = 0;
uint8_t st_register = 0;

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
		if(--programSize <= 0){
			setError("rom file is blank; not a valid program");
			return -1;
		}
	}
	return 0;
}

/* _interp_memory_ is set to all zeros. Font sprites are loaded.
 * No error handling at the moment, return value is 0.
 */
int interp_initialize(void){
	memset(interp_memory, 0, INTERP_MEMORY_LEN);
	memset(interp_screen, 0, SCREEN_LEN);
	memset(v_register, 0, V_REGISTER_LEN);

	/* "0" Hex */
	interp_memory[0] = 0xF0;
	interp_memory[1] = 0x90;
	interp_memory[2] = 0x90;
	interp_memory[3] = 0x90;
	interp_memory[4] = 0xF0;

	/* "1" Hex */
	interp_memory[5] = 0x20;
	interp_memory[6] = 0x60;
	interp_memory[7] = 0x20;
	interp_memory[8] = 0x20;
	interp_memory[9] = 0x70;

	/* "2" Hex */
	interp_memory[10] = 0xF0;
	interp_memory[11] = 0x10;
	interp_memory[12] = 0xF0;
	interp_memory[13] = 0x80;
	interp_memory[14] = 0xF0;

	/* "3" Hex */
	interp_memory[15] = 0xF0;
	interp_memory[16] = 0x10;
	interp_memory[17] = 0xF0;
	interp_memory[18] = 0x10;
	interp_memory[19] = 0xF0;

	/* "4" Hex */
	interp_memory[20] = 0x90;
	interp_memory[21] = 0x90;
	interp_memory[22] = 0xF0;
	interp_memory[23] = 0x10;
	interp_memory[24] = 0x10;

	/* "5" Hex */
	interp_memory[25] = 0xF0;
	interp_memory[26] = 0x80;
	interp_memory[27] = 0xF0;
	interp_memory[28] = 0x10;
	interp_memory[29] = 0xF0;

	/* "6" Hex */
	interp_memory[30] = 0xF0;
	interp_memory[31] = 0x80;
	interp_memory[32] = 0xF0;
	interp_memory[33] = 0x90;
	interp_memory[34] = 0xF0;

	/* "7" Hex */
	interp_memory[35] = 0xF0;
	interp_memory[36] = 0x10;
	interp_memory[37] = 0x20;
	interp_memory[38] = 0x40;
	interp_memory[39] = 0x40;

	/* "8" Hex */
	interp_memory[40] = 0xF0;
	interp_memory[41] = 0x90;
	interp_memory[42] = 0xF0;
	interp_memory[43] = 0x90;
	interp_memory[44] = 0xF0;

	/* "9" Hex */
	interp_memory[45] = 0xF0;
	interp_memory[46] = 0x90;
	interp_memory[47] = 0xF0;
	interp_memory[48] = 0x10;
	interp_memory[49] = 0xF0;

	/* "A" Hex */
	interp_memory[50] = 0xF0;
	interp_memory[51] = 0x90;
	interp_memory[52] = 0xF0;
	interp_memory[53] = 0x90;
	interp_memory[54] = 0x90;

	/* "B" Hex */
	interp_memory[55] = 0xE0;
	interp_memory[56] = 0x90;
	interp_memory[57] = 0xE0;
	interp_memory[58] = 0x90;
	interp_memory[59] = 0xE0;

	/* "C" Hex */
	interp_memory[60] = 0xF0;
	interp_memory[61] = 0x80;
	interp_memory[62] = 0x80;
	interp_memory[63] = 0x80;
	interp_memory[64] = 0xF0;

	/* "D" Hex */
	interp_memory[65] = 0xE0;
	interp_memory[66] = 0x90;
	interp_memory[67] = 0x90;
	interp_memory[68] = 0x90;
	interp_memory[69] = 0xE0;

	/* "E" Hex */
	interp_memory[70] = 0xF0;
	interp_memory[71] = 0x80;
	interp_memory[72] = 0xF0;
	interp_memory[73] = 0x80;
	interp_memory[74] = 0xF0;

	/* "F" Hex */
	interp_memory[75] = 0xF0;
	interp_memory[76] = 0x80;
	interp_memory[77] = 0xF0;
	interp_memory[78] = 0x80;
	interp_memory[79] = 0x80;

	return 0; 
}

int interp_step(long delta)
{

	static long elapsed = 0;

	elapsed += delta;

	if(elapsed >= SECOND * CLOCKRATE){

		int decrement = elapsed / (SECOND * CLOCKRATE);

		elapsed = 0;

		if(dt_register)
			dt_register -= dt_register - decrement < 0 ? dt_register : decrement;
		if(st_register)
			st_register -= st_register - decrement < 0 ? st_register : decrement;
	}

	interp_instruction instruction;

	interp_decode(pc_register, &instruction);

	switch(instruction.id){
		case INTERP_CLS:
			memset(interp_screen, 0, SCREEN_LEN);
			clear();
			pc_register += 2;
		break;
		case INTERP_RET:
			pc_register = *--stackPointer;
		break;
		case INTERP_SYS_ADDR:
		
		break;
		case INTERP_JP_ADDR:
			pc_register = instruction.nnn;
		break;
		case INTERP_CALL_ADDR:
			*stackPointer++ = pc_register + 2;
			pc_register = instruction.nnn;
		break;
		case INTERP_SE_VX_BYTE:
			if(v_register[instruction.x] == instruction.kk)
				pc_register += 4;
			else
				pc_register += 2;
		break;
		case INTERP_SNE_VX_BYTE:
			if(v_register[instruction.x] != instruction.kk)
				pc_register += 4;
			else
				pc_register += 2;
		break;
		case INTERP_SE_VX_VY:
			if(v_register[instruction.x] == v_register[instruction.y])
				pc_register += 4;
			else
				pc_register += 2;
		break;
		case INTERP_LD_VX_BYTE:
			v_register[instruction.x] = instruction.kk;
			pc_register += 2;
		break;
		case INTERP_ADD_VX_BYTE:
			v_register[instruction.x] += instruction.kk;
			pc_register += 2;
		break;
		case INTERP_LD_VX_VY:
			v_register[instruction.x] = v_register[instruction.y];
			pc_register += 2;
		break;
		case INTERP_OR_VX_VY:
			v_register[instruction.x] |= v_register[instruction.y];
			pc_register += 2;
		break;
		case INTERP_AND_VX_VY:
			v_register[instruction.x] &= v_register[instruction.y];
			pc_register += 2;
		break;
		case INTERP_XOR_VX_VY:
			v_register[instruction.x] ^= v_register[instruction.y];
			pc_register += 2;
		break;
		case INTERP_ADD_VX_VY:
			v_register[instruction.x] += v_register[instruction.y];
			pc_register += 2;
		break;
		case INTERP_SUB_VX_VY:
			v_register[instruction.x] -= v_register[instruction.y];
			v_register[0xf] = v_register[instruction.x] > v_register[instruction.y];
			pc_register += 2;
		break;
		case INTERP_SHR_VX_VY:
			v_register[instruction.x] >>= 1;
			v_register[0xf] = v_register[instruction.x] & 1;
			pc_register += 2;
		break;
		case INTERP_SUBN_VX_VY:
			v_register[instruction.x] >>= 1;
			v_register[0xf] = v_register[instruction.x] < v_register[instruction.y];
			pc_register += 2;
		break;
		case INTERP_SHL_VX_VY:
			v_register[instruction.x] <<= 1;
			v_register[0xf] = v_register[instruction.x] & (1 << 7);
			pc_register += 2;
		break;
		case INTERP_SNE_VX_VY:
			if(v_register[instruction.x] != v_register[instruction.y])
				pc_register += 4;
			else
				pc_register += 2;
		break;
		case INTERP_LD_I_ADDR:
			i_register = instruction.nnn;
			pc_register += 2;
		break;
		case INTERP_JP_V0_ADDR:
			pc_register += v_register[0] + instruction.nnn;
		break;
		case INTERP_RND_VX_BYTE:

		break;
		case INTERP_DRW_VX_VY_NIBBLE:
			v_register[0xf] = 0;

			for(int x = v_register[instruction.x]; x < v_register[instruction.x] + 7; x++){
				for(int y = v_register[instruction.y]; y < v_register[instruction.y] + instruction.n; y++){
					int screenIndex = 64 * (y % 32) + (x % 64);
					int spriteBit = interp_memory[i_register + (y - v_register[instruction.y])] & (1 << (7 - (x - v_register[instruction.x]) ) );

					if(!v_register[0xf])
						v_register[0xf] = interp_screen[screenIndex] && spriteBit;
						
					interp_screen[screenIndex] = (interp_screen[screenIndex] > 0) ^ (spriteBit > 0);
				}
			}

			draw(interp_screen);
			pc_register += 2;
		break;
		case INTERP_SKP_VX:

		break;
		case INTERP_SKNP_VX:

		break;
		case INTERP_LD_VX_DT:
			v_register[instruction.x] = dt_register;
			pc_register += 2;
		break;
		case INTERP_LD_VX_K:
			
		break;
		case INTERP_LD_DT_VX:
			dt_register = v_register[instruction.x];
			pc_register += 2;
		break;
		case INTERP_LD_ST_VX:

		break;
		case INTERP_ADD_I_VX:
			i_register += v_register[instruction.x];
			pc_register += 2;
		break;
		case INTERP_LD_F_VX:
			i_register = 5 * (v_register[instruction.x] & 0xf);
			pc_register += 2;
		break;
		case INTERP_LD_B_VX:

		break;
		case INTERP_LD_MEMINDEX_VX:

		break;
		case INTERP_LD_VX_MEMINDEX:

		break;
		case INTERP_INSTRUCTION_UNKNOWN:

		break;
	}
}

/* The encoded intruction in _interp_memory_ at index _addr_ is stored in
 * the _instruction_ pointed by _ins_.
 * Return value is -1 if errors occur, otherwise it is 0.
 */
int interp_decode(uint16_t addr, interp_instruction *ins)
{
	uint16_t word = FETCH(addr);
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
		ins->nnn = word & 0xfff;
	if(instructionDataMask[id] & INSTRUCTIONDATA_BYTE)
		ins->kk = word & 0xff;
	if(instructionDataMask[id] & INSTRUCTIONDATA_VX)
		ins->x = (word & 0xf00) >> 8;
	if(instructionDataMask[id] & INSTRUCTIONDATA_VY)
		ins->y = (word & 0xf0) >> 4;
	if(instructionDataMask[id] & INSTRUCTIONDATA_NIBBLE)
		ins->n = word & 0xf;

	return 0;
}

int interp_encode(uint16_t addr, interp_instruction ins)
{
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
