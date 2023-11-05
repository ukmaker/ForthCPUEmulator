#ifndef UKMAKER_DEBUGGER_H
#define UKMAKER_DEBUGGER_H

#include "../runtime/ForthCPU/ForthVM.h"
#include "Assembler.h"
#include "Dumper.h"

class Debugger
{

public:
    Debugger() {}
    ~Debugger() {}

    void setAssembler(Assembler *fasm)
    {
        _asm = fasm;
    }

    void setVM(ForthVM *vm)
    {
        _vm = vm;
    }

    void setDumper(Dumper *dumper)
    {
        _dumper = dumper;
    }

    void setShowForthWordsOnly()
    {
        _showWords = true;
    }

    void setBump(uint16_t b) 
    {
        _bump = b;
    }

    void setBreakpoint1(uint16_t addr)
    {
        _breakpoint1 = addr;
    }
    void setBreakpoint2(uint16_t addr)
    {
        _breakpoint2 = addr;
    }

    void setLabelBreakpoint1(const char *name)
    {
        int addr = _asm->getLabelAddress(name);
        if (addr != -1)
        {
            setBreakpoint1(addr);
        }
    }

    void setLabelBreakpoint2(const char *name)
    {
        int addr = _asm->getLabelAddress(name);
        if (addr != -1)
        {
            setBreakpoint2(addr);
        }
    }
    
    void printWALabel(uint16_t wa) {
        Token *label = _asm->getLabel(wa);
        if(label != NULL) {
            printf("%s\n", label->name);
        }
    }

    void setVerbose(bool v) {
        _verbose = v;
    }

    void breakpoint()
    {
        if (_vm->get(REG_I) == _breakpoint2)
        {
            printf("I *");
        }
        else if (_vm->getPC() == _breakpoint1)
        {
            printf("PC*");
        }
        else if(_verbose)
        {
            printf("  ");
        }

        if(_bump > 0 && ((_steps % _bump) == 0))
        {
            printf("  ");
        }
        _steps++;
    }

    void reset()
    {
        _vm->reset();
    }

    void step()
    {
        printCurrentStep();
        _vm->step();
    }

    void run()
    {
        while (!_vm->halted())
            step();
    }

    void run(uint16_t steps)
    {
        for (uint16_t i = 0; i < steps; i++)
        {
            if (!_vm->halted())
                step();
        }
    }

    void printCurrentStep()
    {
        uint16_t pc = _vm->getPC();
        uint16_t instr = _vm->ram()->get(pc);

        const char *label = NULL;
        Token *tok = _asm->getOpcode(pc);
        if (tok != NULL && tok->label != NULL)
        {
            label = tok->label->name;
        }

        if (label != NULL) {
            if(_verbose && _showWords) {
                //uint16_t indent = 0x2000 - _vm->get(REG_RS);
                //while(indent > 0) {
                    printf(" ");
                //    indent--;
               // }
            }
            if(_verbose) printf("%s\n", label);
        }

        breakpoint();

        //if(!_verbose || _showWords) return;
        printf("%04x - %04x D[%04x] R[%04x] [S%d O%d Z%d C%d] [0:%04x 1:%04x 2:%04x 3:%04x 4:%04x 5:%04x 6:%04x 7:%04x",
               pc,
               _vm->ram()->get(pc),
               _vm->ram()->get(_vm->get(REG_SP) + 2),
               _vm->ram()->get(_vm->get(REG_RS) + 2),
               _vm->getS() ? 1 : 0, _vm->getO() ? 1 : 0, _vm->getZ() ? 1 : 0, _vm->getC() ? 1 : 0,
               _vm->get(REG_0),
               _vm->get(REG_1),
               _vm->get(REG_2),
               _vm->get(REG_3),
               _vm->get(REG_4),
               _vm->get(REG_5),
               _vm->get(REG_6),
               _vm->get(REG_7));

        // PC [ S O Z C ] CC OP ARGS
        printf(" A:%04x B:%04x PC:%04x WA:%04x SP:%04x RS:%04x FP:%04x I:%04x]",
               _vm->get(REG_A),
               _vm->get(REG_B),
               _vm->getPC(),
               _vm->get(REG_WA),
               _vm->get(REG_SP),
               _vm->get(REG_RS),
               _vm->get(REG_FP),
               _vm->get(REG_I));

        _dumper->printOpcode(_asm, tok);

        printf("\n");
    }

protected:
    Assembler *_asm;
    ForthVM *_vm;
    Dumper *_dumper;
    uint16_t _breakpoint1, _breakpoint2;
    bool _showWords = false;
    uint16_t _bump = 0;
    uint16_t _steps = 0;
    bool _verbose = false;

};
#endif