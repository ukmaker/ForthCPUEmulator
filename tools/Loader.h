#ifndef UKMAKER_LOADER_H
#define UKMAKER_LOADER_H

#include "../runtime/ForthCPU/ForthVM.h"

class Loader {

    public:

    Loader(Memory *ram) : _ram(ram), _pc(0) {}
    ~Loader() {}

    void reset() {
        _pc = 0;
    }

    void loadGen(uint16_t genOp) {
        uint16_t instruction = 
            (GROUP_GEN << GROUP_BITS_POS)
            + (genOp << GEN_OP_BITS_POS);
        load(instruction);       
    }

    // Ra,(Rb) ; Ra,(Rb++), Ra,(--Rb)
    // RA,(RB+U5) ; Ra,(FP-U5) ; Ra,(SP+U5) ; Ra,(RS+U5)
    void loadLDS(uint16_t ldsOp, uint16_t ldsMode, uint16_t arga, uint16_t argb) {
        uint16_t instruction = (GROUP_LDS << GROUP_BITS_POS)
            + (ldsOp << LDS_OP_BITS_POS)
            + (ldsMode << LDS_MODE_BITS_POS)
            + (arga << ARGA_BITS_POS)
            + (argb & 0x0f);
        switch(ldsMode) {
            case LDS_MODE_REG_HERE:
                load(instruction);  
                instruction = argb;
                break;              
            case LDS_MODE_REG_RL:
            case LDS_MODE_REG_FP:
            case LDS_MODE_REG_SP:
            case LDS_MODE_REG_RS:
                instruction += (argb & 0x10) << LDS_U5_BIT_POS;
                break;
            default: break;
        }
        load(instruction);
    }

    void loadJMP(uint16_t skip, uint16_t cc, uint16_t jmpOp, bool link) {
        uint16_t instruction =
            (GROUP_JMP << GROUP_BITS_POS)
            + (skip << JMP_SKIP_BITS_POS)
            + (cc << JMP_CC_BITS_POS)
            + (jmpOp << JMP_MODE_BITS_POS)
            + (link ? JMP_LINK_BIT : 0);
        load(instruction);
    }

    void loadALU(uint16_t aluOp, uint16_t aluMode, uint16_t arga, uint16_t argb) {
        uint16_t instruction = 
           (GROUP_ALU << GROUP_BITS_POS)
            + (aluOp << ALU_OP_BITS_POS)
            + (aluMode << ALU_MODE_BITS_POS)
            + ((arga & 0x0f) << ARGA_BITS_POS)
            + ((argb & 0x0f) << ARGB_BITS_POS);
        load(instruction); 
    }

   void loadALU(uint16_t aluOp, uint16_t aluMode, int16_t value) {
        uint16_t instruction = 
           (GROUP_ALU << GROUP_BITS_POS)
            + (aluOp << ALU_OP_BITS_POS)
            + (aluMode << ALU_MODE_BITS_POS)
            + value;
        load(instruction); 
    }

    void load(uint16_t value) {
         _ram->put(_pc, value);
        _pc += 2;       
    }

    void load(char c, bool inc) {
        uint16_t v = _ram->get(_pc);
        // if inc then this is the high byte and we increment to the next word
        if(inc) {
            v = v & 0xff;
            v = v | ((uint16_t)c << 8);
            load(v);
        } else {
            v = v & 0xff00;
            v = v | c;
            _ram->put(_pc, v);
        }
    }

    uint16_t load(const char *str) {
        char c;
        bool h = false;
        while((c = *str++) != 0) {
            load(c, h);
            h = !h;
        }
        // make sure we're aligned on a word boundary
        if(h) load('\0', h);
        return _pc;
    }

    protected:

    Memory *_ram;
    uint32_t _pc;

};

#endif