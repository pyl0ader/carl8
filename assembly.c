#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "util.h"
#include "logError.h"
#include "interpreter.h"

#define STRING_SIZE 80
#define TOKEN_SIZE 5
#define BRANCHES_MAX ( (INTERP_MEMORY_LEN - INTERP_PROGRAM_START) / 2 )
#define ADDR_FLAGS_LEN (INTERP_MEMORY_LEN / 4)

#define WORD(ADDR) ( interp_readMemory(ADDR) << 8 | interp_readMemory((ADDR) + 1) )

#define SET_LOGIC(ADDR) (addrFlags[(ADDR) >> 2] |= 1 << ((ADDR) & 3) * 2 + 0 )
#define IS_LOGIC(ADDR)  (addrFlags[(ADDR) >> 2] & (1 << ((ADDR) & 3) * 2 + 0 )  )

#define SET_LABEL(ADDR) (addrFlags[(ADDR) >> 2] |= 1 << ((ADDR) & 3) * 2 + 1)
#define IS_LABEL(ADDR)  (addrFlags[(ADDR) >> 2] & (1 << ((ADDR) & 3) * 2 + 1)  )

enum parameter {
	PARAMETER_NONE = -7,
	PARAMETER_ADDR,
	PARAMETER_BYTE,
	PARAMETER_VX,
	PARAMETER_VY,
	PARAMETER_NIBBLE,
	PARAMETER_V0 = 0,
	PARAMETER_DT,
	PARAMETER_ST,
	PARAMETER_I,
	PARAMETER_K,
	PARAMETER_F,
	PARAMETER_B,
	PARAMETER_MEMINDEX,
	PARAMETER_UNKNOWN
};

enum operation {
	OPERATION_CLS,
	OPERATION_RET,
	OPERATION_SYS,
	OPERATION_CALL,
	OPERATION_JP,
	OPERATION_SE,
	OPERATION_SNE,
	OPERATION_LD,
	OPERATION_ADD,
	OPERATION_AND,
	OPERATION_OR,
	OPERATION_XOR,
	OPERATION_SUB,
	OPERATION_SHR,
	OPERATION_SUBN,
	OPERATION_SHL,
	OPERATION_RND,
	OPERATION_DRW,
	OPERATION_SKP,
	OPERATION_SKNP,
	OPERATION_UNKNOWN
};

static const char operationSymbols[][TOKEN_SIZE] = {
	"CLS",
	"RET",
	"SYS",
	"CALL",
	"JP",
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

static const char parameterSymbols[][TOKEN_SIZE] = {
	"V0",
	"DT",
	"ST",
	"I",
	"K",
	"F",
	"B",
	"[I]",
};

struct instructionSymbol {
	enum operation op;
	enum parameter params[3];
};

static struct instructionSymbol instructionSymbols[INTERP_INSTRUCTIONSET_LEN] = {
	{OPERATION_CLS, {PARAMETER_NONE,	PARAMETER_NONE, PARAMETER_NONE} },
	{OPERATION_RET, {PARAMETER_NONE,	PARAMETER_NONE, PARAMETER_NONE} },
	{OPERATION_SYS, {PARAMETER_ADDR,	PARAMETER_NONE, PARAMETER_NONE} },
	{OPERATION_JP,  {PARAMETER_ADDR,	PARAMETER_NONE, PARAMETER_NONE} },
	{OPERATION_CALL,	{PARAMETER_ADDR,	PARAMETER_NONE, PARAMETER_NONE} },
	{OPERATION_SE,  {PARAMETER_VX,  PARAMETER_BYTE, PARAMETER_NONE} },
	{OPERATION_SNE, {PARAMETER_VX,  PARAMETER_BYTE, PARAMETER_NONE} },
	{OPERATION_SE,  {PARAMETER_VX,  PARAMETER_VY,   PARAMETER_NONE} },
	{OPERATION_LD,  {PARAMETER_VX,  PARAMETER_BYTE, PARAMETER_NONE} },
	{OPERATION_ADD, {PARAMETER_VX,  PARAMETER_BYTE, PARAMETER_NONE} },
	{OPERATION_LD,  {PARAMETER_VX,  PARAMETER_VY,   PARAMETER_NONE} },
	{OPERATION_OR,  {PARAMETER_VX,  PARAMETER_VY,   PARAMETER_NONE} },
	{OPERATION_AND, {PARAMETER_VX,  PARAMETER_VY,   PARAMETER_NONE} },
	{OPERATION_XOR, {PARAMETER_VX,  PARAMETER_VY,   PARAMETER_NONE} },
	{OPERATION_ADD, {PARAMETER_VX,  PARAMETER_VY,   PARAMETER_NONE} },
	{OPERATION_SUB, {PARAMETER_VX,  PARAMETER_VY,   PARAMETER_NONE} },
	{OPERATION_SHR, {PARAMETER_VX,  PARAMETER_VY,   PARAMETER_NONE} },
	{OPERATION_SUBN,	{PARAMETER_VX,  PARAMETER_VY,   PARAMETER_NONE} },
	{OPERATION_SHL, {PARAMETER_VX,  PARAMETER_VY,   PARAMETER_NONE} },
	{OPERATION_SNE, {PARAMETER_VX,  PARAMETER_VY,   PARAMETER_NONE} },
	{OPERATION_LD,  {PARAMETER_I,   PARAMETER_ADDR, PARAMETER_NONE} },
	{OPERATION_JP,  {PARAMETER_V0,  PARAMETER_ADDR, PARAMETER_NONE} },
	{OPERATION_RND, {PARAMETER_VX,  PARAMETER_BYTE, PARAMETER_NONE} },
	{OPERATION_DRW, {PARAMETER_VX,  PARAMETER_VY,   PARAMETER_NIBBLE} },
	{OPERATION_SKP, {PARAMETER_VX,  PARAMETER_NONE, PARAMETER_NONE} },
	{OPERATION_SKNP,	{PARAMETER_VX,  PARAMETER_NONE, PARAMETER_NONE} },
	{OPERATION_LD,  {PARAMETER_VX,  PARAMETER_DT,   PARAMETER_NONE} },
	{OPERATION_LD,  {PARAMETER_VX,  PARAMETER_K,	PARAMETER_NONE} },
	{OPERATION_LD,  {PARAMETER_DT,  PARAMETER_VX,   PARAMETER_NONE} },
	{OPERATION_LD,  {PARAMETER_ST,  PARAMETER_VX,   PARAMETER_NONE} },
	{OPERATION_ADD, {PARAMETER_I,   PARAMETER_VX,   PARAMETER_NONE} },
	{OPERATION_LD,  {PARAMETER_F,   PARAMETER_VX,   PARAMETER_NONE} },
	{OPERATION_LD,  {PARAMETER_B,   PARAMETER_VX,   PARAMETER_NONE } },
	{OPERATION_LD,  {PARAMETER_MEMINDEX,	PARAMETER_VX,		   PARAMETER_NONE} },
	{OPERATION_LD,  {PARAMETER_VX,		  PARAMETER_MEMINDEX,	 PARAMETER_NONE} }
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
	char op[STRING_SIZE] = "";
	char arguments[STRING_SIZE] = "";
	uint8_t spriteData[INTERP_SPRITE_DATA_MAX];
	unsigned int spriteDataCount = 0;

	addr = INTERP_PROGRAM_START;

	/* Encoded instructions are translated to symbolic instructions,
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

			struct instructionSymbol insSym = instructionSymbols[instruction.id];

			strcpy(op, operationSymbols[insSym.op]);
			enum parameter *i = insSym.params;

			while(i - insSym.params < 3 && *i != PARAMETER_NONE)
			{
				switch(*i){
					case PARAMETER_ADDR:
						/* if the address parameter is not inside the program, or
						 * for some god known reason in the middle of an instruction
						 * then that address can not be labeled and the parameter will be an 
						 * address literal
						 */
						if( !(instruction.nnn % 2 == 1 && IS_LOGIC(instruction.nnn - 1)) 
							&& (instruction.nnn >= INTERP_PROGRAM_START && instruction.nnn < ( INTERP_PROGRAM_START + programSize) ) 
						  )
							snprintf(token, TOKEN_SIZE, "L%03X" /* a labeled address */, instruction.nnn);
						else 
							snprintf(token, TOKEN_SIZE, "#%03X" /* an address literal */, instruction.nnn);
						break;
					case PARAMETER_BYTE:
						snprintf(token, TOKEN_SIZE, "#%02X", instruction.kk);
						break;
					case PARAMETER_VX:
						snprintf(token, TOKEN_SIZE, "V%01X", instruction.x);
						break;
					case PARAMETER_VY:
						snprintf(token, TOKEN_SIZE, "V%01X", instruction.y);
						break;
					case PARAMETER_NIBBLE:
						snprintf(token, TOKEN_SIZE, "#%01X", instruction.n);
						break;
					default:
						snprintf(token, TOKEN_SIZE, "%s", parameterSymbols[*i]);
						break;
				}

				strncat(arguments, token, STRING_SIZE - 1);

				if(*++i != PARAMETER_NONE)
				{   //Space if more argument tokens follow
					strncat(arguments, " ", STRING_SIZE - 1);
				}

			}

			if(IS_LABEL(addr)) printf("L%03X\t%s\t%-20s;%04X\n", addr, op, arguments, WORD(addr));
			else			   printf("\t%s\t%-20s;%04X\n", op, arguments, WORD(addr));

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
					strncpy(op, "db", TOKEN_SIZE);

					for(int j=0; i < spriteDataCount && j < 4; j++, i++){
						snprintf(token, TOKEN_SIZE, "#%02X", spriteData[i]);

						strncat(arguments, token, STRING_SIZE - 1);

						if(j+1 < 4 && i+1 < spriteDataCount)
						{   //space if more byte tokens follow 
							strncat(arguments, " ", STRING_SIZE - 1);
						}
					}
					printf("\t%s\t%-10s\n", op, arguments);
					arguments[0] = '\0';
				}
				spriteDataCount = 0;

			}
		}
		arguments[0] = '\0';

	}

	return 0;
} 

typedef char* token_t;
typedef token_t* statement_t;

#define TOKEN_INIT (memset(calloc(sizeof(char), TOKEN_SIZE), '\0', sizeof(char)))
#define STATEMENT_INIT(N) (memset(calloc(sizeof(char*), N), 0, sizeof(char*) * N))

#define WHITESPACE " \t\n"
#define ISWHITESPACE(C) ((C) == ' ' || (C) == '\t' || (C) == '\n')

#define PARAMETER_XY PARAMETER_VX
#define ARG_1(N) ( (N + 7) )
#define ARG_2(N) ( (N + 7) << 4 )
#define ARG_3(N) ( (N + 7) << 8 )

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
	int parameters[3];
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
			enum operation op;
			uint8_t isy = 0;
			enum parameter parameterTokens[3] = {
				PARAMETER_NONE, PARAMETER_NONE, PARAMETER_NONE
			};

			for(op = OPERATION_CLS; 
					op < OPERATION_UNKNOWN && strcmp(statement[statementIndex], operationSymbols[op]) != 0; 
					op++);
			if(op == OPERATION_UNKNOWN){
				setError("%d: unknown operation \"%s\"", 
						lineNumber, 
						statement[statementIndex]);
				return -1;
			}

			++statementIndex;
			for(uint8_t parameterIndex = 0; 
					parameterIndex < 3 && *statement[statementIndex] != '\0'; 
					++statementIndex, ++parameterIndex){

				for(
				parameterTokens[parameterIndex] = PARAMETER_V0; 
				parameterTokens[parameterIndex] < PARAMETER_UNKNOWN 
					&& strcmp(statement[statementIndex], parameterSymbols[parameterTokens[parameterIndex]]) != 0; 
				parameterTokens[parameterIndex]++
				);

				if(parameterTokens[parameterIndex] == PARAMETER_V0)
					isy++;

				if(parameterTokens[parameterIndex] == PARAMETER_UNKNOWN)
				{
					if( isLabelSym(statement[statementIndex]) ){
						int val;

						parameterTokens[parameterIndex] = PARAMETER_ADDR;

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
					else if( isAddressLiteral(statement[statementIndex]) ){
						parameterTokens[parameterIndex] = PARAMETER_ADDR;
						instruction.nnn = (int)strtol(&statement[statementIndex][1], (char**)NULL, 16);
					}
					else if( isByteSym(statement[statementIndex]) ){
						parameterTokens[parameterIndex] = PARAMETER_BYTE;
						instruction.kk = (int)strtol(&statement[statementIndex][1], (char**)NULL, 16);
					}
					else if( isxySym(statement[statementIndex])){
						parameterTokens[parameterIndex] = PARAMETER_XY;
						if(isy++)
							instruction.y = (int)strtol(&statement[statementIndex][1], (char**)NULL, 16);
						else
							instruction.x = (int)strtol(&statement[statementIndex][1], (char**)NULL, 16);
					}
					else if( isNibbleSym(statement[statementIndex]) ){
						parameterTokens[parameterIndex] = PARAMETER_NIBBLE;
						instruction.n = (int)strtol(&statement[statementIndex][1], (char**)NULL, 16);
					}
				}

				if(parameterTokens[parameterIndex] == PARAMETER_UNKNOWN) 
					setError("%d: unknown parameter %s", 
							lineNumber, 
							statement[statementIndex]);
			}

			++statementIndex;
			switch(ARG_1(parameterTokens[0]) | ARG_2(parameterTokens[1]) | ARG_3(parameterTokens[2]))
			{
				case ARG_1(PARAMETER_NONE) | ARG_2(PARAMETER_NONE) | ARG_3(PARAMETER_NONE):
					switch(op){
						case OPERATION_CLS:
							instruction.id = INTERP_CLS;
							break;
						case OPERATION_RET:
							instruction.id = INTERP_RET;
							break;
					}
					break;
				case ARG_1(PARAMETER_XY) | ARG_2(PARAMETER_XY) | ARG_3(PARAMETER_NONE):
				case ARG_1(PARAMETER_V0) | ARG_2(PARAMETER_XY) | ARG_3(PARAMETER_NONE):
				case ARG_1(PARAMETER_XY) | ARG_2(PARAMETER_V0) | ARG_3(PARAMETER_NONE):
				case ARG_1(PARAMETER_V0) | ARG_2(PARAMETER_V0) | ARG_3(PARAMETER_NONE):
					switch(op){
						case OPERATION_SE:
							instruction.id = INTERP_SE_VX_VY;
							break;
						case OPERATION_LD:
							instruction.id = INTERP_LD_VX_VY;
							break;
						case OPERATION_OR:
							instruction.id = INTERP_OR_VX_VY;
							break;
						case OPERATION_AND:
							instruction.id = INTERP_AND_VX_VY;
							break;
						case OPERATION_XOR:
							instruction.id = INTERP_XOR_VX_VY;
							break;
						case OPERATION_ADD:
							instruction.id = INTERP_ADD_VX_VY;
							break;
						case OPERATION_SUB:
							instruction.id = INTERP_SUB_VX_VY;
							break;
						case OPERATION_SHR:
							instruction.id = INTERP_SHR_VX_VY;
							break;
						case OPERATION_SUBN:
							instruction.id = INTERP_SUBN_VX_VY;
							break;
						case OPERATION_SHL:
							instruction.id = INTERP_SHL_VX_VY;
							break;
						case OPERATION_SNE:
							instruction.id = INTERP_SNE_VX_VY;
							break;
					}
					break;
				case ARG_1(PARAMETER_XY) | ARG_2(PARAMETER_XY) | ARG_3(PARAMETER_NIBBLE):
				case ARG_1(PARAMETER_V0) | ARG_2(PARAMETER_XY) | ARG_3(PARAMETER_NIBBLE):
				case ARG_1(PARAMETER_XY) | ARG_2(PARAMETER_V0) | ARG_3(PARAMETER_NIBBLE):
				case ARG_1(PARAMETER_V0) | ARG_2(PARAMETER_V0) | ARG_3(PARAMETER_NIBBLE):
					if(op == OPERATION_DRW)
						instruction.id = INTERP_DRW_VX_VY_NIBBLE;
					break;
				case ARG_1(PARAMETER_XY) | ARG_2(PARAMETER_BYTE) | ARG_3(PARAMETER_NONE):
				case ARG_1(PARAMETER_V0) | ARG_2(PARAMETER_BYTE) | ARG_3(PARAMETER_NONE):
					switch(op){
						case OPERATION_SE:
							instruction.id = INTERP_SE_VX_BYTE;
							break;
						case OPERATION_SNE:
							instruction.id = INTERP_SNE_VX_BYTE;
							break;
						case OPERATION_LD:
							instruction.id = INTERP_LD_VX_BYTE;
							break;
						case OPERATION_ADD:
							instruction.id = INTERP_ADD_VX_BYTE;
							break;
						case OPERATION_RND:
							instruction.id = INTERP_RND_VX_BYTE;
							break;
					}
					break;
				case ARG_1(PARAMETER_XY) | ARG_2(PARAMETER_DT) | ARG_3(PARAMETER_NONE):
				case ARG_1(PARAMETER_V0) | ARG_2(PARAMETER_DT) | ARG_3(PARAMETER_NONE):
					if(op == OPERATION_LD)
						instruction.id = INTERP_LD_VX_DT;
					break;
				case ARG_1(PARAMETER_XY) | ARG_2(PARAMETER_K) | ARG_3(PARAMETER_NONE):
				case ARG_1(PARAMETER_V0) | ARG_2(PARAMETER_K) | ARG_3(PARAMETER_NONE):
					if(op == OPERATION_LD)
						instruction.id = INTERP_LD_VX_K;
					break;
				case ARG_1(PARAMETER_XY) | ARG_2(PARAMETER_MEMINDEX) | ARG_3(PARAMETER_NONE):
				case ARG_1(PARAMETER_V0) | ARG_2(PARAMETER_MEMINDEX) | ARG_3(PARAMETER_NONE):
					if(op == OPERATION_LD)
						instruction.id = INTERP_LD_VX_MEMINDEX;
					break;
				case ARG_1(PARAMETER_XY) | ARG_2(PARAMETER_NONE) | ARG_3(PARAMETER_NONE):
				case ARG_1(PARAMETER_V0) | ARG_2(PARAMETER_NONE) | ARG_3(PARAMETER_NONE):
					switch(op){
						case OPERATION_SKP:
							instruction.id = INTERP_SKP_VX;
							break;
						case OPERATION_SKNP:
							instruction.id = INTERP_SKNP_VX;
							break;
					}
					break;
				case ARG_1(PARAMETER_DT) | ARG_2(PARAMETER_XY) | ARG_3(PARAMETER_NONE):
				case ARG_1(PARAMETER_DT) | ARG_2(PARAMETER_V0) | ARG_3(PARAMETER_NONE):
					if(op == OPERATION_LD)
						instruction.id = INTERP_LD_DT_VX;
					break;
				case ARG_1(PARAMETER_ST) | ARG_2(PARAMETER_XY) | ARG_3(PARAMETER_NONE):
				case ARG_1(PARAMETER_ST) | ARG_2(PARAMETER_V0) | ARG_3(PARAMETER_NONE):
					if(op == OPERATION_LD)
						instruction.id = INTERP_LD_ST_VX;
					break;
				case ARG_1(PARAMETER_I) | ARG_2(PARAMETER_XY) | ARG_3(PARAMETER_NONE):
				case ARG_1(PARAMETER_I) | ARG_2(PARAMETER_V0) | ARG_3(PARAMETER_NONE):
					if(op == OPERATION_ADD)
						instruction.id = INTERP_ADD_I_VX;
					break;
				case ARG_1(PARAMETER_F) | ARG_2(PARAMETER_XY) | ARG_3(PARAMETER_NONE):
				case ARG_1(PARAMETER_F) | ARG_2(PARAMETER_V0) | ARG_3(PARAMETER_NONE):
					if(op == OPERATION_LD)
						instruction.id = INTERP_LD_F_VX;
					break;
				case ARG_1(PARAMETER_B) | ARG_2(PARAMETER_XY) | ARG_3(PARAMETER_NONE):
				case ARG_1(PARAMETER_B) | ARG_2(PARAMETER_V0) | ARG_3(PARAMETER_NONE):
					if(op == OPERATION_LD)
						instruction.id = INTERP_LD_B_VX;
					break;
				case ARG_1(PARAMETER_MEMINDEX) | ARG_2(PARAMETER_XY) | ARG_3(PARAMETER_NONE):
				case ARG_1(PARAMETER_MEMINDEX) | ARG_2(PARAMETER_V0) | ARG_3(PARAMETER_NONE):
					if(op == OPERATION_LD)
						instruction.id = INTERP_LD_MEMINDEX_VX;
					break;
				case ARG_1(PARAMETER_I) | ARG_2(PARAMETER_ADDR) | ARG_3(PARAMETER_NONE):
					if(op == OPERATION_LD)
						instruction.id = INTERP_LD_I_ADDR;
					break;
				case ARG_1(PARAMETER_V0) | ARG_2(PARAMETER_ADDR) | ARG_3(PARAMETER_NONE):
					if(op == OPERATION_JP)
						instruction.id = INTERP_JP_V0_ADDR;
					break;
				case ARG_1(PARAMETER_ADDR) | ARG_2(PARAMETER_NONE) | ARG_3(PARAMETER_NONE):
					switch(op){
						case OPERATION_JP:
							instruction.id = INTERP_JP_ADDR;
							break;
						case OPERATION_CALL:
							instruction.id = INTERP_CALL_ADDR;
							break;
					}
					break;
			}

			if(instruction.id == INTERP_INSTRUCTION_UNKNOWN){
				setError("%d: illegal Arguments for %s op\n\t%s", 
						lineNumber, 
						operationSymbols[op], 
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
