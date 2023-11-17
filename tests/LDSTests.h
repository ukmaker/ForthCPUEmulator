#ifndef UKMAKER_LDSTESTS_H
#define UKMAKER_LDSTESTS_H

#include "Test.h"

class LDSTests : public Test
{

public:

    LDSTests(TestSuite *suite, ForthVM *fvm, Assembler *vmasm, Loader *loader) : Test(suite, fvm, vmasm, loader) {}
    
    void run()
    {
        shouldOpenTestLDS();
        shouldGetOpeningComment();
        shouldGetLD();
        shouldGetLDI();
        shouldGetPOP();
        shouldGetPOPR();
        shouldGetLDXRL();
        shouldGetLDXFP();
        shouldGetLDXSP();
        shouldGetLDXRS();
    }

    Token *getOpcode() {
        Token *tok = fasm->getToken();
        assert(tok->type != TOKEN_TYPE_ERROR, "Should tokenize an opcode");
        assertEquals(tok->type, TOKEN_TYPE_OPCODE, "Type should be OPCODE");
        return tok;
    }

    void shouldOpenTestLDS()
    {
        shouldOpenAsmFile("tests/test-lds.fasm", 136);
    }
    void shouldGetOpeningComment()
    {
        printf("         shouldGetOpeningComment\n");
        Token *tok = fasm->getToken();
        assert(tok->type != TOKEN_TYPE_ERROR, "Should tokenize a standalone comment");
        assertEquals(tok->type, TOKEN_TYPE_COMMENT, "Type should be COMMENT");
        assertEquals(tok->line, 1, "First comment is on line 1");
        assertEquals(tok->pos, 1, "First comment is on pos 1");
    }

    void shouldGetLD()
    {
        printf("         shouldGetLD\n");
        Token *tok = getOpcode();
        assertEquals(tok->opcode->getLDSOp(), LDS_OP_LD, "Should be a LD instruction");
    }

    void shouldGetLDI()
    {
        printf("         shouldGetLDI\n");
        Token *tok = getOpcode();
        assertEquals(tok->opcode->getLDSOp(), LDS_OP_LD, "Should be an LDI instruction");
    }
    void shouldGetPOP()
    {
        printf("         shouldGetPOP\n");
        Token *tok = getOpcode();
        assertEquals(tok->opcode->getLDSOp(), LDS_OP_LD, "Should be a POP instruction");
    }
    void shouldGetPOPR()
    {
        printf("         shouldGetPOPR\n");
        Token *tok = getOpcode();
        assertEquals(tok->opcode->getLDSOp(), LDS_OP_LD, "Should be a POPR instruction");
    }
    void shouldGetLDXRL()
    {
        printf("         shouldGetLDXRL\n");
        Token *tok = getOpcode();
        assertEquals(tok->opcode->getLDSOp(), LDS_OP_LD, "Should be an LDX instruction");
    }
    void shouldGetLDXFP()
    {
        printf("         shouldGetLDXFP\n");
        Token *tok = getOpcode();
        assertEquals(tok->opcode->getLDSOp(), LDS_OP_LD, "Should be an LDX instruction");
    }
    void shouldGetLDXSP()
    {
        printf("         shouldGetLDXSP\n");
        Token *tok = getOpcode();
        assertEquals(tok->opcode->getLDSOp(), LDS_OP_LD, "Should be an LDX instruction");
    }
    void shouldGetLDXRS()
    {
        printf("         shouldGetLDXRS\n");
        Token *tok = getOpcode();
        assertEquals(tok->opcode->getLDSOp(), LDS_OP_LD, "Should be an LDX instruction");
    }
};
#endif