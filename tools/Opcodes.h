#ifndef UKMAKER_FORTHCPU_OPCODES_H
#define UKMAKER_FORTHCPU_OPCODES_H
#include <stdint.h>
#include <string.h>
#include "../runtime/ForthCPU/ForthIS.h"
/****************************************************
 * Define all the opcode names used by the assembler
****************************************************/
class Opcode {

    public:

    Opcode(const char *name, uint16_t code) :
        _name(name), _code(code), _isLdx(false), _isJmp(false), _isImmediate(false) {}

    Opcode(const char *name, uint16_t code, bool isLdx) :
        _name(name), _code(code), _isLdx(isLdx), _isJmp(false), _isImmediate(false) {}

    Opcode(const char *name, uint16_t code, bool isLdx, bool isJmp) :
        _name(name), _code(code), _isLdx(isLdx), _isJmp(isJmp), _isImmediate(false) {}

    Opcode(const char *name, uint16_t code, bool isLdx, bool isJmp, bool isImmediate) :
        _name(name), _code(code), _isLdx(isLdx), _isJmp(isJmp), _isImmediate(isImmediate) {}

    static Opcode *aluCode(const char *name, uint16_t op, uint16_t mode) {
        uint16_t opcode = 
            (GROUP_ALU << GROUP_BITS_POS)
        +   (op << ALU_OP_BITS_POS)
        +   (mode << ALU_MODE_BITS_POS);

        return new Opcode(name, opcode);
    }

    static Opcode *genCode(const char *name, uint16_t op) {
        uint16_t opcode = 
            (GROUP_GEN << GROUP_BITS_POS)
        +   (op << GEN_OP_BITS_POS);

        return new Opcode(name, opcode);
    }

    static Opcode *ldsCode(const char *name, uint16_t op, uint16_t mode, bool imm = false) {
        uint16_t opcode = 
            (GROUP_LDS << GROUP_BITS_POS)
        +   (op << LDS_OP_BITS_POS)
        +   (mode << LDS_MODE_BITS_POS);

        return new Opcode(name, opcode, false, false, imm);
    }

    static Opcode *ldxCode(const char *name, uint16_t op) {
        uint16_t opcode = 
            (GROUP_LDS << GROUP_BITS_POS)
        +   (op << LDS_OP_BITS_POS);

        return new Opcode(name, opcode, true);
    }

    static Opcode *jmpCode(const char *name, uint16_t mode, uint16_t link, bool imm = false) {
        uint16_t opcode = 
            (GROUP_JMP << GROUP_BITS_POS)
        +   (mode << JMP_MODE_BITS_POS)
        +   (link << JMP_LINK_BIT_POS);

        return new Opcode(name, opcode, false, true, imm);
    }

    uint16_t getCode() { 
        return _code; 
    }

    uint8_t getGroup() {
        return (_code & GROUP_BITS) >> GROUP_BITS_POS;
    }

    const char *getName() {
        return _name;
    }

    bool isNamed(char *source, int pos, int len) {
        int equal = false;

        if((int)strlen(_name) == len) {
            int equal = true;
            for(int j=0; j<len; j++) {
                if(source[pos + j] != _name[j]) {
                    equal = false;
                }
            }
        }

        return equal;
    }

    bool isGen() {
        return getGroup() == GROUP_GEN;
    }

    bool isALU() {
        return getGroup() == GROUP_ALU;
    }

    bool isLDS() {
        return getGroup() == GROUP_LDS;
    }

    bool isJMP() {
        return getGroup() == GROUP_JMP;
    }

    bool isLdx() {
        return _isLdx;
    }

    void setLdxModeAndOffset(uint16_t mode, uint16_t u5) {
        _code |= (mode << LDS_MODE_BITS_POS);
        _code |= (u5 & 0x000f);
        _code |= (u5 & 0x0010) << (LDS_U5_BIT_POS - 4);
    }

    bool isJmp() {
        return _isJmp;
    }

    void setJmpSkipAndCC(uint16_t skip, uint16_t cc) {
        _code |= (skip << JMP_SKIP_BITS_POS);
        _code |= (cc << JMP_CC_BITS_POS);
    }

    bool isImmediate() {
        return _isImmediate;
    }

    uint8_t getALUMode() {
        return (_code & ALU_MODE_BITS) >> ALU_MODE_BITS_POS;
    }

    uint8_t getLDSMode() {
        return (_code & LDS_MODE_BITS) >> LDS_MODE_BITS_POS;
    }

    uint8_t getJMPMode() {
        return (_code & JMP_MODE_BITS) >> JMP_MODE_BITS_POS;
    }

    uint8_t getALUOp() {
        return (_code & ALU_OP_BITS) >> ALU_OP_BITS_POS;
    }

    uint8_t getLDSOp() {
        return (_code & LDS_OP_BITS) >> LDS_OP_BITS_POS;
    }

    uint8_t getJMPOp() {
        return (_code & JMP_OP_BITS) >> JMP_OP_BITS_POS;
    }

    bool expectsRa() {
        return 
            (isALU() && getALUMode() == ALU_MODE_REG_REG)
        |   (isALU() && getALUMode() == ALU_MODE_REG_U4)
        |   isLDS();
    }

    bool expectsRb() {
        return 
            (isALU() && getALUMode() == ALU_MODE_REG_REG)
        |   (isLDS() && getLDSMode() == LDS_MODE_REG_REG_INC)
        |   (isLDS() && getLDSMode() == LDS_MODE_REG_REG_DEC)
        |   (isLDS() && getLDSMode() == LDS_MODE_REG_RL)
        |   (isLDS() && getLDSMode() == LDS_MODE_REG_FP)
        |   (isLDS() && getLDSMode() == LDS_MODE_REG_SP)
        |   (isLDS() && getLDSMode() == LDS_MODE_REG_RS)
        |   (isJMP() && getJMPMode() == JMP_MODE_ABS_REG)
        |   (isJMP() && getJMPMode() == JMP_MODE_IND_REG);
    }

    bool expectsU16() {
        return
            (isLDS() && getLDSMode() == LDS_MODE_REG_HERE)
        |   (isJMP() && getJMPMode() == JMP_MODE_ABS_HERE)        
        |   (isJMP() && getJMPMode() == JMP_MODE_REL_HERE);  
    }


    bool expectsU4() {
        return
            (isALU() && getALUMode() == ALU_MODE_REG_U4);       
    }

    bool expectsU5() {
        return
            (isLDS() && getJMPMode() == LDS_MODE_REG_RL)
        |   (isLDS() && getJMPMode() == LDS_MODE_REG_FP)    
        |   (isLDS() && getJMPMode() == LDS_MODE_REG_SP)    
        |   (isLDS() && getJMPMode() == LDS_MODE_REG_RS);    
    }

   bool expectsU8() {
        return
            (isALU() && getALUMode() == ALU_MODE_REGA_U8);
    }

   bool expectsS8() {
        return  
           (isALU() && getALUMode() == ALU_MODE_REGA_S8); 
    }

    void setArgA(uint8_t a) {
        _code &= 0xff0f;
        _code |= ((a & 0x0f) << 4);
    }

    void setArgB(uint8_t b) {
        _code &= 0xfff0;
        _code |= (b & 0x0f);
    }

    void setU5(uint8_t u) {
        _code &= ~(1 << LDS_U5_BIT_POS);
        _code |= ((u & 0x10) << (LDS_U5_BIT_POS - 4));
        setArgB(u);
    }


    protected:

    const char *_name;
    uint16_t _code;
    bool _isLdx;
    bool _isJmp;
    bool _isImmediate;

};

class Opcodes {
    public: 

        Opcodes() {
            _aluCodes("MOV", "MOVI", "MOVAI", "MOVAS", ALU_OP_MOV);
            _aluCodes("ADD", "ADDI", "ADDAI", "ADDAS", ALU_OP_ADD);
            _aluCodes("SUB", "SUBI", "SUBAI", "SUBAS", ALU_OP_SUB);
            _aluCodes("MUL", "MULI", "MULAI", "MULAS", ALU_OP_MUL);
            _aluCodes("OR",  "ORI",  "ORAI",  "ORAS",  ALU_OP_OR);
            _aluCodes("AND", "ANDI", "ANDAI", "ANDAS", ALU_OP_AND);
            _aluCodes("XOR", "XORI", "XORAI", "XORAS", ALU_OP_XOR);
            _aluCodes("SL",  "SLI",  "SLAI",  "SLAS",  ALU_OP_SL);
            _aluCodes("SR",  "SRI",  "SRAI",  "SRAS",  ALU_OP_SR);
            _aluCodes("SRA", "SRAI", "SRAAI", "SRAAS", ALU_OP_SRA);
            _aluCodes("ROT", "ROTI", "ROTAI", "ROTAS", ALU_OP_ROT);
            _aluCodes("BIT", "BITI", "BITAI", "BITAS", ALU_OP_BIT);
            _aluCodes("SET", "SETI", "SETAI", "SETAS", ALU_OP_SET);
            _aluCodes("CLR", "CLRI", "CLRAI", "CLRAS", ALU_OP_CLR);
            _aluCodes("CMP", "CMPI", "CMPAI", "CMPAS", ALU_OP_CMP);
            _aluCodes("SEX", "SEXI", "SEXAI", "SEXAS", ALU_OP_SEX);

            _genCode("NOP",  GEN_OP_NOP);
            _genCode("HALT", GEN_OP_HALT);
            _genCode("DI",   GEN_OP_DI);
            _genCode("EI",   GEN_OP_EI);
            _genCode("RETI", GEN_OP_RETI);

            _ldsCodes("LD",   "LDI",   "POP",    "POPR",    LDS_OP_LD);
            _ldsCodes("LD_B", "LDI_B", "POP_B",  "POPR_B",  LDS_OP_LD_B);
            _ldsCodes("ST",   "STI",   "PUSH",   "PUSHR",   LDS_OP_ST);
            _ldsCodes("ST_B", "STI_B", "PUSH_B", "PUSHR_B", LDS_OP_ST_B);

            _ldxCodes();

            _jmpCodes();


        }

        Opcode *find(char *source, int pos, int len) {
            for(uint16_t i=0; i< idx; i++) {
                if(opcodes[i]->isNamed(source, pos, len)) {
                    return opcodes[i];
                }
            }

            return NULL;
        }

    protected:

    void _aluCodes(const char *a, const char *b, const char *c, const char *d, uint16_t op) {
        opcodes[idx++] = Opcode::aluCode(a, op, ALU_MODE_REG_REG);
        opcodes[idx++] = Opcode::aluCode(b, op, ALU_MODE_REG_U4);
        opcodes[idx++] = Opcode::aluCode(c, op, ALU_MODE_REGA_U8);
        opcodes[idx++] = Opcode::aluCode(d, op, ALU_MODE_REGA_S8);
    }

    void _genCode(const char *name, uint16_t op) {
        opcodes[idx++] = Opcode::genCode(name, op);
    }

    void _ldsCodes(const char *a, const char *b, const char *c, const char *d, uint16_t op) {
        opcodes[idx++] = Opcode::ldsCode(a, op, LDS_MODE_REG_REG);
        opcodes[idx++] = Opcode::ldsCode(b, op, LDS_MODE_REG_HERE, true);
        opcodes[idx++] = Opcode::ldsCode(c, op, LDS_MODE_REG_REG_INC);
        opcodes[idx++] = Opcode::ldsCode(d, op, LDS_MODE_REG_REG_DEC);
    }

    void _ldxCodes() {
        opcodes[idx++] = Opcode::ldxCode("LDX",   LDS_OP_LD);
        opcodes[idx++] = Opcode::ldxCode("LDX_B", LDS_OP_LD_B);
        opcodes[idx++] = Opcode::ldxCode("STX",   LDS_OP_ST);
        opcodes[idx++] = Opcode::ldxCode("STX_B", LDS_OP_ST_B);
    }

    void _jmpCodes() {
        opcodes[idx++] = Opcode::jmpCode("JP",  JMP_MODE_ABS_REG,  JMP_LINK_NONE);
        opcodes[idx++] = Opcode::jmpCode("JPM", JMP_MODE_IND_REG,  JMP_LINK_NONE);
        opcodes[idx++] = Opcode::jmpCode("JI",  JMP_MODE_ABS_HERE, JMP_LINK_NONE, true);
        opcodes[idx++] = Opcode::jmpCode("JR",  JMP_MODE_REL_HERE, JMP_LINK_NONE);

        opcodes[idx++] = Opcode::jmpCode("JPL",  JMP_MODE_ABS_REG,  JMP_LINK_LINK);
        opcodes[idx++] = Opcode::jmpCode("JPML", JMP_MODE_IND_REG,  JMP_LINK_LINK);
        opcodes[idx++] = Opcode::jmpCode("JIL",  JMP_MODE_ABS_HERE, JMP_LINK_LINK, true);
        opcodes[idx++] = Opcode::jmpCode("JRL",  JMP_MODE_REL_HERE, JMP_LINK_LINK);
    }


    Opcode *opcodes[256];
    uint8_t idx = 0;
};



#endif