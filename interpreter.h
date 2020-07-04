#include <stdint.h>

#define INTERP_INSTRUCTIONSET_LEN 35
#define INTERP_MEMORY_LEN 0x1000
#define INTERP_PROGRAM_START 0x200
#define INTERP_SPRITE_DATA_MAX (INTERP_MEMORY_LEN - INTERP_PROGRAM_START - 1)

/* function declarations */
extern int interp_initialize(void);
extern int interp_loadRom(const char* rom);
extern int interp_step(void);

enum interp_instructionId {
    INTERP_CLS,
    INTERP_RET,
    INTERP_SYS_ADDR,
    INTERP_JP_ADDR,
    INTERP_CALL_ADDR,
    INTERP_SE_VX_BYTE,
    INTERP_SNE_VX_BYTE,
    INTERP_SE_VX_VY,
    INTERP_LD_VX_BYTE,
    INTERP_ADD_VX_BYTE,
    INTERP_LD_VX_VY,
    INTERP_OR_VX_VY,
    INTERP_AND_VX_VY,
    INTERP_XOR_VX_VY,
    INTERP_ADD_VX_VY,
    INTERP_SUB_VX_VY,
    INTERP_SHR_VX_VY,
    INTERP_SUBN_VX_VY,
    INTERP_SHL_VX_VY,
    INTERP_SNE_VX_VY,
    INTERP_LD_I_ADDR,
    INTERP_JP_V0_ADDR,
    INTERP_RND_VX_BYTE,
    INTERP_DRW_VX_VY_NIBBLE,
    INTERP_SKP_VX,
    INTERP_SKNP_VX,
    INTERP_LD_VX_DT,
    INTERP_LD_VX_K,
    INTERP_LD_DT_VX,
    INTERP_LD_ST_VX,
    INTERP_ADD_I_VX,
    INTERP_LD_F_VX,
    INTERP_LD_B_VX, 
    INTERP_LD_MEMINDEX_VX,
    INTERP_LD_VX_MEMINDEX,
    INTERP_INSTRUCTION_UNKNOWN
};

typedef struct interp_instruction {
    enum interp_instructionId id;
    uint16_t nnn;
    uint8_t kk;
    uint8_t x;
    uint8_t y;
    uint8_t n;
} interp_instruction;

extern int interp_decode(uint16_t addr, interp_instruction *ins);
extern int interp_encode(uint16_t addr, interp_instruction ins);

/* _byte_ is written to memory at _addr_ */
extern int interp_writeMemory(uint16_t addr, uint8_t byte);

/* A byte is read from memory at _addr_, and is returned */
extern int interp_readMemory(uint16_t addr);

/* The size of the program is returned */
extern int interp_getProgramSize(void);
