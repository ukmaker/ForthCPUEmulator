#ifndef UKMAKER_FORTHVM_H
#define UKMAKER_FORTHVM_H

#include "Memory.h"
#include "FArduino.h"
#include "ForthIS.h"

class ForthVM; 

class ForthVM
{

public:
    ForthVM(Memory *ram)
    :
    _ram(ram)
    {
        _halted = true;
    }

    ~ForthVM() {}

    bool halted() {
        return _halted;
    }

    void reset() {
        _pc     = 0;
        _here   = 0;
        _intr0  = 0;
        _intr1  = 0;
        _halted = false;
        _interruptsEnabled = false;
    }

    void warm() {
        _pc = 0;
        _halted = false;
    }

    void step() {
        _clock();
        _ram->clk();
    }

    void run() {
        while(!halted()) step();
    }

    uint16_t get(uint8_t reg) {
        return _regs[reg];
    }

    void set(uint8_t reg, uint16_t v) {
        _regs[reg] = v;
    }

    void setPC(uint16_t a) {
        _pc = a;
    }

    uint16_t getPC() {
        return _pc;
    }

    bool getC() {
        return _c;
    }

    bool getZ() {
        return _z;
    }

    bool getO() {
        return _odd;
    }

    bool getS() {
        return _sign;
    }

    uint8_t readByte(uint16_t addr) {
        return _ram->getC(addr);
    }

    uint16_t read(uint16_t addr) {
        return _ram->get(addr);
    }

    Memory *ram() {
        return _ram;
    }

    void halt() {
        _halted = true;
    }

    protected:

    bool _halted;
    bool _interruptsEnabled = false;

    Memory *_ram;

    // Registers
    uint16_t _regs[16];
    uint16_t _pc;
    uint16_t _here;
    uint16_t _intr0;
    uint16_t _intr1;

    // flags
    bool _z;
    bool _c;
    bool _sign;
    bool _odd;

    uint8_t _arga(uint16_t instr) {
        return (instr & ARGA_BITS) >> ARGA_BITS_POS;
    }

    uint8_t _argb(uint16_t instr) { 
        return (instr & ARGB_BITS) >> ARGB_BITS_POS;
    }

    uint8_t _u4(uint16_t instr) {
        return _argb(instr);
    }

    int8_t _s4(uint16_t instr) {
        return _sex(_argb(instr));
    }

    uint8_t _u5(uint16_t instr) {
        return _argb(instr) + (((instr & LDS_U5_BIT) >> LDS_U5_BIT_POS) << 4);
    }

    int16_t _u8(uint16_t instr) {
        return (instr & 0x00ff);
    }

    int16_t _s8(uint16_t instr) {
        return _sex(instr & 0xff);
    }

    void _ldsx(uint16_t instr) {
        if(instr & LDS_LDX_BIT) {
            _ldx(instr);
        } else {
            _lds(instr);
        }
    }

    void _lds(uint16_t instr) {
        uint8_t op   = (instr & LDS_OP_BITS) >> LDS_OP_BITS_POS;
        uint8_t mode = (instr & LDS_MODE_BITS) >> LDS_MODE_BITS_POS;

        uint8_t rega;
        uint8_t regb;
        uint16_t arga;
        uint16_t argb;

        bool byteMode = (op == LDS_OP_LD_B || op == LDS_OP_ST_B);
        uint8_t delta = byteMode ? 1 : 2;

        uint8_t U5 = _u5(instr);

        switch(mode)
        {
            case LDS_MODE_REG_REG:
                rega = _arga(instr);
                regb = _argb(instr);
                arga = _regs[rega];
                argb = _regs[regb];
            break;
            
            case LDS_MODE_REG_HERE:
                rega = _arga(instr);
                regb = _argb(instr);
                arga = _regs[rega];
                argb = _here;
                _pc += 2;
            break;
            
            case LDS_MODE_REG_REG_INC:
                rega = _arga(instr);
                regb = _argb(instr);
                arga = _regs[rega];
                argb = _regs[regb];
                _regs[regb] = _regs[regb] + delta;
            break;
            
            case LDS_MODE_REG_REG_DEC:
                rega = _arga(instr);
                regb = _argb(instr);
                _regs[regb] = _regs[regb] - delta;
                arga = _regs[rega];
                argb = _regs[regb];
            break;
        }
        switch(op)
        {
            case LDS_OP_LD:
                _regs[rega] = _ram->get(argb);
            break;

            case LDS_OP_LD_B:
                _regs[rega] = _ram->getC(argb);
            break;

            case LDS_OP_ST:
                _ram->put(argb, arga);
            break;

            case LDS_OP_ST_B:
                _ram->putC(argb, arga);
            break;
        }
    }
    void _ldx(uint16_t instr) {
        uint8_t op   = (instr & LDS_OP_BITS) >> LDS_OP_BITS_POS;
        uint8_t mode = (instr & LDS_MODE_BITS) >> LDS_MODE_BITS_POS;

        uint8_t rega;
        uint8_t regb;
        uint16_t arga;
        uint16_t argb;

        bool byteMode = (op == LDS_OP_LD_B || op == LDS_OP_ST_B);
        uint8_t delta = byteMode ? 1 : 2;

        uint8_t U5 = _u5(instr);

        switch(mode)
        {          
            case LDX_MODE_REG_RL:
                rega = _arga(instr);
                regb = _argb(instr);
                arga = _regs[rega];
                argb = _regs[REG_RL] + U5;
             break;
            
            case LDX_MODE_REG_FP:
                rega = _arga(instr);
                regb = _argb(instr);
                arga = _regs[rega];
                argb = _regs[REG_FP] - U5;
            break;
            
            case LDX_MODE_REG_SP:
                rega = _arga(instr);
                regb = _argb(instr);
                arga = _regs[rega];
                argb = _regs[REG_SP] + U5;
            break;
            
            case LDX_MODE_REG_RS:
                rega = _arga(instr);
                regb = _argb(instr);
                arga = _regs[rega];
                argb = _regs[REG_RS] + U5;
            break;
            
        }
        switch(op)
        {
            case LDS_OP_LD:
                _regs[rega] = _ram->get(argb);
            break;

            case LDS_OP_LD_B:
                _regs[rega] = _ram->getC(argb);
            break;

            case LDS_OP_ST:
                _ram->put(argb, arga);
            break;

            case LDS_OP_ST_B:
                _ram->putC(argb, arga);
            break;
        }
    }

    void _alu(uint16_t instr) {
        uint8_t op   = (instr & ALU_OP_BITS) >> ALU_OP_BITS_POS;
        uint8_t mode = (instr & ALU_MODE_BITS) >> ALU_MODE_BITS_POS;

        uint8_t rega;
        uint8_t regb;
        uint16_t arga;
        uint16_t argb;

        switch(mode)
        {
            case ALU_MODE_REG_REG:
                rega = _arga(instr);
                regb = _argb(instr);
                arga = _regs[rega];
                argb = _regs[regb];
            break;
            
            case ALU_MODE_REG_U4:
                rega = _arga(instr);
                arga = _regs[rega];
                argb = _u4(instr);
            break;
            
            case ALU_MODE_REGA_U8:
                rega = REG_A;
                arga = _regs[rega];
                argb = _u8(instr);
           break;
            
            case ALU_MODE_REGA_S8:
                rega = REG_A;
                arga = _regs[rega];
                argb = _s8(instr);
            break;
            
        }

        switch(op)
        {
            case ALU_OP_MOV:
                _regs[rega] = argb;
            break;

            case ALU_OP_ADD:
                _add(rega, argb);
            break;

            case ALU_OP_SUB:
                _sub(rega, argb);
            break;

            case ALU_OP_MUL:
                _mul(rega, argb);
            break;

            case ALU_OP_OR:
                _or(rega, argb);
            break;

            case ALU_OP_AND:
                _and(rega, argb);
            break;

            case ALU_OP_XOR:
                _xor(rega, argb);
            break;

            case ALU_OP_SL:
                _sl(rega, argb);
            break;

            case ALU_OP_SR:
                _sr(rega, argb);
            break;

            case ALU_OP_SRA:
                _sra(rega, argb);
            break;

            case ALU_OP_ROT:
                _rot(rega, argb);
            break;

            case ALU_OP_BIT:
                _bit(rega, argb);
            break;

            case ALU_OP_SET:
                _set(rega, argb);
            break;

            case ALU_OP_CLR:
                _clr(rega, argb);
            break;

            case ALU_OP_CMP:
                _cmp(rega, argb);
            break;

            case ALU_OP_SEX:
                _sex(rega, argb);
            break;            
        }
              
    }

    void _jmp(uint16_t instr) {

        bool ccapply = false;
        bool ccinvert = false;
        bool skip = true;
        bool link = false;

        uint8_t cc   = (instr & JMP_CC_BITS) >> JMP_CC_BITS_POS;
        uint8_t op   = (instr & JMP_MODE_BITS) >> JMP_MODE_BITS_POS;
        uint8_t mode = (instr & JMP_MODE_BITS) >> JMP_MODE_BITS_POS;
        link         = (instr & JMP_LINK_BIT) != 0;

        switch((instr & JMP_SKIP_BITS) >> JMP_SKIP_BITS_POS)
        {
            case JMP_SKIP_CC:
                ccapply = true;
                ccinvert = true;
                break;
            case JMP_SKIP_NOT_CC:
                ccapply = true;
                break;
            default: break;
        }

        if(ccapply) {

            switch (cc)
            {
                case JMP_CC_C:
                    skip = !_c;
                    break;
                case JMP_CC_Z:
                    skip = !_z;
                    break;
                case JMP_CC_S:
                    skip = !_sign;
                    break;
                case JMP_CC_P:
                    skip = !_odd;
                    break;
                default:
                    break;
            }

            if(ccinvert) skip = !skip;
        }

        if(skip) {
            switch(mode)
            {
                case JMP_MODE_ABS_REG:
                    if(link) _regs[REG_RL] = _here;
                    _pc = _regs[_argb(instr)];
                    break;
                case JMP_MODE_IND_REG:
                    if(link) _regs[REG_RL] = _here;
                    _pc = _ram->get(_regs[_argb(instr)]);
                    break;
                case JMP_MODE_ABS_HERE:
                    if(link) _regs[REG_RL] = _here;
                    _pc = _ram->get(_pc);
                    break;
                case JMP_MODE_REL_HERE:
                    if(link) _regs[REG_RL] = _here;
                    _pc += _ram->get(_pc) - 2;
                    break;
            }
        } else {
            if(mode == JMP_MODE_ABS_HERE || mode == JMP_MODE_REL_HERE) {
                _pc +=2;
            }
        }
    }
    void _gen(uint16_t instr) {
        uint8_t op   = (instr & GEN_OP_BITS) >> GEN_OP_BITS_POS;
        switch(op)
        {
            case GEN_OP_NOP: break;
            case GEN_OP_HALT: _halted = true; break;
            case GEN_OP_EI: _interruptsEnabled = true; break;
            case GEN_OP_DI: _interruptsEnabled = false; break;
            default: break;
        }
    }

    // Run one processor cycle
    void _clock()
    {
        if(_halted) {
            return;
        }

        uint16_t instr = _ram->get(_pc);
        _pc += 2;
        _here = _pc;

        // Decode the opcode
        uint8_t group = (instr & GROUP_BITS) >> GROUP_BITS_POS;
        switch(group)
        {
            case GROUP_GEN: 
                _gen(instr);
                break;
            case GROUP_LDS:
                _ldsx(instr);
                break;
            case GROUP_ALU:
                _alu(instr);
                break;
            case GROUP_JMP:
                _jmp(instr);
                break;
        }    
    }

    void _add(uint8_t rega, uint16_t argb) {
        uint32_t r = (uint32_t)_regs[rega] + (uint32_t)argb;
        _regs[rega] = r & 0xffff;
        _arithmeticFlags(r);
    }

    void _sub(uint8_t rega, uint16_t argb) {
        uint32_t r = (uint32_t)_regs[rega] - (uint32_t)argb;
        _regs[rega] = r & 0xffff;
        _arithmeticFlags(r);
    }

    void _cmp(uint8_t rega, uint16_t argb) {
        uint32_t r = (uint32_t)_regs[rega] - (uint32_t)argb;
        _cmpFlags((uint32_t)_regs[rega], (uint32_t)argb, r);
    }

   void _mul(uint8_t rega, uint16_t argb) {
        uint32_t r = (uint32_t)_regs[rega] * (uint32_t)argb;
        _regs[rega] = r & 0xffff;
        _arithmeticFlags(r);
    }

    void _and(uint8_t rega, uint16_t argb) {
        uint16_t r = _regs[rega] & argb;
        _regs[rega] = r;
        _booleanFlags(r);
    }

    void _or(uint8_t rega, uint16_t argb) {
        uint16_t r = _regs[rega] | argb;
        _regs[rega] = r;
        _booleanFlags(r);
    }

    void _not(uint8_t rega, uint16_t argb) {
        uint16_t r = ~argb;
        _regs[rega] = r;
        _booleanFlags(r);
    }

    void _xor(uint8_t rega, uint16_t argb) {
        uint16_t r = _regs[rega] ^ argb;
        _regs[rega] = r;
        _booleanFlags(r);
    }

    void _sl(uint8_t rega, uint16_t argb) {
        uint32_t r = _regs[rega] << argb;
        _regs[rega] = r & 0xffff;
        _booleanFlags(r);
    }

    void _sr(uint8_t rega, uint16_t argb) {
        uint16_t r = _regs[rega] >> argb;
        bool c = _regs[rega] & 1;
        _regs[rega] = r;
        _booleanFlags(r);
        _c = c;
    }

    void _sra(uint8_t rega, uint16_t argb) {
        uint16_t r = _regs[rega] >> argb;
        bool c = _regs[rega] & 1;
        _regs[rega] = r;
        _sex(rega, 15 - argb);
        _booleanFlags(r);
        _c = c;
    }

    void _rot(uint8_t rega, uint16_t argb) {
        uint16_t mask = ~((0xffff >> argb) << argb);
        uint16_t bits = _regs[rega] & mask;
        _regs[rega] = (_regs[rega] >> argb) | (bits << (16-argb));
        _booleanFlags(_regs[rega]);
        _c = (_regs[rega] & 0x8000) != 0;
    }

    void _booleanFlags(uint32_t v) {
        _c = (v & (uint32_t)0x10000) == (uint32_t)0x10000;
        _z = v == 0;
        _odd = v & 0x01;
        _sign = (v & (uint32_t)0x08000) != 0;
    }

    void _arithmeticFlags(uint32_t v) {
        _c = (v & (uint32_t)0x10000) == (uint32_t)0x10000;
        _z = v == 0;
        // odd(P) functions as overflow
        _odd = (v & (uint32_t)0xffff0000) != 0;
        _sign = (v & (uint32_t)0x08000) != 0;
    }

    void _cmpFlags(uint32_t a,uint32_t b, uint32_t v) {
        // carry => carry as normal
        // z => a == b
        // p => a < b
        // Boolean equation for this is
        // Signs differ: SD = (a xor b ) & 0x8000
        // Result sign: SR = r & 0x8000
        // a < b : P = SR & (SD + ~Z)
        _c = (v & (uint32_t)0x10000) == (uint32_t)0x10000;
        _z = v == 0;
        // odd(P) functions as <
        bool SD = (a ^ b) != 0;
        bool SR = (v & 0x8000) != 0;
        _odd = (SR && (SD || !_z));
        _sign = (v & (uint32_t)0x08000) != 0;
    }

    /*
    * Sign-extend n and return a signed number
    * b is the sign bit index. Make the number n a signed 8-bit number
    */
    int8_t _sex(uint8_t n, uint8_t b) {
        
        if(n & (1 << b)) {
            uint8_t bits = (0xff >> b) << b;
            return n | bits;
        }

        return n;
    }

    uint16_t _sex(uint8_t n) {
        uint16_t r = n;
        if(n & 0x80) {
            r |= 0xff00;
        }
        return r;
    }

    void _bit(uint8_t rega, uint16_t argb) {
        _z = (_regs[rega] & (1 << argb)) == 0;
    }

    void _set(uint8_t rega, uint16_t argb) {
        _regs[rega] = _regs[rega] | (1 << argb);
    }

    void _clr(uint8_t rega, uint16_t argb) {
        _regs[rega] = _regs[rega] & ~(1 << argb);
    }
};
#endif