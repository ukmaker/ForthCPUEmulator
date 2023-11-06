#ifndef UKMAKER_DUMPER_H
#define UKMAKER_DUMPER_H

#include "Assembler.h"

class Dumper {

    public:
    Dumper() {}
    ~Dumper() {}

    void writeCPP(const char *name, Assembler *fasm, Memory *mem, uint16_t romStart, uint16_t romSize, bool progmem) {
        FILE *fp = fopen(name, "w");
        fprintf(fp, "#ifndef UKMAKER_FORTH_IMAGE_H\n");
        fprintf(fp, "#define UKMAKER_FORTH_IMAGE_H\n");
        fprintf(fp, "#include \"FArduino.h\"\n");
        fprintf(fp,"/******************************\n");
        fprintf(fp, "* Constants\n");
        fprintf(fp, "*****************************/\n");
        fprintf(fp,"#define FORTH_ROMSIZE 0x%x\n", romSize);
        Token *tok = fasm->tokens;
        while(tok != NULL) {
            if(tok->isConst()) {
                fprintf(fp, "#define FORTH_%s 0x%04x\n", tok->name+1, tok->value); 
            }
            tok = tok->next;
        }
        // Write the ROM image
        if(progmem) {
            fprintf(fp, "const uint8_t rom[%d] PROGMEM = {\n",romSize);
        } else {
            fprintf(fp, "const uint8_t rom[%d] = {\n",romSize);
        }

        uint16_t last = romSize % 8;
        bool first = true;

        for(uint16_t i=0; i<romSize-last; i+=8) {
            writeLine(fp, mem, romStart, i, 8, first);
            first = false;
        }

        writeLine(fp, mem, romStart, romSize - last, last, false);

        fprintf(fp, "}; // rom\n");

        fprintf(fp, "#endif // UKMAKER_FORTH_IMAGE_H\n");
        

        fclose(fp);
    }

    void writeLine(FILE *fp, Memory *mem, uint16_t romStart, uint16_t start, uint16_t n, bool first) {

        fprintf(fp, "/* 0x%04x */  ", (uint16_t)romStart + start);

        for(uint16_t j=0; j<n; j++) {
            if(first) {
                fprintf(fp," 0x%02x ", mem->getC(romStart + start + j));
            } else {
                fprintf(fp,",0x%02x ", mem->getC(romStart + start + j));
            }
            first = false;
        }
        fprintf(fp, "   /* ");
        for(uint16_t j=0; j<n; j++) {
            uint8_t c = (char)mem->getC(romStart + start +j);
            c = c > 32 ? c : 32;
            fprintf(fp,"%c ", (char)c);
        }
        
        fprintf(fp, "*/\n");
    }

    void dump(Assembler *fasm) {

        printf("==============================\n");
        printf("Code \n");
        printf("==============================\n");
        Token *tok = fasm->tokens;
        while(tok != NULL) {
            switch(tok->type) {
                case TOKEN_TYPE_COMMENT: break; // discard comments
                case TOKEN_TYPE_CONST: 
                    printf("%s: %04x\n", tok->name, tok->value); 
                    break;
                case TOKEN_TYPE_LABEL:  
                    printf("%04x ", tok->address);
                    printf("%s: \n", tok->name); 
                    break;
                case TOKEN_TYPE_OPCODE: 
                    printf("%04x   ", tok->address);
                    printOpcode(fasm, tok);
                    printf("\n"); 
                    break;
                case TOKEN_TYPE_STR:  
                    printf("%04x ", tok->address);
                    printf("%s: %s\n", tok->name, tok->str); 
                    break;
                case TOKEN_TYPE_VAR:  
                    printf("%04x ", tok->address);
                    printf("%s: %04x\n", tok->name, tok->value); 
                    break;
                case TOKEN_TYPE_DIRECTIVE:  
                    
                    switch(tok->directive) {
                        case DIRECTIVE_TYPE_ORG: printf(".ORG: %04x\n", tok->value); break;
                        case DIRECTIVE_TYPE_DATA: 
                            printf("%04x ", tok->address);
                            printf(".DATA: %04x\n", tok->value); 
                            break;
                        case DIRECTIVE_TYPE_PLAIN_STRING: 
                            printf("%04x ", tok->address);
                            printf(".SDATA: \"%s\"\n", tok->str); 
                            break;
                        case DIRECTIVE_TYPE_NWORD_STRING: 
                        case DIRECTIVE_TYPE_RWORD_STRING: 
                        case DIRECTIVE_TYPE_IWORD_STRING: 
                        case DIRECTIVE_TYPE_XWORD_STRING: 
                        case DIRECTIVE_TYPE_CWORD_STRING: 
                            printf("%04x ", tok->address);
                            printf("%04x .SDATA: \"%s\"\n", fasm->headerWord(tok), tok->str); 
                            break;
                        default: break;
                    }
                break;
                default: break;
            }
            tok = tok->next;
        }
    }

    void instructionRR(Assembler *fasm, Token *tok) {
        printf("%s,%s",fasm->vocab.argname(tok->opcode->getArgA()), fasm->vocab.argname(tok->opcode->getArgB()));
    }

    void instructionRU4(Assembler *fasm, Token *tok) {
        // register in arga 4-bit tiny immediates in value
        if(tok->symbolic) {
            printf("%s,%s",fasm->vocab.argname(tok->opcode->getArgA()), tok->str);
        } else {
            printf("%s,%01x",fasm->vocab.argname(tok->opcode->getArgA()), tok->value);
        }
    }

    void instructionRU5(Assembler *fasm, Token *tok) {
        // register in arga 4-bit tiny immediates in value
        if(tok->symbolic) {
            printf("%s,%s",fasm->vocab.argname(tok->opcode->getArgA()), tok->str);
        } else {
            printf("%s,%01x",fasm->vocab.argname(tok->opcode->getArgA()), tok->opcode->getU5());
        }
    }

    void instructionU8(Assembler *fasm, Token *tok) {
        if(tok->symbolic) {
            printf("%s", tok->str);
        } else {
            printf("%0x2", tok->value);
        }
    }

    void instructionS8(Assembler *fasm, Token *tok) {
        if(tok->symbolic) {
            printf("%s", tok->str);
        } else {
            printf("%0x2", tok->value);
        }
    }

    void instructionU16(Assembler *fasm, Token *tok) {
        if(tok->symbolic) {
            printf("%s,%s",fasm->vocab.argname(tok->opcode->getArgA()), tok->str);
        } else {
            printf("%s,%04x",fasm->vocab.argname(tok->opcode->getArgA()), tok->value);
        }
    }

    void printOpcode(Assembler *fasm, Token *tok) {
        if(tok == NULL || tok->opcode == NULL) {
            return;
        }
        
        printf("%s", tok->opcode->getName());
        if(tok->opcode->isConditional()) {
            printf("[");
            if(tok->opcode->isConditionNegated()) {
                printf("N");
            }
            printf("%s", fasm->vocab.ccname(tok->opcode->getCondition()));
            printf("] ");
        } else {
            printf(" ");
        }

        if(tok->opcode->isALU()) {
            switch(tok->opcode->getALUMode()) {

                case ALU_MODE_REG_REG:
                    instructionRR(fasm, tok);
                break;

                case ALU_MODE_REG_U4:
                    instructionRU4(fasm, tok);
                break;

                case ALU_MODE_REGA_U8:
                    instructionU8(fasm, tok);
                 break;
            
                default: // ALU_MODE_REGA_S8
                    instructionS8(fasm, tok);
                break;
            }

        } else if(tok->opcode->isLDS()) {
            switch(tok->opcode->getLDSMode()) {
                case LDS_MODE_REG_REG:
                    instructionRR(fasm, tok);
                break;

                case LDS_MODE_REG_HERE:
                    instructionU16(fasm, tok);
                break;

                case LDS_MODE_REG_REG_INC:
                    instructionRR(fasm, tok);
                break;

                case LDS_MODE_REG_REG_DEC:
                    instructionRR(fasm, tok);
                break;

                case LDS_MODE_REG_RL:
                    instructionRR(fasm, tok);
                break;

                case LDS_MODE_REG_FP:
                    instructionRR(fasm, tok);
                break;

                case LDS_MODE_REG_SP:
                    instructionRR(fasm, tok);
                break;

                default: // LDS_MODE_REG_RS:
                    instructionRR(fasm, tok);
                break;
            }

        } else if(tok->opcode->isJMP()) {

        } else {

        }
    }

};
#endif