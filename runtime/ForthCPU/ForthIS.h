#ifndef UKMAKER_FORTH_IS_H
#define UKMAKER_FORTH_IS_H

// General purpose registers
#define REG_0 0
#define REG_1 1
#define REG_2 2
#define REG_3 3
#define REG_4 4
#define REG_5 5
#define REG_6 6
#define REG_7 7

// Special purpose registers
// quick registers
#define REG_A 8
#define REG_B 9
// Forth instruction pointer
#define REG_I 10
// Forth return stack frame pointer
#define REG_FP 11
// Forth word address pointer
#define REG_WA 12
// Data stack pointer
#define REG_SP 13
// Return stack pointer
#define REG_RS 14
// Link register
#define REG_RL 15

// Opcodes
#define GROUP_BITS_POS     14
#define GROUP_BITS     0xc000
// Jumps
#define JMP_SKIP_BITS_POS      12
#define JMP_SKIP_BITS      0x3000
#define JMP_CC_BITS_POS        10
#define JMP_CC_BITS        0x0c00
#define JMP_MODE_BITS_POS       8
#define JMP_MODE_BITS      0x0300
#define JMP_LINK_BIT_POS        7
#define JMP_LINK_BIT       0x0080
// Load/Store
#define LDS_U5_BIT_POS         13
#define LDS_U5_BIT         0x2000
#define LDS_OP_BITS_POS        11
#define LDS_OP_BITS        0x1800
#define LDS_MODE_BITS_POS       8
#define LDS_MODE_BITS      0x0700
// ALU 
#define ALU_OP_BITS_POS        10
#define ALU_OP_BITS        0x3c00
#define ALU_MODE_BITS_POS       8
#define ALU_MODE_BITS      0x0300
// General group
#define GEN_OP_BITS_POS        10
#define GEN_OP_BITS        0x1c00

#define ARGA_BITS_POS 4
#define ARGB_BITS_POS 0
#define ARGA_BITS (0x0f << ARGA_BITS_POS)
#define ARGB_BITS (0x0f << ARGB_BITS_POS)

// Define the instruction groups
#define GROUP_GEN 0
#define GROUP_LDS 1
#define GROUP_JMP 2
#define GROUP_ALU 3

// General opcodes
#define GEN_OP_NOP   0
#define GEN_OP_HALT  1
#define GEN_OP_EI    2
#define GEN_OP_DI    3
#define GEN_OP_RETI  4

// Load/Store opcodes
#define LDS_OP_LD    0
#define LDS_OP_LD_B  1
#define LDS_OP_ST    2
#define LDS_OP_ST_B  3

#define LDS_MODE_REG_REG     0
#define LDS_MODE_REG_HERE    1
#define LDS_MODE_REG_REG_INC 2
#define LDS_MODE_REG_REG_DEC 3
#define LDS_MODE_REG_RL      4
#define LDS_MODE_REG_FP      5
#define LDS_MODE_REG_SP      6
#define LDS_MODE_REG_RS      7

// Jump opcodes
#define JMP_MODE_ABS_REG     0 
#define JMP_MODE_IND_REG     1
#define JMP_MODE_ABS_HERE    2 
#define JMP_MODE_REL_HERE    3 

#define JMP_SKIP_ALWAYS_A 0 
#define JMP_SKIP_ALWAYS_B 1
#define JMP_SKIP_CC       2
#define JMP_SKIP_NOT_CC   3

#define JMP_CC_Z 0
#define JMP_CC_C 1
#define JMP_CC_S 2
#define JMP_CC_P 3

#define JMP_LINK_NONE 0
#define JMP_LINK_LINK 1

// ALU opcodes
#define ALU_OP_MOV  0
#define ALU_OP_ADD  1
#define ALU_OP_SUB  2
#define ALU_OP_MUL  3
#define ALU_OP_OR   4
#define ALU_OP_AND  5
#define ALU_OP_XOR  6
#define ALU_OP_SL   7
#define ALU_OP_SR   8
#define ALU_OP_SRA  9
#define ALU_OP_ROT 10
#define ALU_OP_BIT 11
#define ALU_OP_SET 12
#define ALU_OP_CLR 13
#define ALU_OP_CMP 14
#define ALU_OP_SEX 15

#define ALU_MODE_REG_REG   0
#define ALU_MODE_REG_U4    1
#define ALU_MODE_REGA_U8   2
#define ALU_MODE_REGA_S8   3


#endif