#ifndef UKMAKER_ASSEMBLER_H
#define UKMAKER_ASSEMBLER_H
#include "../runtime/ForthCPU/ForthIS.h"
#include "AssemblyVocabulary.h"
#include "Token.h"
#include "Symbol.h"
#include "Option.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include "../runtime/ForthCPU/Memory.h"

class Assembler
{

public:

    static inline const char *NAME_EXPECTED = "Name expected";
    static inline const char *ALIAS_EXPECTED = "Alias expected";
    static inline const char *REGISTER_NAME_EXPECTED = "Register name expected";  
    static inline const char *DIRECTIVE_EXPECTED = "Directive expected";
    static inline const char *NUMBER_EXPECTED = "Number expected";
    static inline const char *OPCODE_EXPECTED = "Opcode expected";
    static inline const char *INVALID_CONDITION = "Invalid condition";
    static inline const char *COMMA_EXPECTED = "Comma expected";
    static inline const char *TINY_RANGE_EXCEEDED = "Tiny immediate range (-8 to 7) exceeded"; 
    static inline const char *TINY_UNSIGNED_RANGE_EXCEEDED = "Tiny unsigned immediate range (0 to 15) exceeded";
    static inline const char *IMMEDIATE_RANGE_EXCEEDED = "Immediate range (-128 to 127) exceeded";
    static inline const char *UNSIGNED_IMMEDIATE_RANGE_EXCEEDED = "Unsigned immediate range (0 to 255) exceeded";
    static inline const char *NUMBER_OR_LABEL_EXPECTED = "Number or label expected";
    static inline const char *CONSTANT_EXPECTED = "Constant expected";
    static inline const char *STRING_EXPECTED = "String expected";
    static inline const char *HEX_NUMBER_EXPECTED = "Hex number expected";
    static inline const char *BINARY_NUMBER_EXPECTED = "Binary number expected";
    static inline const char *DECIMAL_NUMBER_EXPECTED = "Decimal number expected";

    Assembler()
    {
        // vocab.init();
    }
    ~Assembler() {}

    bool hasErrors() {
        return phase1Error | phase2Error | phase3Error;
    }

    Token *firstLabel()
    {
        currentLabel = tokens;
        return nextLabel();
    }

    Token *nextLabel()
    {
        Token *found;
        while (currentLabel != NULL)
        {
            found = currentLabel;
            currentLabel = currentLabel->next;
            if (found->type == TOKEN_TYPE_LABEL)
            {

                return found;
            }
        }
        return NULL;
    }

    void clearOptions() {
        Option *opt = options;
        options = NULL;
        while(opt != NULL) {
            Option *next = opt->next;
            delete opt;
            opt = next;
        }
    }

    void setOption(const char *name, uint16_t value) {
        if(getOption(name) != NULL) {
            printf("Error: option %s already defined\n", name);
        } else {
            Option *opt = new Option(name,value);
            if(options == NULL) {
                options = opt;
            } else {
                Option *n = options;
                while(n->next != NULL) n = n->next;
                n->next = opt;
            }
        }
    }

    Option  *getOption(const char *name) {
        if(options == NULL) return NULL;
        Option *opt = options;
        while(opt != NULL) {
            if(strcmp(opt->name, name) == 0) return opt;
            opt = opt->next;
        }

        return NULL;
    }

    int getLabelAddress(const char *name)
    {
        Token *tok = tokens;
        while (tok != NULL)
        {
            if (tok->isLabel() && tok->name != NULL && (strcmp(tok->name, name) == 0))
            {
                return tok->address;
            }
            tok = tok->next;
        }
        return -1;
    }

    Token *getOpcode(uint16_t addr)
    {
        Token *tok = tokens;
        while (tok != NULL)
        {
            if (tok->type == TOKEN_TYPE_OPCODE && tok->address == addr)
            {
                return tok;
            }
            tok = tok->next;
        }
        return NULL;
    }

    Token *getLabel(uint16_t addr) {
        Token *tok = tokens;
        while (tok != NULL)
        {
            if (tok->type == TOKEN_TYPE_LABEL && tok->address == addr)
            {
                return tok;
            }
            tok = tok->next;
        }
        return NULL;        
    }

    bool slurp(const char *fileName)
    {
        phase1Error = false;
        phase2Error = false;
        phase3Error = false;

        FILE *file = fopen(fileName, "r");
        size_t n = 0;
        int c;

        if (file == NULL)
        {
            return false; // could not open file
        }

        fseek(file, 0, SEEK_END);
        sourceLen = ftell(file);
        fseek(file, 0, SEEK_SET);
        source = (char *)malloc(sourceLen + 1);

        while ((c = fgetc(file)) != EOF)
        {
            source[n++] = (char)c;
        }

        source[n] = '\0';
       //sourceLen = n - 1;

        // fclose(file);

        line = 1;
        pos = 1;
        lastPos = 1;
        idx = 0;

        return true;
    }

    long fileSize()
    {
        return sourceLen;
    }

    /**
     * Scan and tokenize
     */
    void pass1()
    {
        symbols = NULL;
        idx = 0;
        line = 1;
        pos = 1;

        tokens = getToken();
        Token *tok = tokens;
        Symbol *sym = symbols;

        while (tok->type != TOKEN_TYPE_EOF)
        {

            switch (tok->type)
            {
            case TOKEN_TYPE_ERROR:
                phase1Error = true;
                printf("ERROR: %s at line %d col %d\n", tok->str, tok->line, tok->pos);
                break;

            case TOKEN_TYPE_CONST:
            case TOKEN_TYPE_LABEL:
            case TOKEN_TYPE_STR:
            case TOKEN_TYPE_VAR:
                sym = getSymbol(tok->name);
                if (sym->token != NULL)
                {
                    printf("ERROR: symbol \"%s\" redefined at line %d (first defined at line %d)",
                           tok->name,
                           tok->line,
                           sym->token->line);
                    phase1Error = true;
                }
                else
                {
                    sym->token = tok;
                }
                break;
            case TOKEN_TYPE_DIRECTIVE:
                if(tok->isAlias()) {
                    vocab.setAlias(tok->opcode->getArgA(), tok->str);
                }
                break;

            default:
                break;
            }
            tok->next = getToken();
            tok = tok->next;
        }
    }

    /**
     * Assign addresses to vars, strings and opcodes
     */
    void pass2()
    {
        uint16_t addr = 0; // no .org or anything yet. All code starts at address zero
        Token *tok = tokens;
        Token *label = firstLabel();
        Symbol *sym;
        while (tok != NULL)
        {
            switch (tok->type)
            {
            case TOKEN_TYPE_COMMENT:
                break; // No code to emit
            case TOKEN_TYPE_CONST:
                break; // No code to emit
            case TOKEN_TYPE_DIRECTIVE:
                if (tok->isOrg())
                {
                    if (tok->symbolic)
                    {
                        // must resolve to an already defined constant
                        sym = getSymbol(tok->str);
                        if (sym == NULL)
                        {
                            printf("ERROR: undefined symbol %s at line %d\n",
                                   tok->str, tok->line);
                            phase2Error = true;
                        }
                        else
                        {
                            tok->value = sym->token->value;
                        }
                    }
                    addr = tok->value;
                }
                else if (tok->isData())
                {
                    tok->address = addr;
                    if (label != NULL && tok->address == label->address)
                    {
                        tok->label = label;
                        label = nextLabel();
                    }
                    addr += 2;
                }
                else if (tok->isPStringData())
                {
                    // Must be a string
                    tok->address = addr;
                    if (label != NULL && tok->address == label->address)
                    {
                        tok->label = label;
                        label = nextLabel();
                    }
                    addr += tok->value;
                    if (addr & 1)
                        addr++; // align to word boundaries
                }
                else if (tok->isHeader())
                {
                    // Must be a string
                    tok->address = addr;
                    if (label != NULL && tok->address == label->address)
                    {
                        tok->label = label;
                        label = nextLabel();
                    }
                    addr += tok->value + 2; // allow space for the string and the length word
                    if (addr & 1)
                        addr++; // align to word boundaries
                }
                break;

            case TOKEN_TYPE_VAR:
                if (tok->symbolic)
                {
                    // must resolve to an already defined constant
                    sym = getSymbol(tok->str);
                    if (sym == NULL)
                    {
                        printf("ERROR: undefined symbol %s at line %d\n",
                               tok->str, tok->line);
                        phase2Error = true;
                    }
                    else
                    {
                        tok->value = sym->token->value;
                    }
                }
                tok->address = addr;
                addr += tok->value;
                break;

            case TOKEN_TYPE_OPCODE:
            {
                tok->address = addr;
                if (label != NULL && tok->address == label->address)
                {
                    tok->label = label;
                    label = NULL;//nextLabel();
                }
                addr += 2;
                if(tok->opcode->isImmediate()) {
                    addr += 2;
                }
            }
            // Resolve symbols on the next pass
            break;

            case TOKEN_TYPE_STR:
                tok->address = addr;
                addr += tok->value + 2;
                if (addr & 1)
                    addr++; // align to word boundaries
                break;

            case TOKEN_TYPE_LABEL:
                tok->address = addr;
                label = tok;
                break;

            case TOKEN_TYPE_EOF:
                break;
            case TOKEN_TYPE_ERROR:
                printf("ERROR: %s at line %d col %d\n", tok->str, tok->line, tok->pos);
                phase2Error = true;
                break;
            default:
                break;
            }

            tok = tok->next;
        }
    }

    /**
     * On this pass we can resolve any symbols
     */
    void pass3()
    {
        // Resolve all the symbols now that we can reserve space appropriately
        Token *tok = tokens;
        while (tok != NULL)
        {
            switch (tok->type)
            {
            case TOKEN_TYPE_DIRECTIVE:
            case TOKEN_TYPE_OPCODE:
            case TOKEN_TYPE_VAR:
                if (tok->symbolic)
                {
                    if(!dereferenceSymbol(tok)) {
                        phase3Error = true;
                    }
                }
                break;

            case TOKEN_TYPE_ERROR:
                printf("ERROR: %s at line %d col %d\n", tok->str, tok->line, tok->pos);
                phase3Error = true;
                break;
             
            case TOKEN_TYPE_STR:
            case TOKEN_TYPE_COMMENT:
            case TOKEN_TYPE_CONST:
            case TOKEN_TYPE_LABEL:
            case TOKEN_TYPE_EOF:
            default:
                break;
            }
            tok = tok->next;
        }
    }

    void dump()
    {
        Token *tok = tokens;
        Symbol *sym = symbols;

        printf("==============================\n");
        printf("Unresolved Symbols\n");
        printf("==============================\n");
        while (sym != NULL)
        {
            if (!sym->resolved())
                printf("%s\n", sym->name);
            sym = sym->next;
        }
        sym = symbols;
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
        sym = symbols;
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
        sym = symbols;
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
        tok = firstLabel();
        printf("==============================\n");
        printf("Unresolved Labels \n");
        printf("==============================\n");
        while (tok != NULL)
        {
            if (tok->address == -1)
            {
                printf("%s: Line %d\n", tok->name, tok->line);
            }
            tok = nextLabel();
        }
    }

    uint16_t headerWord(Token *tok) {
        int r = (1 << HEADER_HEADER_BIT);
        if(tok->isHeader()) {
            switch(tok->directive) {
                case DIRECTIVE_TYPE_NWORD_STRING:
                    r |= tok->strlen() | (HEADER_SCOPE_NORMAL << HEADER_SCOPE_BITS);
                    break;

                case DIRECTIVE_TYPE_RWORD_STRING:
                    r |= tok->strlen() | (HEADER_SCOPE_RUNTIME << HEADER_SCOPE_BITS);
                    break;
                    
                case DIRECTIVE_TYPE_IWORD_STRING:
                    r |= tok->strlen() | (HEADER_SCOPE_IMMEDIATE << HEADER_SCOPE_BITS);
                    break;
                    
                case DIRECTIVE_TYPE_XWORD_STRING:
                    r |= tok->strlen() | (HEADER_SCOPE_EXECUTIVE << HEADER_SCOPE_BITS);
                    break;
                    
                case DIRECTIVE_TYPE_CWORD_STRING:
                    r |= tok->strlen() | (HEADER_SCOPE_COMPILE << HEADER_SCOPE_BITS);
                    break;
                    

                default: break;
            }
        }

        return r;
    }

    void writeCode()
    {
        Token *tok = tokens;
        uint8_t written;
        const char *c;
        uint16_t headerLen;

        while (tok != NULL)
        {
            switch (tok->type)
            {

            case TOKEN_TYPE_DIRECTIVE:
                switch (tok->directive)
                {
                case DIRECTIVE_TYPE_DATA:
                    printf("%04x %02x %02x\n", tok->address, (tok->value & 0xff00) >> 8, tok->value & 0xff);
                    break;

                case DIRECTIVE_TYPE_PLAIN_STRING:
                {
                    c = tok->str;
                    written = 0;
                    while (*c != '\0')
                    {
                        if (written % 8 == 0)
                        {
                            printf("\n%04x ", tok->address + written);
                        }
                        printf("%02x ", *c);
                        written++;
                        c++;
                    }
                    printf("\n");
                }
                case DIRECTIVE_TYPE_NWORD_STRING:
                case DIRECTIVE_TYPE_RWORD_STRING:
                case DIRECTIVE_TYPE_IWORD_STRING:
                case DIRECTIVE_TYPE_XWORD_STRING:
                case DIRECTIVE_TYPE_CWORD_STRING:
                {
                    headerLen = headerWord(tok);
                    c = tok->str;
                    printf("\n%04x %02x %02x ", tok->address, headerLen & 0xff, headerLen >> 8);
                    written = 2;
                    while (*c != '\0')
                    {
                        if (written % 8 == 0)
                        {
                            printf("\n%04x ", tok->address + written);
                        }
                        printf("%02x ", *c);
                        written++;
                        c++;
                    }
                    printf("\n");
                }
                break;
                default:
                    break;
                }
                break;

            case TOKEN_TYPE_VAR:
                printf("%04x %02x %02x\n", tok->address, (tok->value & 0xff00) >> 8, tok->value & 0xff);
                break;

            case TOKEN_TYPE_STR:
            {
                c = tok->str;
                written = 0;
                while (*c != '\0')
                {
                    if (written % 8 == 0)
                    {
                        printf("\n%04x ", tok->address + written);
                    }
                    printf("%02x ", *c);
                    written++;
                    c++;
                }
                printf("\n");
            }
            break;

            case TOKEN_TYPE_OPCODE:
            {
                printf("%04x %02x %02x\n", tok->address, tok->highByte(), tok->lowByte());
                if(tok->opcode->isImmediate()) {
                    printf("%04x %02x %02x\n", tok->address + 2, tok->value & 0xff, (tok->value & 0xff00) >> 8);
                }
            }
            break;

            default:
                break;
            }

            tok = tok->next;
        }
    }

    void writeMemory(Memory *ram)
    {
        Token *tok = tokens;
        const char *c;
        int written;
        uint16_t headerLen;

        while (tok != NULL)
        {
            switch (tok->type)
            {

            case TOKEN_TYPE_DIRECTIVE:
                switch (tok->directive)
                {
                case DIRECTIVE_TYPE_DATA:
                    ram->put(tok->address, tok->value);
                    break;

                case DIRECTIVE_TYPE_PLAIN_STRING:
                {
                    c = tok->str;
                    // ram->put(tok->address, tok->value);
                    written = 0;
                    while (*c != '\0')
                    {
                        ram->putC(tok->address + written, *c);
                        written++;
                        c++;
                    }
                }
                break;
                
                case DIRECTIVE_TYPE_NWORD_STRING:
                case DIRECTIVE_TYPE_RWORD_STRING:
                case DIRECTIVE_TYPE_IWORD_STRING:
                case DIRECTIVE_TYPE_XWORD_STRING:
                case DIRECTIVE_TYPE_CWORD_STRING:
                {
                    headerLen = headerWord(tok);
                    c = tok->str;
                    ram->put(tok->address, headerLen);
                    written = 2;
                    while (*c != '\0')
                    {
                        ram->putC(tok->address + written, *c);
                        written++;
                        c++;
                    }
                }
                break;

                default:
                    break;
                }
                break;

            case TOKEN_TYPE_VAR:
                ram->put(tok->address, tok->value);
                break;

            case TOKEN_TYPE_STR:
            {
                c = tok->str;
                // ram->put(tok->address, tok->value);
                written = 0;
                while (*c != '\0')
                {
                    ram->putC(tok->address + written, *c);
                    written++;
                    c++;
                }
            }
            break;

            case TOKEN_TYPE_OPCODE:
            {
                ram->putC(tok->address + 1, tok->highByte());
                ram->putC(tok->address, tok->lowByte());
                if(tok->opcode->isImmediate()) {
                    ram->putC(tok->address + 3, (tok->value & 0xff00) >> 8);
                    ram->putC(tok->address + 2, tok->value & 0xff);
                }
            }
            break;

            default:
                break;
            }

            tok = tok->next;
        }
    }

    bool dereferenceSymbol(Token *tok)
    {

        Symbol *sym;

        if (tok->isData() || tok->isOrg() || tok->isVar())
        {
            // constants are put in-place
            // references to strings or vars are the address
            sym = getSymbol(tok->str);
            if (sym->token == NULL)
            {
                printf("ERROR: unknown symbol %s at line %d \n", tok->str, tok->line);
                return false;
            }
            switch (sym->token->type)
            {
            case TOKEN_TYPE_CONST:
                tok->value = sym->token->value;
                break;
            case TOKEN_TYPE_LABEL:
            case TOKEN_TYPE_STR:
            case TOKEN_TYPE_VAR:
                tok->value = sym->token->address;
                break;
            default:
                break;
            }
        }
        else if (tok->isStringData())
        {
            printf("ERROR: invalid string data %s at line %d \n", tok->str, tok->line);
            return false;
        }
        else
        {
            if(tok->opcode->expectsU4()) {

                // register in arga 4-bit tiny immediates in value
                sym = getSymbol(tok->str);
                if (sym->token == NULL)
                {
                    printf("ERROR: unknown symbol %s at line %d \n", tok->str, tok->line);
                    return false;
                }
                switch (sym->token->type)
                {
                    case TOKEN_TYPE_CONST:
                        tok->value = sym->token->value;
                        break;
                    case TOKEN_TYPE_LABEL:
                    case TOKEN_TYPE_STR:
                    case TOKEN_TYPE_VAR:
                        tok->value = sym->token->address;
                        break;
                    default:
                        break;
                }

                if (tok->value > 15 || tok->value < 0)
                {
                    printf("ERROR: tiny value exceeded (%d) at line %d \n", tok->value, tok->line);
                    return false;
                }

                tok->opcode->setArgB(tok->value);
                tok->symbolic = false;                    

            } else if(tok->opcode->expectsU8() || tok->opcode->expectsU8()) {

                // RA, 8-bit immediate
                sym = getSymbol(tok->str);
                if (sym->token == NULL)
                {
                    printf("ERROR: unknown symbol %s at line %d \n", tok->str, tok->line);
                    return false;
                }
                tok->value = sym->token->value;

                if(tok->opcode->expectsU8() && (tok->value > 127 || tok->value < -128))
                {
                    printf("ERROR: signed byte value exceeded (%d) at line %d \n", tok->value, tok->line);
                    return false;
                } else if(tok->value > 255 || tok->value < -0) {
                    printf("ERROR: unsigned byte value exceeded (%d) at line %d \n", tok->value, tok->line);
                    return false;
                }

                tok->opcode->setArgA((tok->value & (0xf << 4)) >> 4);
                tok->opcode->setArgA(tok->value);
                tok->symbolic = false;
            } else if(tok->opcode->expectsU5()) {
                // 5-bit immediate
                sym = getSymbol(tok->str);
                if (sym->token == NULL)
                {
                    printf("ERROR: unknown symbol %s at line %d \n", tok->str, tok->line);
                    return false;
                }

                tok->value = sym->token->value;

                if((tok->value > 31 || tok->value < 0))
                {
                    printf("ERROR: U5 value exceeded (%d) at line %d \n", tok->value, tok->line);
                    return false;
                }
                tok->opcode->setU5(tok->value);
                tok->symbolic = false;
            } else if(tok->opcode->expectsU16()) {
                sym = getSymbol(tok->str);
                if (sym->token == NULL)
                {
                    printf("ERROR: unknown symbol %s at line %d \n", tok->str, tok->line);
                    return false;
                }
                if (sym->token->isLabel() || sym->token->isVar() || sym->token->isStr())
                {
                    tok->value = sym->token->address;
                }
                else
                {
                    tok->value = sym->token->value;
                    if (tok->value > 65535 || tok->value < -32768)
                    {
                        printf("ERROR: long value exceeded (%d) at line %d \n", tok->value, tok->line);
                        return false;
                    }
                    if(tok->opcode->getJMPOp() == JMP_OP_JR || tok->opcode->getJMPOp() == JMP_OP_JRL) {
                        tok->value = (sym->token->address - tok->address - 2) >> 1;
                    }
                }
                tok->symbolic = false;
            }
        }

        return true;
    }

    Symbol *getSymbol(const char *name)
    {
        Symbol *sym = symbols;
        if (symbols == NULL)
        {
            symbols = new Symbol;
            symbols->name = name;
            sym = symbols;
        }
        else
        {
            Symbol *last;
            while (sym != NULL)
            {
                if (strcmp(sym->name, name) == 0)
                {
                    return sym;
                }
                last = sym;
                sym = sym->next;
            }
            // nothing found, append a new one
            sym = new Symbol;
            sym->name = name;
            last->next = sym;
        }
        return sym;
    }

    void skipLine()
    {
        while (idx < sourceLen)
        {
            if (source[idx] == '\n')
            {
                line++;
                lastPos = pos;
                pos = 1;
                return;
            }
            else
            {
                pos++;
            }
            idx++;
        }
    }

    Token *getToken() {
        Token *tok = _getToken();
        if(tok->isError()) {
            skipLine();
        }
        return tok;
    }

    Token *_getToken()
    {

        char c;
        while (idx < sourceLen)
        {

            skipSpaceOrEOL();
            if (idx >= sourceLen)
            {
                return Token::eof();
            }

            c = source[idx];
            switch (c)
            {
            case ' ':
                inc();
                break; // ignore leading spaces
            case '\n':
                inc();
                break; // ignore newlines
            case ';':
                inc();
                return getComment();
                break; // rest of the line is a comment
            case '#':
                inc();
                return getConstant();
                break;
            case '%':
                inc();
                return getVariable();
                break;
            case '$':
                inc();
                return getString();
                break;
            case '.':
                inc();
                return getDirective();
                break;
            default:
                if (isLabel())
                {
                    return getLabel();
                }
                else
                {
                    return getOpcode();
                }
                break;
            }
        }
        return Token::eof();
    }

    void skipSpaces()
    {
        while (idx < sourceLen)
        {
            if (isSpace(source[idx]))
            {
                inc();
            }
            else
            {
                break;
            }
        }
    }

    void skipSpaceOrEOL()
    {
        while (idx < sourceLen)
        {
            if (isSpaceOrEOL(source[idx]))
            {
                inc();
            }
            else
            {
                break;
            }
        }
    }

    // Returns true if the next token is a label
    // i.e. [_A-Za-z0-9+]:
    bool isLabel()
    {
        int here = idx;

        while (here < sourceLen)
        {
            char c = source[here];
            if (isAlphaNumeric(c) || isUnderscore(c))
            {
                here++;
            }
            else
            {
                if (c == ':')
                {
                    return here > idx;
                }
                return false;
            }
        }
        return false;
    }

    bool isUnderscore(char c)
    {
        return c == '_';
    }

    bool isAlphaNumeric(char c)
    {
        return isAlpha(c) || isNumeric(c) || isSpecial(c);
    }

    bool isAlpha(char c)
    {
        if (c >= 'A' && c <= 'Z')
            return true;
        if (c >= 'a' && c <= 'z')
            return true;
        return false;
    }

    bool isAlphaOrUnderscore(char c)
    {
        return isAlpha(c) || isUnderscore(c);
    }

    bool isNumeric(char c)
    {
        if (c >= '0' && c <= '9')
            return true;
        return false;
    }

    bool isSpecial(char c)
    {
        return c == '_';
    }

    bool isSpace(char c)
    {
        if (c == ' ' || c == '\t')
        {
            return true;
        }
        return false;
    }

    bool isSpaceOrSemi(char c)
    {
        if (c == ' ' || c == '\t' || c == ';')
        {
            return true;
        }
        return false;
    }

    bool isSpaceSemiOrEOL(char c)
    {
        if (c == ' ' || c == '\t' || c == ';' || c == '\n')
        {
            return true;
        }
        return false;
    }

    bool isStar(char c) 
    {
        return c == '*';
    }

    bool isAliasChar(char c)
    {  
        return isAlphaNumeric(c) || isStar(c);
    }

    bool isTerminator(char c)
    {
        const char *terminators = " ;\n";
        const char *p = strchr(terminators, c);
        return p != NULL;
    }

    bool isSpaceOrEOL(char c)
    {
        const char *spaces = " \n\t";
        const char *p = strchr(spaces, c);
        return p != NULL;
    }

    bool isEOL(char c) {
        return c == '\n';
    }

    void inc()
    {
        if (idx < sourceLen)
        {
            if (source[idx] == '\n')
            {
                line++;
                lastPos = pos;
                pos = 1;
            }
            else
            {
                pos++;
            }
            idx++;
        }
    }

    void dec()
    {
        if (idx > 0)
        {
            idx--;
            if (source[idx] == '\n')
            {
                pos = lastPos;
                line--;
            }
        }
    }

    Token *getDirective()
    {
        // First char must be alpha
        if (!isAlpha(source[idx]))
        {
            return Token::error(line, pos, DIRECTIVE_EXPECTED);
        }
        int here = idx;
        int p = pos - 1;
        inc();
        while ((idx < sourceLen) && isAlpha(source[idx]))
        {
            inc();
        }

        int directive = vocab.findDirective(source, here, idx - here);
        if (directive == -1)
        {
            return Token::error(line, pos, DIRECTIVE_EXPECTED);
        }

        Token *tok = new Token(NULL, line, p);
        tok->type = TOKEN_TYPE_DIRECTIVE;
        tok->directive = directive;
        switch (tok->directive)
        {
        case DIRECTIVE_TYPE_DATA:
        case DIRECTIVE_TYPE_ORG:
            if (!getImm(tok))
            {
                return tok;
            }
            break;
        case DIRECTIVE_TYPE_ALIAS:
            if (!getAlias(tok))
            {
                return tok;
            }
            break;        
        case DIRECTIVE_TYPE_PLAIN_STRING:
        case DIRECTIVE_TYPE_NWORD_STRING:
        case DIRECTIVE_TYPE_RWORD_STRING:
        case DIRECTIVE_TYPE_IWORD_STRING:
        case DIRECTIVE_TYPE_XWORD_STRING:
        case DIRECTIVE_TYPE_CWORD_STRING:
            if (parseString(tok) == -1)
            {
                return Token::error(line, pos, NUMBER_EXPECTED);
            }
            break;

        default:
            break;
        }

        return tok;
    }

    bool getAlias(Token *tok) {
        if(getArgA(tok) && comma(tok) && getAliasName(tok)) {
            return true;
        }
        return false;
    }

    Token *getOpcode()
    {
        Token *tok;

        // First char must be alpha
        if (!isAlpha(source[idx]))
        {
            return Token::error(line, pos, OPCODE_EXPECTED);
        }
        int here = idx;
        int condition = 0;
        inc();
        while ((idx < sourceLen) && isAlphaOrUnderscore(source[idx]))
        {
            inc();
        }

        Opcode *opcode = vocab.findOpcode(source, here, idx - here);
        if (opcode == NULL)
        {
            return Token::error(line, pos, OPCODE_EXPECTED);
        }

        tok = new Token(NULL, line, pos);
        tok->type = TOKEN_TYPE_OPCODE;
        tok->opcode = opcode;
        if (source[idx] == '[')
        {
            inc();
            condition = getCondition();
            if (condition == -1)
            {
                return Token::error(line, pos, INVALID_CONDITION);
            }
        }

        if (condition != 0)
        {
            tok->opcode->setCondition(condition);
        }

        if(tok->opcode->isALU()) {
            switch(tok->opcode->getALUMode()) {

                case ALU_MODE_REG_REG:
                    instructionRR(tok);
                break;

                case ALU_MODE_REG_U4:
                    instructionRU4(tok);
                break;

                case ALU_MODE_REGA_U8:
                    instructionU8(tok);
                 break;
            
                default: // ALU_MODE_REGA_S8
                    instructionS8(tok);
                break;
            }

        } else if(tok->opcode->isLDS()) {
            switch(tok->opcode->getLDSMode()) {
                case LDS_MODE_REG_REG:
                    instructionRR(tok);
                break;

                case LDS_MODE_REG_HERE:
                    instructionRI(tok);
                break;

                case LDS_MODE_REG_REG_INC:
                    instructionRR(tok);
                break;

                case LDS_MODE_REG_REG_DEC:
                    instructionRR(tok);
                break;

                case LDS_MODE_REG_RL:
                    instructionRR(tok);
                break;

                case LDS_MODE_REG_FP:
                    instructionRR(tok);
                break;

                case LDS_MODE_REG_SP:
                    instructionRR(tok);
                break;

                default: // LDS_MODE_REG_RS:
                    instructionRR(tok);
                break;
            }

        } else if(tok->opcode->isJMP()) {
            switch(tok->opcode->getJMPOp()) {
                case JMP_OP_JP:
                case JMP_OP_JPM:
                    instructionRb(tok);
                break;

                case JMP_OP_JPL:
                case JMP_OP_JPML:
                    instructionRb(tok);
                    link(tok);
                break;

                case JMP_OP_JPI:
                case JMP_OP_JR:
                    instructionU16(tok);
                    break;

                case JMP_OP_JPIL:
                case JMP_OP_JRL:
                    instructionU16(tok);
                    link(tok);
                break;
            }
        } else {
            // GEN calls
           // No arguments for any of these opcodes yet

        }
        return tok;
    }

    void instructionRa(Token *tok)
    {
        getArgA(tok);
    }

    void instructionRb(Token *tok)
    {
        getArgB(tok);
    }

    void instructionRR(Token *tok)
    {
        getArgA(tok) & comma(tok) & getArgB(tok);
    }

    void instructionU8(Token *tok)
    {
        getUImm8(tok);
    }

    void instructionS8(Token *tok)
    {
        getImm8(tok);
    }

    void instructionRU4(Token *tok)
    {
        getArgA(tok) & comma(tok) & getUImm4(tok);
    }

    void instructionRI(Token *tok)
    {
        getArgA(tok) & comma(tok) & getImm(tok);
    }

    void instructionU16(Token *tok)
    {
        getImm(tok);
    }

    void link(Token *tok) {
        tok->opcode->setLink(true);
    }

    /**
     * Returns the condition code as used in bit 3, negation in bit 2, code in bits 1 and 0
     * Leaves idx pointing at the next character after the ]
     * Returns -1 if an error occurred
     */
    int getCondition()
    {
        int cc = 8;
        if (source[idx] == 'N')
        {
            cc += 4;
            inc();
        }

        switch (source[idx])
        {
        case 'C':
            cc += JMP_CC_C;
            break;
        case 'Z':
            cc += JMP_CC_Z;
            break;
        case 'M':
            cc += JMP_CC_S;
            break;
        case 'P':
            cc += JMP_CC_P;
            break;
        default:
            return -1;
        }
        inc();
        if (source[idx] != ']')
        {
            return -1;
        }
        inc();

        return cc;
    }

    /**
     * @return True if parsing should continue
     */
    bool comma(Token *tok)
    {
        skipSpaces();
        if (source[idx] != ',')
        {
            tok->type = TOKEN_TYPE_ERROR;
            tok->str = COMMA_EXPECTED;
            return false;
        }
        inc();
        return true;
    }

    bool getUImm4(Token *tok)
    {
        if (getImm(tok))
        {
            if (tok->value >= 0 && tok->value < 16)
            {
                tok->opcode->setArgB(tok->value);
                return true;
            }
            tok->type = TOKEN_TYPE_ERROR;
            tok->str = TINY_UNSIGNED_RANGE_EXCEEDED;
        }
        return false;
    }

    bool getImm8(Token *tok)
    {
        if (getImm(tok))
        {
            if (tok->value > -129 && tok->value < 128)
            {
                tok->opcode->setArgA(tok->value >> 4);
                tok->opcode->setArgB(tok->value & 0x0f);
                return true;
            }
            tok->type = TOKEN_TYPE_ERROR;
            tok->str = IMMEDIATE_RANGE_EXCEEDED;
        }
        return false;
    }

    bool getUImm8(Token *tok)
    {
        if (getImm(tok))
        {
            if (tok->value >= 0 && tok->value < 256)
            {
                tok->opcode->setArgA(tok->value >> 4);
                tok->opcode->setArgB(tok->value & 0x0f);
                return true;
            }
            tok->type = TOKEN_TYPE_ERROR;
            tok->str = UNSIGNED_IMMEDIATE_RANGE_EXCEEDED;
        }
        return false;
    }

    // this should reference constants also. TBD
    bool getImm(Token *tok)
    {
        skipSpaces();
        if (source[idx] == '#' || source[idx] == '$' || source[idx] == '%')
        {
            if (!getSymbolName(tok))
            {
                return false;
            }
        }
        else if (isAlpha(source[idx]))
        {
            if (!getSymbolName(tok))
            {
                return false;
            }
        }
        else if (parseNumber(tok) == -1)
        {
            return error(tok, NUMBER_OR_LABEL_EXPECTED);
        }
        return true;
    } 

    // move the name following idx to the token's str
    // set the symoblic flag
    bool getSymbolName(Token *tok)
    {
        int here = idx;
        idx++;
        pos++;
        if (idx == sourceLen)
        {
            return error(tok, NAME_EXPECTED);
        }

        while (idx <= sourceLen)
        {
            if (isSpaceSemiOrEOL(source[idx]))
            {
                break;
            }
            if (!isAlphaNumeric(source[idx]))
            {
                return error(tok, NAME_EXPECTED);
            }
            idx++;
            pos++;
        }
        int len = idx - here;        char *name = (char *)malloc(len + 1); // +1 for the ending \0
        strncpy(name, &source[idx - len], len);
        name[len] = '\0';
            tok->str = name;
            tok->symbolic = true;
        return true;
    }


    // move the name following idx to the token's str
    // set the symoblic flag
    bool getAliasName(Token *tok)
    {
        int here = idx;
        idx++;
        pos++;
        if (idx == sourceLen)
        {
            return error(tok, ALIAS_EXPECTED);
        }
        skipSpaces();
        while (idx < sourceLen)
        {
            if (isSpaceSemiOrEOL(source[idx]))
            {
                break;
            }
            if (!isAliasChar(source[idx]))
            {
                return error(tok, ALIAS_EXPECTED);
            }
            idx++;
            pos++;
        }
        int len = idx - here;        char *name = (char *)malloc(len + 1); // +1 for the ending \0
        strncpy(name, &source[idx - len], len);
        name[len] = '\0';
        tok->str = name;
        return true;
    }

    bool error(Token *tok, const char *msg)
    {
        tok->type = TOKEN_TYPE_ERROR;
        tok->str = msg;
        return false;
    }

    bool getArgA(Token *tok)
    {
        int arg;
        if ((arg = getArg(tok)) != -1)
        {
            tok->opcode->setArgA(arg);
            return true;
        }

        return false;
    }

    bool getArgB(Token *tok)
    {
        int arg;
        if ((arg = getArg(tok)) != -1)
        {
            tok->opcode->setArgB(arg);
            return true;
        }

        return false;
    }

    int getArg(Token *tok)
    {
        // Any of the register definitions
        skipSpaces();
        int here = idx;
        while (idx < sourceLen)
        {
            if (!isAliasChar(source[idx]))
            {
                break;
            }
            inc();
        }

        if (idx == here)
        {
            tok->type = TOKEN_TYPE_ERROR;
            tok->str = REGISTER_NAME_EXPECTED;
            return -1;
        }
        int arg = vocab.findArg(source, here, idx - here);
        if (arg == -1)
        {
            tok->type = TOKEN_TYPE_ERROR;
            tok->str = REGISTER_NAME_EXPECTED;
            return -1;
        }

        return arg;
    }

    Token *getComment()
    {

        Token *tok = new Token(NULL, line, pos);

        int len;
        len = moveTo('\n');
        if (len == -1)
        {
            return Token::eof();
        }
        // append to end of line
        char *comment = (char *)malloc(len + 1); // +1 for the ending \0
        strncpy(comment, &source[idx - len], len);
        comment[len] = '\0';
        tok->str = comment;
        tok->type = TOKEN_TYPE_COMMENT;
        tok->pos -= 1;
        return tok;
    }

    Token *getConstant()
    {
        Token *tok = _getLabel(TOKEN_TYPE_CONST, ':');
        inc();
        if (parseNumber(tok) == -1)
        {
            return Token::error(line, pos, NUMBER_EXPECTED);
        }
        // Now see if in fact the constant was supplied as an option
        // If so, use that value
        Option *opt = getOption(tok->name);
        if(opt != NULL) {
            tok->value = opt->value;
        }

        return tok;
    }

    Token *getVariable()
    {
        Token *tok = _getLabel(TOKEN_TYPE_VAR, ':');
        inc();
        skipSpaces();
        if (source[idx] == '#')
        {
            if (!getSymbolName(tok))
            {
                return Token::error(line, pos, CONSTANT_EXPECTED);
            }
        }
        else if (parseNumber(tok) == -1)
        {
            return Token::error(line, pos, NUMBER_EXPECTED);
        }
        return tok;
    }

    Token *getLabel()
    {
        Token *tok = _getLabel(TOKEN_TYPE_LABEL, ':', false);
        inc();
        return tok;
    }

    Token *getString()
    {
        Token *tok = _getLabel(TOKEN_TYPE_STR, ':');
        inc();
        if (parseString(tok) == -1)
        {
            return Token::error(line, pos, STRING_EXPECTED);
        }
        return tok;
    }

    Token *_getLabel(uint8_t type, char sep, bool typed = true)
    {

        int len;
        len = moveTo(sep);
        if (len == -1)
        {
            return Token::eof();
        }

        if (typed)
            len++;

        Token *tok = new Token(NULL, line, pos - len);
        char *name = (char *)malloc(len + 1);
        strncpy(name, &source[idx - len], len);
        name[len] = '\0';
        tok->name = name;
        tok->type = type;
        return tok;
    }

    /**
     * Fill in the token's value
     * @return 0 on succes, -1 on some sort of failure
     */
    int parseNumber(Token *tok)
    {
        // Allowed formats
        // 93 - decimal
        // 0x3A - hex
        // 0b0110 - binary
        // 'C' - a single character
        char c;
        while (idx < sourceLen)
        {
            c = source[idx];
            switch (c)
            {
            case '\'':
                inc();
                if (idx >= sourceLen)
                {
                    return -1;
                }
                // backslash is allowed
                if (source[idx] == '\\')
                {
                    inc();
                    if (idx >= sourceLen)
                    {
                        return -1;
                    }
                    c = parseEscapedChar(source[idx]);
                }
                break;
            case '0':
                inc();
                if (idx >= sourceLen)
                {
                    return -1;
                }
                if (source[idx] == 'x')
                {
                    inc();
                    return parseHex(tok);
                }

                if (source[idx] == 'b')
                {
                    inc();
                    return parseBin(tok);
                }
                // fallthrough
                dec();
                [[fallthrough]];
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '-':
                return parseDec(tok);

            case ' ':
            case '\t':
                // leading spaces and tabs are allowed
                inc();
                break;

            default:
                return -1;
            }
        }
        return -1;
    }

    char parseEscapedChar(char c)
    {
        switch (c)
        {
        case 'n':
            return '\n';
        case '0':
            return 0;
        case 'b':
            return '\b';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case '\\':
            return '\\';
        case '\'':
            return '\'';
        case '"':
            return '"';
        default:
            return c;
        }
    }

    int parseString(Token *tok)
    {
        int c;
        int len;
        int escapedLen = 0;
        while ((c = getChar()) != -1)
        {
            if (c == '"')
            {
                len = moveTo('"');
                char *str = (char *)malloc(len + 1);
                tok->str = str;
                // Now copy the characters, unescaping as needed
                // The resultant length may be less than allocated
                for (int i = 0; i < len; i++)
                {
                    c = source[idx - len + i];
                    if (c == '\\')
                    {
                        c = parseEscapedChar(source[idx - len + i + 1]);
                        i++;
                    }
                    *str = c;
                    str++;
                    escapedLen++;
                }

                *str = '\0';
                tok->value = escapedLen;
                inc();
                return 0;
            }
            else if (c != ' ')
            {
                return -1;
            }
        }
        return -1;
    }

    /*
     * @return true on success
     * @return false on failure and mutate the Token to an error
     */
    bool parseHex(Token *tok)
    {
        tok->value = 0;
        int c = getChar();

        if (c == -1)
        {
            return hexError(tok);
        }

        int hv = hexVal(c);
        if (hv == -1)
        {
            return hexError(tok);
        }

        tok->value = hv;
        // can be up to 3 more hex chars before the separator
        for (uint8_t i = 0; i < 3; i++)
        {
            c = getChar();
            if (c == -1)
            {
                // EOF is allowed
                return true;
            }
            if (isTerminator(c))
            {
                return true;
            }
            hv = hexVal(c);
            if (hv == -1)
            {
                return hexError(tok);
            }
            tok->value = (tok->value * 16) + hv;
        }
        // next char must be EOF or a valid terminator
        if (idx >= sourceLen)
        {
            return true;
        }
        if (isTerminator(source[idx]))
        {
            return true;
        }

        return hexError(tok);
    }

    bool hexError(Token *tok)
    {
        tok->type = TOKEN_TYPE_ERROR;
        tok->str = HEX_NUMBER_EXPECTED;
        return false;
    }

    int hexVal(char c)
    {
        const char *hexen = "0123456789ABCDEFabcdef";
        const char *p = strchr(hexen, c);
        if (p != NULL)
        {
            uint8_t l = p - hexen;
            if (l > 15)
                l = l - 6;
            return l;
        }
        return -1;
    }

    /*
     * @return true on success
     * @return false on failure and mutate the Token to an error
     */
    bool parseBin(Token *tok)
    {
        tok->value = 0;
        int c = getChar();

        if (c == -1)
        {
            return binError(tok);
        }

        int hv = binVal(c);
        if (hv == -1)
        {
            return binError(tok);
        }

        tok->value = hv;
        // can be up to 15 more bits before the separator
        for (uint8_t i = 0; i < 15; i++)
        {
            c = getChar();
            if (c == -1)
            {
                // EOF is allowed
                return true;
            }
            if (isTerminator(c))
            {
                return true;
            }
            hv = binVal(c);
            if (hv == -1)
            {
                return binError(tok);
            }
            tok->value = (tok->value * 2) + hv;
        }
        // next char must be EOF or a valid terminator
        if (idx >= sourceLen)
        {
            return true;
        }
        if (isTerminator(source[idx]))
        {
            return true;
        }

        return binError(tok);
    }

    bool binError(Token *tok)
    {
        tok->type = TOKEN_TYPE_ERROR;
        tok->str = BINARY_NUMBER_EXPECTED;
        return false;
    }

    int binVal(char c)
    {
        const char *hexen = "01";
        const char *p = strchr(hexen, c);
        if (p != NULL)
        {
            uint8_t l = p - hexen;
            return l;
        }
        return -1;
    }

    /*
     * @return true on success
     * @return false on failure and mutate the Token to an error
     */
    bool parseDec(Token *tok)
    {
        tok->value = 0;
        int c = getChar();
        bool negative = false;

        if (c == '-')
        {
            negative = true;
            c = getChar();
        }

        if (c == -1)
        {
            return decError(tok);
        }

        int hv = decVal(c);
        if (hv == -1)
        {
            return decError(tok);
        }

        tok->value = hv;
        // can be up to 5 more digits before the separator
        for (uint8_t i = 0; i < 5; i++)
        {
            c = getChar();
            if (c == -1)
            {
                // EOF is allowed
                return true;
            }
            if (isTerminator(c))
            {
                if (negative)
                    tok->value = 0 - tok->value;
                return true;
            }
            hv = decVal(c);
            if (hv == -1)
            {
                return decError(tok);
            }
            tok->value = (tok->value * 10) + hv;
        }
        if (negative)
            tok->value = 0 - tok->value;
        // next char must be EOF or a valid terminator
        if (idx >= sourceLen)
        {
            return true;
        }
        if (isTerminator(source[idx]))
        {
            return true;
        }

        return decError(tok);
    }

    bool decError(Token *tok)
    {
        tok->type = TOKEN_TYPE_ERROR;
        tok->str = DECIMAL_NUMBER_EXPECTED;
        return false;
    }

    int decVal(char c)
    {
        const char *decs = "0123456789";
        const char *p = strchr(decs, c);
        if (p != NULL)
        {
            uint8_t l = p - decs;
            return l;
        }
        return -1;
    }

    int getChar()
    {
        if (idx < sourceLen)
        {
            char c = source[idx];
            inc();
            return c;
        }
        return -1;
    }

    /*
     * @return the length of the string
     * 0 if there is no string
     * -1 if the end of the file is reached
     * Leaves the idx pointing at the separator
     * */
    int moveTo(char sep)
    {
        // look forward until the given char, or end of line, or end of file
        char c;
        int len = 0;
        bool found = false;
        while (idx < sourceLen)
        {
            c = source[idx];
            if(c == '\\') {
                found = true;
                inc();
                inc();
                len++;
                len++;
            } else {
                if ((c == sep) || (c == '\n'))
                {
                    break;
                }
                found = true;
                inc();
                len++;
            }
        }

        if (found)
        {
            return len;
        }

        if (idx >= sourceLen)
        {
            return -1;
        }

        return 0;
    }

    /**
     * @return a new Token or NULL if a token with the given name already exists
     */
    Token *append(char *name, int line, int pos)
    {
        if (tokens == NULL)
        {
            // This is the first token
            tokens = new Token(name, line, pos);
            return tokens;
        }

        // Search the list to see if a match already exists
        Token *tok = tokens;
        bool more = true;
        while (more)
        {
            if (tok->isNamed(name))
            {
                printf("Token %s at line %d pos %d redefined at line %d pos %d",
                       name, tok->line, tok->pos, line, pos);
                return NULL;
            }
            if (tok->next != NULL)
            {
                tok = tok->next;
            }
            else
            {
                more = false;
            }
        }

        Token *last = new Token(name, line, pos);
        tok->next = last;
        return last;
    }

    char *source;
    long sourceLen;
    long idx;
    int line;
    int pos;
    int lastPos;
    long addr;
    Token *tokens = NULL;
    Token *currentLabel = NULL;
    Symbol *symbols = NULL;
    Option *options = NULL;
    AssemblyVocabulary vocab;
    bool phase1Error;
    bool phase2Error;
    bool phase3Error;
};
#endif