#ifndef UKMAKER_ASSEMBLY_VOCABULARY_H
#define UKMAKER_ASSEMBLY_VOCABULARY_H

#include <string.h>
#include <stdint.h>
#include "../runtime/ForthCPU/ForthIS.h"
#include "Token.h"
#include "Opcodes.h"

class AssemblyVocabulary {

    public:
    AssemblyVocabulary() {

        opcodes = new Opcodes();

        
        for(int i=0; i<65; i++) {
            for(int j=0; j<16; j++) {
                aliases[j][i] = '\0';
            }
        }

        argnames[REG_0]  = "R0";
        argnames[REG_1]  = "R1";
        argnames[REG_2]  = "R2";
        argnames[REG_3]  = "R3";
        argnames[REG_4]  = "R4";
        argnames[REG_5]  = "R5";
        argnames[REG_6]  = "R6";
        argnames[REG_7]  = "R7";
        argnames[REG_A]  = "RA";
        argnames[REG_B]  = "RB";
        argnames[REG_RL] = "RL";
        argnames[REG_WA] = "WA";
        argnames[REG_SP] = "SP";
        argnames[REG_RS] = "RS";
        argnames[REG_FP] = "FP";
        argnames[REG_I]  = "RI";

        ccnames[JMP_CC_C] = "C";
        ccnames[JMP_CC_Z] = "Z";
        ccnames[JMP_CC_P] = "P";
        ccnames[JMP_CC_S] = "M";

        directives[DIRECTIVE_TYPE_ORG] = "ORG";
        directives[DIRECTIVE_TYPE_DATA] = "DATA";
        directives[DIRECTIVE_TYPE_ALIAS] = "ALIAS";
        directives[DIRECTIVE_TYPE_PLAIN_STRING] = "SDATA";
        directives[DIRECTIVE_TYPE_NWORD_STRING] = "N";
        directives[DIRECTIVE_TYPE_RWORD_STRING] = "R";
        directives[DIRECTIVE_TYPE_IWORD_STRING] = "I";
        directives[DIRECTIVE_TYPE_XWORD_STRING] = "X";
        directives[DIRECTIVE_TYPE_CWORD_STRING] = "C";

    }

    void setAlias(uint8_t aliasReg, const char *aliasName) {
        strcpy(aliases[aliasReg],aliasName);
    }
   
    const char *argname(uint8_t arg) {
        if(arg > 15) return NULL;
        return argnames[arg];
    }
   
    const char *ccname(uint8_t cc) {
        if(cc > 3) return NULL;
        return ccnames[cc];
    }

    const char *directive(uint8_t dir) {
        if(dir > 8) return NULL;
        return directives[dir];
    }

    Opcode *findOpcode(char *source, int pos, int len) {
        return opcodes->find(source, pos, len);
    }

    int findAlias(char *source, int pos, int len) {
        for(int i=0; i<16; i++) {
            if((int)strlen(aliases[i]) == len) {
                int equal = true;
                for(int j=0; j<len; j++) {
                    if(source[pos + j] != aliases[i][j]) {
                        equal = false;
                    }
                }
                if(equal) return i;
            }
        }
        return -1;
    }

    int findArg(char *source, int pos, int len) {
        for(int i=0; i<16; i++) {
            if((int)strlen(argnames[i]) == len) {
                int equal = true;
                for(int j=0; j<len; j++) {
                    if(source[pos + j] != argnames[i][j]) {
                        equal = false;
                    }
                }
                if(equal) return i;
            }
        }
        return findAlias(source, pos, len);
    }

    int findDirective(char *source, int pos, int len) {
        for(int i=0; i<9; i++) {
            if((int)strlen(directives[i]) == len) {
                int equal = true;
                for(int j=0; j<len; j++) {
                    if(source[pos + j] != directives[i][j]) {
                        equal = false;
                    }
                }
                if(equal) return i;
            }
        }
        return -1;
    }

    protected:

        Opcodes *opcodes;
        const char *argnames[16];
        const char *ccnames[4];
        const char *directives[9];
        char aliases[16][65];
};
#endif