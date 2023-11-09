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

    void setBreakpointI(uint16_t addr)
    {
        _breakpointI = addr;
    }
    void setBreakpointPC(uint16_t addr)
    {
        _breakpointPC = addr;
    }

    void setLabelBreakpointI(const char *name)
    {
        int addr = _asm->getLabelAddress(name);
        if (addr != -1)
        {
            setBreakpointI(addr);
            printf("Set breakpoint at address %04x\n", addr);
        } else {
            printf("Label not found\n");
        }
    }

    void setLabelBreakpointPC(const char *name)
    {
        int addr = _asm->getLabelAddress(name);
        if (addr != -1)
        {
            setBreakpointPC(addr);
            printf("Set breakpoint at address %04x\n", addr);
        } else {
            printf("Label not found\n");
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
        if (_vm->get(REG_I) == _breakpointI)
        {
            printf("I *");
            _broken = true;
        }
        else if (_vm->getPC() == _breakpointPC)
        {
            printf("PC*");
            _broken = true;
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

    void restart() {
        _broken = false;
        reset();
        run();
    }

    void restep() {
        reset();
        step();
    }

    void reset()
    {
        _vm->reset();
    }

    void step()
    {
        _broken = false;

        char c;
        do {
            printCurrentStep();
            _vm->step();
            c = getCommand();
        } while(c == '\n');
    }

    void cont() {
        _broken = false;
        run();
    }

    void run()
    {
        while (!_vm->halted() && !broken()) {
            printCurrentStep();
           _vm->step();
        }
    }

    void run(uint16_t steps)
    {
        for (uint16_t i = 0; i < steps; i++)
        {
            if (!_vm->halted()) {
                printCurrentStep();
                _vm->step();
            }
        }
    }

    bool broken() {
        return _broken;
    }

    void printCurrentStep()
    {
        if(_verbose) {
            uint16_t pc = _vm->getPC();
            uint16_t instr = _vm->ram()->get(pc);

            const char *label = NULL;
            Token *tok = _asm->getOpcode(pc);
            if (tok != NULL && tok->label != NULL)
            {
                label = tok->label->name;
            }

            if (label != NULL) {
                printf("%s\n", label);
            }

            breakpoint();

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
            printf(" A:%04x B:%04x PC:%04x WA:%04x SP:%04x RS:%04x FP:%04x I:%04x RL:%04x]",
                _vm->get(REG_A),
                _vm->get(REG_B),
                _vm->getPC(),
                _vm->get(REG_WA),
                _vm->get(REG_SP),
                _vm->get(REG_RS),
                _vm->get(REG_FP),
                _vm->get(REG_I),
                _vm->get(REG_RL));

            _dumper->printOpcode(_asm, tok);

            printf("\n");
        }
    }

    char getCommand() {
        int c = getchar();
        return (char)c;
    }

    void prompt() {
        Serial.print("DEBUG >>"); 
    }

    bool commandLine() {
        char c;
        while((char)(c = getCommand()) == '\n') {
            prompt();
        }
        switch(c) {
            case 'r': restart(); break;
            case 'S': restep(); break;
            case 's': step(); break;
            case 'c': cont(); break;
            case 'P': setPCBreakpoint(); break; 
            case 'I': setIBreakpoint(); break; 
            case 'V': setVerbose(true); break;
            case 'Q': setVerbose(false); break;
            case 'L': listSymbols(); break;
            case 'X': clearBreakpoints(); break;

            case 'e': return false;

            case '?': 
            Serial.println("Commands:");
            Serial.println("r - restart and run");
            Serial.println("S - restart and step");
            Serial.println("s - step");
            Serial.println("c - continue run from breakpoint");
            Serial.println("P - set ProgramCounter breakpoint");
            Serial.println("I - set I register breakpoint");
            Serial.println("V - verbose output");
            Serial.println("Q - be quiet");
            Serial.println("L - list symbols");
            Serial.println("X - clear breakpoints");

            Serial.println("e - Exit");
            break;
            default: break;
        }
        return true;
    }

    void setIBreakpoint() {
        getText(_breakPointIName, 64);
        setLabelBreakpointI(_breakPointIName);    
    }

    void setPCBreakpoint() {
        getText(_breakPointPCName, 64);
        setLabelBreakpointPC(_breakPointPCName);
    }

    void clearBreakpoints() {
        _breakpointI = -1;
        _breakpointPC = -1;
    }

    size_t getText(char *buf, uint16_t len) {
        size_t read = Serial.readBytesUntil(0x0a, buf, len-1);
        buf[read] = 0;
        return read-1;
    }

    void listSymbols()
    {
        Token *tok = _asm->tokens;
        Symbol *sym = _asm->symbols;

        printf("==============================\n");
        printf("Unresolved Symbols\n");
        printf("==============================\n");
        while (sym != NULL)
        {
            if (!sym->resolved())
                printf("%s\n", sym->name);
            sym = sym->next;
        }
        sym = _asm->symbols;
        printf("==============================\n");
        printf("Constants \n");
        printf("==============================\n");
        while (sym != NULL)
        {
            if (sym->resolved() && sym->token->type == TOKEN_TYPE_CONST)
            {
                printf("%s = %d\n", sym->name, sym->token->value);
            }
            sym = sym->next;
        }
        sym = _asm->symbols;
        printf("==============================\n");
        printf("Variables \n");
        printf("==============================\n");
        while (sym != NULL)
        {
            if (sym->resolved() && sym->token->type == TOKEN_TYPE_VAR)
            {
                printf("%s = %d\n", sym->name, sym->token->value);
            }
            sym = sym->next;
        }
        sym = _asm->symbols;
        printf("==============================\n");
        printf("Strings \n");
        printf("==============================\n");
        while (sym != NULL)
        {
            if (sym->resolved() && sym->token->type == TOKEN_TYPE_STR)
            {
                printf("%s Length %d - %s\n", sym->name, sym->token->value, sym->token->str);
            }
            sym = sym->next;
        }
        tok = _asm->firstLabel();
        printf("==============================\n");
        printf("Labels \n");
        printf("==============================\n");
        while (tok != NULL)
        {
            if (tok->address != -1)
            {
                printf("%04x %s: Line %d\n", tok->address, tok->name, tok->line);
            }
            tok = _asm->nextLabel();
        }
        tok = _asm->firstLabel();
        printf("==============================\n");
        printf("Unresolved Labels \n");
        printf("==============================\n");
        while (tok != NULL)
        {
            if (tok->address == -1)
            {
                printf("%s: Line %d\n", tok->name, tok->line);
            }
            tok = _asm->nextLabel();
        }
    }

protected:
    Assembler *_asm;
    ForthVM *_vm;
    Dumper *_dumper;
    int _breakpointI = -1;
    int _breakpointPC = -1;

    char _breakPointIName[64];
    char _breakPointPCName[64];

    bool _showWords = false;
    uint16_t _bump = 0;
    uint16_t _steps = 0;
    bool _verbose = false;
    bool _broken = false;

};
#endif