#ifndef UKMAKER_TOKEN_H
#define UKMAKER_TOKEN_H

#include <stdint.h>
#include <string.h>
#include "../runtime/ForthCPU/ForthIS.h"
#include "Opcodes.h"

#define TOKEN_TYPE_OPCODE 0
#define TOKEN_TYPE_LABEL 1
#define TOKEN_TYPE_CONST 2
#define TOKEN_TYPE_VAR 3
#define TOKEN_TYPE_STR 4
#define TOKEN_TYPE_COMMENT 5
#define TOKEN_TYPE_ERROR 6
#define TOKEN_TYPE_EOF 7
#define TOKEN_TYPE_DIRECTIVE 8 // e.g. ORG for setting start location in memory

#define DIRECTIVE_TYPE_ORG 0
#define DIRECTIVE_TYPE_DATA 1
#define DIRECTIVE_TYPE_ALIAS 2

// String directives to make building Forth words easier
#define DIRECTIVE_TYPE_PLAIN_STRING 3 // Normal string (just bytes, unaligned)
#define DIRECTIVE_TYPE_NWORD_STRING 4 // Normal Forth word, Aligned flags.len string
#define DIRECTIVE_TYPE_RWORD_STRING 5 // Runtime only Forth word, Aligned flags.len string
#define DIRECTIVE_TYPE_IWORD_STRING 6 // Immediate (compile-time only) Forth word, Aligned flags.len string
#define DIRECTIVE_TYPE_XWORD_STRING 7 // Executive Forth word - has both compile-time and runtime behaviours, Aligned flags.len string
#define DIRECTIVE_TYPE_CWORD_STRING 8 // Compile only word - e.g. *IF

/*
Syntax:
#CNAME: 0x33 ; A constant definition. Can be overridden by supplying an option to the assembler
%VNAME: N    ; A variable definition. Reserves N words
$SNAME: "This is a string" ; Stored as a Forth-string <len><str>

.ORG: 0x0000
.ORG: #CONST
.DATA: 23
.DATA: #CONST          ; TOKEN_TYPE_DATA
.SDATA: "Some string"  ; TOKEN_TYPE_STRING_DATA An anonymous string

LABEL: JR NZ,#3 ; Jump with literal
LABEL: JR NZ,%CONST ; Jump with literal value defined as $CONST:
LABEL: JR NZ,$VAR ; Jump to address of VAR. Emits a warning
LABEL: JR NZ,*LABEL ; Relative jump to the label. Error if the destination is too far
*/

class Token {
    public:


    Token(const char *name, int line, int pos) {
        this->name = name;
        this->line = line;
        this->pos = pos;
    }

    ~Token() {}

    static Token *eof() {
        Token *t = new Token(NULL,0,0);
        t->type = TOKEN_TYPE_EOF;
        return t;
    }

    static Token *error(int line, int pos, const char *message) {
        Token *t = new Token(NULL,line,pos);
        t->type = TOKEN_TYPE_ERROR;
        t->str = message;
        return t;
    }

    const char *name = NULL;
    int line = 0;
    int pos = 0;

    uint8_t type = 0;

    uint16_t address = 0;

    /*
    * NULL for a label or instruction; 
    * value of a const 
    * length of a variable in words
    * length of a string
    */
    int value = 0; 
    
    const char *str = NULL;
    const char *strB = NULL;

    // For an instruction, a string or a directive
    Opcode *opcode;
    uint8_t directive = 0;
    uint8_t arga = 0;
    uint8_t argb = 0;
    uint8_t condition = 0;
    uint8_t apply = 0;
    uint8_t invert = 0;
    uint8_t U5 = 0;

    // Instructions with immediate values may need to
    // have the value resolved
    // num3 and num6 values may be taken from a constant definition
    // num16 may be a constant or the address of a variable or label
    bool symbolic = false;

    Token *next = NULL;
    Token *label = NULL;

    bool isConditional() {
        return (condition & 8) != 0;
    }

    bool isConditionNegated() {
         return (condition & 4) != 0;       
    }

    uint8_t getCondition() {
        return condition & 0x03;
    }

    bool isNamed(char *n) {
        return strcasecmp(name, n) == 0;
    }

    bool isCode() {
        return type == TOKEN_TYPE_OPCODE;
    }

    bool isLabel() {
        return type == TOKEN_TYPE_LABEL;
    }

    bool isConst() {
        return type == TOKEN_TYPE_CONST;
    }

    bool isVar() {
        return type == TOKEN_TYPE_VAR;
    }

    bool isStr() {
        return type == TOKEN_TYPE_STR;
    }

    bool isComment() {
        return type == TOKEN_TYPE_COMMENT;
    }

    bool isDirective() {
        return type == TOKEN_TYPE_DIRECTIVE;
    }

    bool isOrg() {
        return isDirective() && (directive == DIRECTIVE_TYPE_ORG);
    }

    bool isData() {
        return isDirective() && (directive == DIRECTIVE_TYPE_DATA);
    }

    bool isStringData() {
        return isDirective() && (
            (directive == DIRECTIVE_TYPE_PLAIN_STRING)
            || (directive == DIRECTIVE_TYPE_NWORD_STRING)
            || (directive == DIRECTIVE_TYPE_RWORD_STRING)
            || (directive == DIRECTIVE_TYPE_IWORD_STRING)
            || (directive == DIRECTIVE_TYPE_XWORD_STRING)
            || (directive == DIRECTIVE_TYPE_CWORD_STRING)
        );
    }

    bool isHeader() {
            return (directive == DIRECTIVE_TYPE_NWORD_STRING)
            || (directive == DIRECTIVE_TYPE_RWORD_STRING)
            || (directive == DIRECTIVE_TYPE_IWORD_STRING)
            || (directive == DIRECTIVE_TYPE_XWORD_STRING)      
            || (directive == DIRECTIVE_TYPE_CWORD_STRING);      
    }

    bool isPStringData() {
        return isDirective() && (directive == DIRECTIVE_TYPE_PLAIN_STRING);
    }

    bool isNStringData() {
        return isDirective() && (directive == DIRECTIVE_TYPE_NWORD_STRING);
    }

    bool isRStringData() {
        return isDirective() && (directive == DIRECTIVE_TYPE_RWORD_STRING);
    }

    bool isIStringData() {
        return isDirective() && (directive == DIRECTIVE_TYPE_IWORD_STRING);
    }

    bool isXStringData() {
        return isDirective() && (directive == DIRECTIVE_TYPE_XWORD_STRING);
    }

    bool isCStringData() {
        return isDirective() && (directive == DIRECTIVE_TYPE_CWORD_STRING);
    }

    bool isAlias() {
        return isDirective() && (directive == DIRECTIVE_TYPE_ALIAS);
    }

    bool isLocation() {
        // anything which is not a comment or another label is a location to which a label can be attached
        return !(isLabel() || isComment());
    }

    bool isError() {
        return type == TOKEN_TYPE_ERROR;
    }

    bool isSymbolic() {
        return symbolic;
    }

    int strlen() {
        if(isStringData()) {
            return ::strlen(str);
        }
        return -1;
    }

    uint16_t opWord() {
        return opcode->getCode();
    }

    uint8_t lowByte() {
        return opWord() & 0xff;
    }

    uint8_t highByte() {
        return (opWord() & 0xff00) >> 8;
    }
};

#endif