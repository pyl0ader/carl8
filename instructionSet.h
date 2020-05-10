#define TOKEN_SIZE 5
#define PARAMETERMAX 14

enum parameter {
    NONE,
    NNN,
    KK,
    X,
    Y,
    V0,
    DT,
    ST,
    N,
    I,
    F,
    K,
    B,
    MEMINDEX
};

enum operation {
    CLS,
    RET,
    SYS,
    JP,
    CALL,
    SE,
    SNE,
    LD,
    ADD,
    OR,
    AND,
    XOR,
    SUB,
    SHR,
    SUBN,
    SHL,
    RND,
    DRW,
    SKP,
    SKNP,
    UNKNOWN
};

typedef struct instruction {
    uint16_t opcode;
    enum operation opSymbol;
    enum parameter parameters[3];
    uint8_t isBranch;
    uint8_t isJump;
} Instruction;

static const Instruction instructionSet[] = {
/*  opcode   opsymbol parameters     isBranch isJump */
    {0x00E0, CLS,     {NONE},        0,       0},
    {0x00EE, RET,     {NONE},        0,       0},
    {0x0000, SYS,     {NONE},        0,       0},
    {0x1000, JP,      {NNN},         0,       1},
    {0x2000, CALL,    {NNN},         0,       0},
    {0x3000, SE,      {X, KK},       1,       0},
    {0x4000, SNE,     {X, KK},       1,       0},
    {0x5000, SE,      {X, Y},        1,       0},
    {0x6000, LD,      {X, KK},       0,       0},
    {0x7000, ADD,     {X, KK},       0,       0},
    {0x8000, LD,      {X, Y},        0,       0},
    {0x8001, OR,      {X, Y},        0,       0},
    {0x8002, AND,     {X, Y},        0,       0},
    {0x8003, XOR,     {X, Y},        0,       0},
    {0x8004, ADD,     {X, Y},        0,       0},
    {0x8005, SUB,     {X, Y},        0,       0},
    {0x8006, SHR,     {X, Y},        0,       0},
    {0x8007, SUBN,    {X, Y},        0,       0},
    {0x800E, SHL,     {X, Y},        0,       0},
    {0x9000, SNE,     {X, Y},        1,       0},
    {0xA000, LD,      {I, NNN},      0,       0},
    {0xB000, JP,      {V0, NNN},     0,       1},
    {0xC000, RND,     {X, KK},       0,       0},
    {0xD000, DRW,     {X, Y, N},     0,       0},
    {0xE09E, SKP,     {X},           1,       0},
    {0xE0A1, SKNP,    {X},           1,       0},
    {0xF007, LD,      {X, DT},       0,       0},
    {0xF00A, LD,      {X, K},        0,       0},
    {0xF015, LD,      {DT, X},       0,       0},
    {0xF018, LD,      {ST, X},       0,       0},
    {0xF01E, ADD,     {I, X},        0,       0},
    {0xF029, LD,      {F, X},        0,       0},
    {0xF033, LD,      {B, X},        0,       0},
    {0xF055, LD,      {MEMINDEX, X}, 0,       0},
    {0xF065, LD,      {X, MEMINDEX}, 0,       0}
};

const char operationSymbols[][TOKEN_SIZE] = {
    "CLS",
    "RET",
    "SYS",
    "JP",
    "CALL",
    "SE",
    "SNE",
    "LD",
    "ADD",
    "OR",
    "AND",
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

const char paramterSymbols[][TOKEN_SIZE] = {
    "",     //NONE    
    "L",    //NNN
    "#",    //KK
    "V",    //X
    "V",    //Y
    "V0",   //V0
    "DT",   //DT
    "ST",   //ST
    "#",    //N
    "I",    //I
    "F",    //F
    "K",    //K
    "B",    //B
    "[I]"   //MEMINDEX
};
