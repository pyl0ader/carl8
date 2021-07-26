#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "util.h"
#include "logError.h"
#include "interpreter.h"

#define STRING_SIZE 80
#define TOKEN_SIZE 6
#define BRANCHES_MAX ( (INTERP_MEMORY_LEN - INTERP_PROGRAM_START) / 2 )
#define ADDR_FLAGS_LEN (INTERP_MEMORY_LEN / 4)

#define WORD(ADDR) ( interp_readMemory(ADDR) << 8 | interp_readMemory((ADDR) + 1) )

#define SET_LOGIC(ADDR) (addrFlags[(ADDR) >> 2] |= 1 << ((ADDR) & 3) * 2 + 0 )
#define IS_LOGIC(ADDR)  (addrFlags[(ADDR) >> 2] & (1 << ((ADDR) & 3) * 2 + 0 )  )

#define SET_LABEL(ADDR) (addrFlags[(ADDR) >> 2] |= 1 << ((ADDR) & 3) * 2 + 1)
#define IS_LABEL(ADDR)  (addrFlags[(ADDR) >> 2] & (1 << ((ADDR) & 3) * 2 + 1)  )

/* The operand set. */
enum operand {
	OPERAND_NONE = -6,
	OPERAND_ADDR,
	OPERAND_BYTE,
	OPERAND_VX,
	OPERAND_VY,
	OPERAND_NIBBLE,
	OPERAND_DT = 0,
	OPERAND_ST,
	OPERAND_I,
	OPERAND_K,
	OPERAND_F,
	OPERAND_B,
	OPERAND_MEMINDEX,
	OPERAND_UNKNOWN
};

/* The mnemonic set. */
enum mnemonic {
	MNEMONIC_CLS,
	MNEMONIC_RET,
	MNEMONIC_SYS,
	MNEMONIC_CALL,
	MNEMONIC_JP,
	MNEMONIC_JP_V0,
	MNEMONIC_SE,
	MNEMONIC_SNE,
	MNEMONIC_LD,
	MNEMONIC_ADD,
	MNEMONIC_AND,
	MNEMONIC_OR,
	MNEMONIC_XOR,
	MNEMONIC_SUB,
	MNEMONIC_SHR,
	MNEMONIC_SUBN,
	MNEMONIC_SHL,
	MNEMONIC_RND,
	MNEMONIC_DRW,
	MNEMONIC_SKP,
	MNEMONIC_SKNP,
	MNEMONIC_UNKNOWN
};

/* The textual Symbols for mnemonics. */
static const char mnemonicSymbols[][TOKEN_SIZE] = {
	"CLS",
	"RET",
	"SYS",
	"CALL",
	"JP",
	"JP\tV0",
	"SE",
	"SNE",
	"LD",
	"ADD",
	"AND",
	"OR",
	"XOR",
	"SUB",
	"SHR",
	"SUBN",
	"SHL",
	"RND",
	"DRW",
	"SKP",
	"SKNP"
};

/* Some of the textual Symbols for operands. */
static const char operandSymbols[][TOKEN_SIZE] = {
	"DT",
	"ST",
	"I",
	"K",
	"F",
	"B",
	"[I]",
};

struct instructionType {
	enum mnemonic op;
	enum operand operands[3];
};

static struct instructionType instructionTypes[INTERP_INSTRUCTIONSET_LEN] = {
	{MNEMONIC_CLS, {OPERAND_NONE,	OPERAND_NONE, OPERAND_NONE} },
	{MNEMONIC_RET, {OPERAND_NONE,	OPERAND_NONE, OPERAND_NONE} },
	{MNEMONIC_SYS, {OPERAND_ADDR,	OPERAND_NONE, OPERAND_NONE} },
	{MNEMONIC_JP,  {OPERAND_ADDR,	OPERAND_NONE, OPERAND_NONE} },
	{MNEMONIC_CALL,	{OPERAND_ADDR,	OPERAND_NONE, OPERAND_NONE} },
	{MNEMONIC_SE,  {OPERAND_VX,  OPERAND_BYTE, OPERAND_NONE} },
	{MNEMONIC_SNE, {OPERAND_VX,  OPERAND_BYTE, OPERAND_NONE} },
	{MNEMONIC_SE,  {OPERAND_VX,  OPERAND_VY,   OPERAND_NONE} },
	{MNEMONIC_LD,  {OPERAND_VX,  OPERAND_BYTE, OPERAND_NONE} },
	{MNEMONIC_ADD, {OPERAND_VX,  OPERAND_BYTE, OPERAND_NONE} },
	{MNEMONIC_LD,  {OPERAND_VX,  OPERAND_VY,   OPERAND_NONE} },
	{MNEMONIC_OR,  {OPERAND_VX,  OPERAND_VY,   OPERAND_NONE} },
	{MNEMONIC_AND, {OPERAND_VX,  OPERAND_VY,   OPERAND_NONE} },
	{MNEMONIC_XOR, {OPERAND_VX,  OPERAND_VY,   OPERAND_NONE} },
	{MNEMONIC_ADD, {OPERAND_VX,  OPERAND_VY,   OPERAND_NONE} },
	{MNEMONIC_SUB, {OPERAND_VX,  OPERAND_VY,   OPERAND_NONE} },
	{MNEMONIC_SHR, {OPERAND_VX,  OPERAND_VY,   OPERAND_NONE} },
	{MNEMONIC_SUBN,	{OPERAND_VX,  OPERAND_VY,   OPERAND_NONE} },
	{MNEMONIC_SHL, {OPERAND_VX,  OPERAND_VY,   OPERAND_NONE} },
	{MNEMONIC_SNE, {OPERAND_VX,  OPERAND_VY,   OPERAND_NONE} },
	{MNEMONIC_LD,  {OPERAND_I,   OPERAND_ADDR, OPERAND_NONE} },
	{MNEMONIC_JP_V0,  {OPERAND_ADDR,  OPERAND_NONE, OPERAND_NONE} },
	{MNEMONIC_RND, {OPERAND_VX,  OPERAND_BYTE, OPERAND_NONE} },
	{MNEMONIC_DRW, {OPERAND_VX,  OPERAND_VY,   OPERAND_NIBBLE} },
	{MNEMONIC_SKP, {OPERAND_VX,  OPERAND_NONE, OPERAND_NONE} },
	{MNEMONIC_SKNP,	{OPERAND_VX,  OPERAND_NONE, OPERAND_NONE} },
	{MNEMONIC_LD,  {OPERAND_VX,  OPERAND_DT,   OPERAND_NONE} },
	{MNEMONIC_LD,  {OPERAND_VX,  OPERAND_K,	OPERAND_NONE} },
	{MNEMONIC_LD,  {OPERAND_DT,  OPERAND_VX,   OPERAND_NONE} },
	{MNEMONIC_LD,  {OPERAND_ST,  OPERAND_VX,   OPERAND_NONE} },
	{MNEMONIC_ADD, {OPERAND_I,   OPERAND_VX,   OPERAND_NONE} },
	{MNEMONIC_LD,  {OPERAND_F,   OPERAND_VX,   OPERAND_NONE} },
	{MNEMONIC_LD,  {OPERAND_B,   OPERAND_VX,   OPERAND_NONE } },
	{MNEMONIC_LD,  {OPERAND_MEMINDEX,	OPERAND_VX,		   OPERAND_NONE} },
	{MNEMONIC_LD,  {OPERAND_VX,		  OPERAND_MEMINDEX,	 OPERAND_NONE} }
};


static inline int isBranch(enum interp_instructionId id){
	switch(id){
		case INTERP_SE_VX_BYTE:
		case INTERP_SNE_VX_BYTE:
		case INTERP_SE_VX_VY:
		case INTERP_SNE_VX_VY:
		case INTERP_SKP_VX:
		case INTERP_SKNP_VX:
			return 1;
			break;
		default:
			return 0;
			break;
	}
}

/* If a program is loaded it is translated to an Assembly Language.
 * The Assembly text is then written to _stdout_.
 * Return value is -1 if errors occur, otherwise it is 0.
 */
int assm_disassemble(void)
{ 
	uint8_t addrFlags[ADDR_FLAGS_LEN];
	uint16_t addr; 
	int branchStack[BRANCHES_MAX];
	int* branchPointer;
	interp_instruction instruction;

	addr = INTERP_PROGRAM_START;
	branchPointer = branchStack + 1;

	memset(branchStack, 0, BRANCHES_MAX);
	memset(addrFlags, 0, ADDR_FLAGS_LEN);

	/* The program's control flow is traced to discern program instructions
	 * from sprite data.
	 */
	while(addr < INTERP_MEMORY_LEN && !IS_LOGIC(addr) || ( addr = *--branchPointer ))
	{
		SET_LOGIC(addr);

		if( interp_decode(addr, &instruction) < 0){
			setError("interp_decode: %s", getError() );
			return -1;
		}

		if( isBranch(instruction.id) ){
			*branchPointer++ = addr+2;
			addr+=4;
		}
		else if( instruction.id == INTERP_JP_ADDR || instruction.id == INTERP_JP_V0_ADDR){
			addr = instruction.nnn;
			SET_LABEL(addr);
		}
		else if( instruction.id == INTERP_LD_I_ADDR ){
			SET_LABEL(instruction.nnn);
			addr += 2;
		}
		else if(instruction.id == INTERP_CALL_ADDR){
			*branchPointer++ = addr+2;
			addr = instruction.nnn;
			SET_LABEL(addr);
		}
		else if(instruction.id == INTERP_RET){
			continue;
		}
		else{
			addr+=2;
		}
	}

	int programSize;
	char token[TOKEN_SIZE] = "";
	char mnemonicOutputToken[STRING_SIZE] = "";
	char operandOutputTokens[STRING_SIZE] = "";
	uint8_t spriteData[INTERP_SPRITE_DATA_MAX];
	unsigned int spriteDataCount = 0;

	addr = INTERP_PROGRAM_START;

	/* Encoded instructions are translated to assembly statements,
	 * and sprite data to data directives. Then they are printed to 
	 * _stdout_.
	 */
	memset(spriteData, 0, INTERP_SPRITE_DATA_MAX);

	programSize = interp_getProgramSize();
	while(addr < 0x200 + programSize )
	{
		if(IS_LOGIC(addr))
		{
			if(interp_decode(addr, &instruction) < 0){
				setError("interp_decode: %s", getError() );
				return -1;
			}

			struct instructionType insType = instructionTypes[instruction.id];

			strcpy(mnemonicOutputToken, mnemonicSymbols[insType.op]);
			enum operand *i = insType.operands;

			while(i - insType.operands < 3 && *i != OPERAND_NONE)
			{
				switch(*i){
					case OPERAND_ADDR:
						/* if the memory address operand is not inside the program, or
						 * for some ungodly reason in the middle of an instruction
						 * then that address cannot be labeled and the operand will be a
						 * memory address literal.
						 */
						if( !(instruction.nnn % 2 == 1 && IS_LOGIC(instruction.nnn - 1)) 
							&& (instruction.nnn >= INTERP_PROGRAM_START && instruction.nnn < ( INTERP_PROGRAM_START + programSize) ) 
						  )
							snprintf(token, TOKEN_SIZE, "L%03X" /* a labeled memory address */, instruction.nnn);
						else 
							snprintf(token, TOKEN_SIZE, "#%03X" /* a memory address literal */, instruction.nnn);
						break;
					case OPERAND_BYTE:
						snprintf(token, TOKEN_SIZE, "#%02X", instruction.kk);
						break;
					case OPERAND_VX:
						snprintf(token, TOKEN_SIZE, "V%01X", instruction.x);
						break;
					case OPERAND_VY:
						snprintf(token, TOKEN_SIZE, "V%01X", instruction.y);
						break;
					case OPERAND_NIBBLE:
						snprintf(token, TOKEN_SIZE, "#%01X", instruction.n);
						break;
					default:
						snprintf(token, TOKEN_SIZE, "%s", operandSymbols[*i]);
						break;
				}

				strncat(operandOutputTokens, token, STRING_SIZE - 1);

				if(*++i != OPERAND_NONE)
				{   // Space if more argument tokens follow
					strncat(operandOutputTokens, " ", STRING_SIZE - 1);
				}

			}

			if(IS_LABEL(addr)) printf("L%03X\t%s\t%-20s;%04X\n", addr, mnemonicOutputToken, operandOutputTokens, WORD(addr));
			else			   printf("\t%s\t%-20s;%04X\n", mnemonicOutputToken, operandOutputTokens, WORD(addr));

			addr += 2;
		}
		else 
		{
			/* When a segment of sprite data is encountered, it is collected in 
			 * _spriteData_.
			 */
			spriteData[spriteDataCount++] = interp_readMemory(addr);

			++addr;
			if( addr >= INTERP_PROGRAM_START + programSize || IS_LOGIC(addr) || IS_LABEL(addr) ){

				/* When the end of that segment arrives it is printed, and
				 * _spriteData_ is set to be overwritten by the next segment.
				 */
				int i = 0;
				char label[TOKEN_SIZE]; 


				printf("L%03X", (addr - spriteDataCount));
				while( i < spriteDataCount ){
					strncpy(mnemonicOutputToken, "db", TOKEN_SIZE);

					for(int j=0; i < spriteDataCount && j < 4; j++, i++){
						snprintf(token, TOKEN_SIZE, "#%02X", spriteData[i]);

						strncat(operandOutputTokens, token, STRING_SIZE - 1);

						if(j+1 < 4 && i+1 < spriteDataCount)
						{   //space if more byte tokens follow 
							strncat(operandOutputTokens, " ", STRING_SIZE - 1);
						}
					}
					printf("\t%s\t%-10s\n", mnemonicOutputToken, operandOutputTokens);
					operandOutputTokens[0] = '\0';
				}
				spriteDataCount = 0;

			}
		}
		operandOutputTokens[0] = '\0';

	}

	return 0;
} 

typedef char* token_t;
typedef token_t* statement_t;

#define TOKEN_INIT (memset(calloc(sizeof(char), TOKEN_SIZE), '\0', sizeof(char)))
#define STATEMENT_INIT(N) (memset(calloc(sizeof(char*), N), 0, sizeof(char*) * N))

#define WHITESPACE " \t\n"
#define ISWHITESPACE(C) ((C) == ' ' || (C) == '\t' || (C) == '\n')

#define OPERAND_XY OPERAND_VX

#define ENCODE_OPERAND_1(N) ( (N + 6) )
#define ENCODE_OPERAND_2(N) ( (N + 6) << 4 )
#define ENCODE_OPERAND_3(N) ( (N + 6) << 8 )

static inline int createStatement(statement_t s, char* line){
	token_t token; 

	token = strtok(line, WHITESPACE);
	while(ISWHITESPACE(*token) && token++);
	for(int i = 0; token != NULL && *token != ';'; token = strtok(NULL, WHITESPACE), *s[++i] = '\0'  ){
		strcpy(s[i], token);
	}
	return 0;
}

static inline int isLabelSym(token_t tok){
	if(strlen(tok) <= 8 && tok[0] == 'L' && strcmp(tok, "LD") != 0){
		for(int i=1; i <= 8 && tok[i] != '\0'; i++)
			if( !isalnum( tok[i] ) )
				return 0;
		return 1;
	}
	else return 0;
}

static inline int isAddressLiteral(token_t tok){
	if(strlen(tok) == 4 && tok[0] == '#'){
		for(int i=1; i < 4; i++)
			if(!isxdigit(tok[i]))
				return 0;
		return 1;
	}
	else return 0;
}

static inline int isByteSym(token_t tok){
	if(strlen(tok) == 3 && tok[0] == '#'){
		for(int i=1; i < 3; i++)
			if(!isxdigit(tok[i]))
				return 0;
		return 1;
	}
	else return 0;
}

static inline int isNibbleSym(token_t tok){
	if(strlen(tok) == 2 && tok[0] == '#' && isxdigit(tok[1]))
		return 1;
	
	else return 0;
}

static inline int isxySym(token_t tok){
	if(strlen(tok) == 2 && tok[0] == 'V' && isxdigit(tok[1]))
		return 1;
	
	else return 0;
}

int assm_assemble(const char* fileName)
{
	size_t lineSize = STRING_SIZE;
	char* line = malloc(STRING_SIZE);
	int lineNumber = 1;
	int addr = INTERP_PROGRAM_START;
	int operands[3];
	util_linkedDictionary incompleteInstructions;
	util_dictionary labels;

	/* Dictionaries (or hashtables) are created for instructions that are 
	 * incomplete (as in the address of a label is not yet known) and for 
	 * labels themselves
	 */
	if( util_createLinkedDictionary(&incompleteInstructions) < 0 ){
		setError("util_createLinkedDictionary: %s", getError() );
		return -1;
	}

	if( util_createDictionary(&labels) < 0){
		setError("util_createDictionary: %s", getError() );
		return -1;
	}

	FILE* assemblyFile = fopen(fileName, "r");

	if(assemblyFile == NULL){
		setError("fopen: %s", strerror(errno));
		return -1;
	}

	/* A statement type, _statement_t_, is used for parsing text.
	 * Theses statement types are arrays of token types _token_t_.
	 */
	statement_t statement = STATEMENT_INIT(8);
	for(int i = 0; i < 8; statement[i++] = TOKEN_INIT);

	while(getline(&line, &lineSize, assemblyFile) > 0){

		int statementIndex = 0;
		interp_instruction instruction;

		instruction.id = INTERP_INSTRUCTION_UNKNOWN;
		instruction.nnn = 
			instruction.kk = 
			instruction.x = 
			instruction.y = 
			instruction.n = 0; 

		if(createStatement(statement, line) < 0 ){
			setError("createStatement: %s", getError() );
			return -1;
		}

		/* If the first token is a label then the address 
		 * of this instruction is recorded in _labels_
		 */
		if( isLabelSym(statement[statementIndex]) ){
			if( util_insert(labels, 
					&statement[statementIndex][1],
					addr) < 0){
				setError("util_insert: %s", getError() );
				return -1;
			}

			util_linkedList* vals;

			/* Then _incompleteInstructions_ is searched for
			 * this label.
			 *
			 * If any incomplete instructions are found they are 
			 * completed now and deleted from _incompleteInstructions_.
			 */
			if( util_linkedSearch(incompleteInstructions,
					&statement[statementIndex][1],
					&vals) ) 
			{
				int incompleteLowByte;

				do {
					incompleteLowByte = WORD(vals->val);
					incompleteLowByte |= addr;
					if( interp_writeMemory(vals->val, incompleteLowByte >> 8) < 0){
						setError("interp_writeMemory: %s", getError() );
						return -1;
					}
					if( interp_writeMemory(vals->val + 1, incompleteLowByte & 0xff) < 0){
						setError("interp_writeMemory: %s", getError() );
						return -1;
					}
					vals = vals->next;
				} while(vals != NULL);

				util_linkedDelete(incompleteInstructions, 
						&statement[statementIndex][1]);
			}
			++statementIndex;
		}


		if(strcmp(statement[statementIndex], "db") == 0)
		{	//is data directive 
			while(isByteSym(statement[++statementIndex])){
				if( interp_writeMemory(addr++, 
							(uint8_t)strtol(&statement[statementIndex][1], 
								(char**)NULL, 
								16 ) ) < 0 )
				{
					setError("interp_writeMemory: %s", getError());
					return -1;
				}
			}

			if(statement[statementIndex][0] != '\0')
				setError("%d: illegal Argument \"%s\" in data directive", 
						lineNumber, 
						statement[statementIndex]);
		}
		else
		{
			enum mnemonic op;
			uint8_t isy = 0;
			enum operand operandTokens[3] = {
				OPERAND_NONE, OPERAND_NONE, OPERAND_NONE
			};

            /* The mnemonic token is compared with each symbol in the mnemonic set. */
			for(op = MNEMONIC_CLS; 
					op < MNEMONIC_UNKNOWN && strcmp(statement[statementIndex], mnemonicSymbols[op]) != 0; 
					op++);
			if(op == MNEMONIC_UNKNOWN){
				setError("%d: unknown mnemonic \"%s\"", 
						lineNumber, 
						statement[statementIndex]);
				return -1;
			}

            // A much more intuitive way of handling that jump operation than the garbage I was doing.
            if( strcmp(statement[++statementIndex], "V0") == 0 && op == MNEMONIC_JP ){
                op = MNEMONIC_JP_V0;
            }
                

            /* For each operand token, */
			for(uint8_t operandIndex = 0; 
					operandIndex < 3 && *statement[statementIndex] != '\0'; 
					++statementIndex, ++operandIndex){

                /* that operand token is compared with some of the 
                 * symbols that are invariable. */
				for(
				operandTokens[operandIndex] = OPERAND_DT; 
				operandTokens[operandIndex] < OPERAND_UNKNOWN 
					&& strcmp(statement[statementIndex], operandSymbols[operandTokens[operandIndex]]) != 0; 
				operandTokens[operandIndex]++
				);

                /* If the operand is not one those symbols
                 * then it's variable; which is to say it could be one of 
                 * a memory address label, memory address literal, byte literal, 
                 * register address, or nibble literal. */
				if(operandTokens[operandIndex] == OPERAND_UNKNOWN)
				{
                    // label
					if( isLabelSym(statement[statementIndex]) ){
						int val;

						operandTokens[operandIndex] = OPERAND_ADDR;

						if(util_search(labels, 
									&statement[statementIndex][1],
									&val) ) 
							instruction.nnn = val;

						else if( util_linkedInsert(incompleteInstructions,
									&statement[statementIndex][1],
									addr) < 0 ){
							setError("util_linkedInsert: %s:\n\t%d: %s",
								getError(),
								lineNumber,
								line);
						}
					}
                    // memory address literal
					else if( isAddressLiteral(statement[statementIndex]) ){
						operandTokens[operandIndex] = OPERAND_ADDR;
						instruction.nnn = (int)strtol(&statement[statementIndex][1], (char**)NULL, 16);
					}
                    // byte literal
					else if( isByteSym(statement[statementIndex]) ){
						operandTokens[operandIndex] = OPERAND_BYTE;
						instruction.kk = (int)strtol(&statement[statementIndex][1], (char**)NULL, 16);
					}
                    // register address literal
					else if( isxySym(statement[statementIndex])){
						operandTokens[operandIndex] = OPERAND_XY;
						if(isy++)
							instruction.y = (int)strtol(&statement[statementIndex][1], (char**)NULL, 16);
						else
							instruction.x = (int)strtol(&statement[statementIndex][1], (char**)NULL, 16);
					}
                    // nibble literal
					else if( isNibbleSym(statement[statementIndex]) ){
						operandTokens[operandIndex] = OPERAND_NIBBLE;
						instruction.n = (int)strtol(&statement[statementIndex][1], (char**)NULL, 16);
					}
				}

				if(operandTokens[operandIndex] == OPERAND_UNKNOWN) 
					setError("%d: unknown operand %s", 
							lineNumber, 
							statement[statementIndex]);
			}

            // and this is absolutely garbage!
			++statementIndex;
			switch(ENCODE_OPERAND_1(operandTokens[0]) | ENCODE_OPERAND_2(operandTokens[1]) | ENCODE_OPERAND_3(operandTokens[2]))
			{
				case 0 | 0 | 0:
					switch(op){
						case MNEMONIC_CLS:
							instruction.id = INTERP_CLS;
							break;
						case MNEMONIC_RET:
							instruction.id = INTERP_RET;
							break;
					}
					break;
				case ENCODE_OPERAND_1(OPERAND_XY) | ENCODE_OPERAND_2(OPERAND_XY) | 0:
					switch(op){
						case MNEMONIC_SE:
							instruction.id = INTERP_SE_VX_VY;
							break;
						case MNEMONIC_LD:
							instruction.id = INTERP_LD_VX_VY;
							break;
						case MNEMONIC_OR:
							instruction.id = INTERP_OR_VX_VY;
							break;
						case MNEMONIC_AND:
							instruction.id = INTERP_AND_VX_VY;
							break;
						case MNEMONIC_XOR:
							instruction.id = INTERP_XOR_VX_VY;
							break;
						case MNEMONIC_ADD:
							instruction.id = INTERP_ADD_VX_VY;
							break;
						case MNEMONIC_SUB:
							instruction.id = INTERP_SUB_VX_VY;
							break;
						case MNEMONIC_SHR:
							instruction.id = INTERP_SHR_VX_VY;
							break;
						case MNEMONIC_SUBN:
							instruction.id = INTERP_SUBN_VX_VY;
							break;
						case MNEMONIC_SHL:
							instruction.id = INTERP_SHL_VX_VY;
							break;
						case MNEMONIC_SNE:
							instruction.id = INTERP_SNE_VX_VY;
							break;
					}
					break;
				case ENCODE_OPERAND_1(OPERAND_XY) | ENCODE_OPERAND_2(OPERAND_XY) | ENCODE_OPERAND_3(OPERAND_NIBBLE):
					if(op == MNEMONIC_DRW)
						instruction.id = INTERP_DRW_VX_VY_NIBBLE;
					break;
				case ENCODE_OPERAND_1(OPERAND_XY) | ENCODE_OPERAND_2(OPERAND_BYTE) | 0:
					switch(op){
						case MNEMONIC_SE:
							instruction.id = INTERP_SE_VX_BYTE;
							break;
						case MNEMONIC_SNE:
							instruction.id = INTERP_SNE_VX_BYTE;
							break;
						case MNEMONIC_LD:
							instruction.id = INTERP_LD_VX_BYTE;
							break;
						case MNEMONIC_ADD:
							instruction.id = INTERP_ADD_VX_BYTE;
							break;
						case MNEMONIC_RND:
							instruction.id = INTERP_RND_VX_BYTE;
							break;
                    }
					break;
				case ENCODE_OPERAND_1(OPERAND_XY) | ENCODE_OPERAND_2(OPERAND_DT) | 0:
					if(op == MNEMONIC_LD)
				        instruction.id = INTERP_LD_VX_DT;
					break;
				case ENCODE_OPERAND_1(OPERAND_XY) | ENCODE_OPERAND_2(OPERAND_K) | 0:
					if(op == MNEMONIC_LD)
						instruction.id = INTERP_LD_VX_K;
					break;
				case ENCODE_OPERAND_1(OPERAND_XY) | ENCODE_OPERAND_2(OPERAND_MEMINDEX) | 0:
					if(op == MNEMONIC_LD)
						instruction.id = INTERP_LD_VX_MEMINDEX;
					break;
				case ENCODE_OPERAND_1(OPERAND_XY) | 0 | 0:
					switch(op){
						case MNEMONIC_SKP:
							instruction.id = INTERP_SKP_VX;
							break;
						case MNEMONIC_SKNP:
							instruction.id = INTERP_SKNP_VX;
							break;
					}
					break;
				case ENCODE_OPERAND_1(OPERAND_DT) | ENCODE_OPERAND_2(OPERAND_XY) | 0:
					if(op == MNEMONIC_LD)
						instruction.id = INTERP_LD_DT_VX;
					break;
				case ENCODE_OPERAND_1(OPERAND_ST) | ENCODE_OPERAND_2(OPERAND_XY) | 0:
					if(op == MNEMONIC_LD)
						instruction.id = INTERP_LD_ST_VX;
					break;
				case ENCODE_OPERAND_1(OPERAND_I) | ENCODE_OPERAND_2(OPERAND_XY) | 0:
					if(op == MNEMONIC_ADD)
						instruction.id = INTERP_ADD_I_VX;
					break;
				case ENCODE_OPERAND_1(OPERAND_F) | ENCODE_OPERAND_2(OPERAND_XY) | 0:
					if(op == MNEMONIC_LD)
						instruction.id = INTERP_LD_F_VX;
					break;
				case ENCODE_OPERAND_1(OPERAND_B) | ENCODE_OPERAND_2(OPERAND_XY) | 0:
					if(op == MNEMONIC_LD)
						instruction.id = INTERP_LD_B_VX;
					break;
				case ENCODE_OPERAND_1(OPERAND_MEMINDEX) | ENCODE_OPERAND_2(OPERAND_XY) | 0:
					if(op == MNEMONIC_LD)
						instruction.id = INTERP_LD_MEMINDEX_VX;
					break;
				case ENCODE_OPERAND_1(OPERAND_I) | ENCODE_OPERAND_2(OPERAND_ADDR) | 0:
					if(op == MNEMONIC_LD)
						instruction.id = INTERP_LD_I_ADDR;
					break;
				case ENCODE_OPERAND_1(OPERAND_ADDR) | 0 | 0:
					switch(op){
						case MNEMONIC_JP:
							instruction.id = INTERP_JP_ADDR;
							break;
						case MNEMONIC_JP_V0:
							instruction.id = INTERP_JP_V0_ADDR;
							break;
						case MNEMONIC_CALL:
							instruction.id = INTERP_CALL_ADDR;
							break;
					}
					break;
			}

			if(instruction.id == INTERP_INSTRUCTION_UNKNOWN){
				setError("%d: illegal operands for %s mnemonic\n\t%s", 
						lineNumber, 
						mnemonicSymbols[op], 
						line);
				return -1;
			}
			else { 
				if( interp_encode(addr, instruction) < 0 ){
					setError("interp_encode: %s", getError() );
					return -1;
				}
			}

			addr += 2;
		}

		lineNumber++;
	}

	return 0;
}
