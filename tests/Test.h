#ifndef UKMAKER_TEST_H
#define UKMAKER_TEST_H

#include "TestSuite.h"
#include "../runtime/ArduForth/ForthVM.h"
#include "../tools/Assembler.h"
#include "../tools/Loader.h"

class Test {

    public:

    TestSuite *testSuite;
    ForthVM *vm;
    Assembler *fasm;
    Loader *loader;

    Test(TestSuite *suite, ForthVM *fvm, Assembler *vmasm, Loader *vmloader) {
        testSuite = suite;
        vm = fvm;
        fasm = vmasm;
        loader = vmloader;
    }

    ~Test() {}

    virtual void run()=0;

    void shouldOpenAsmFile(const char *file, uint32_t size) {
        printf("         shouldOpenAsmFile\n");
        assert(fasm->slurp(file), "Failed to open file ", file);
        assertEquals(fasm->fileSize(), size, "Wrong file size");
    }


    void shouldGetAnOpcode(const char *name, uint8_t opcode) {
        printf("         shouldGet%s\n", name);
        Token *tok = fasm->getToken();
        assert(tok->type != TOKEN_TYPE_ERROR, "Should tokenize an opcode");
        assertEquals(tok->type, TOKEN_TYPE_OPCODE, "Type should be OPCODE");
        assertEquals(tok->opcode, opcode, "Should be an instruction");
    }

    void shouldGetALabel(const char *name) {
        printf("         shouldGetLabel %s\n", name);
        Token *tok = fasm->getToken();
        assert(tok->type != TOKEN_TYPE_ERROR, "Should tokenize a line label");
        assertEquals(tok->type, TOKEN_TYPE_LABEL, "Type should be LABEL");
        assertString(tok->name, name, "Should parse label");
    }

   void shouldGetAConstant(const char *name, uint16_t value) {
        printf("         shouldGetConstant %s\n", name);
        Token *tok = fasm->getToken();
        assertEquals(tok->type, TOKEN_TYPE_CONST, "Type should be CONST");
        assertString(tok->name, name, "Name");
        assertEquals(tok->value, value, "Value");
    }

   void shouldGetAnAlias(uint8_t reg, const char *alias) {
        printf("         shouldGetAlias %s\n", alias);
        Token *tok = fasm->getToken();
        assertEquals(tok->type, TOKEN_TYPE_DIRECTIVE, "Type should be DIRECTIVE");
        assertEquals(tok->opcode, DIRECTIVE_TYPE_ALIAS, "Should be an ALIAS");
        assertEquals(tok->arga, reg, "Register");
        assertString(tok->str, alias, "Alias");
    }


    void skipTokens(int n) {
        for(int i=0; i<n; i++) {
            fasm->getToken();
        }
    }
    
    bool should(bool passedTest) {
        testSuite->tests++;
        if(passedTest) {
            testSuite->passed++;
        } else {
            testSuite->failed++;
        }
        return passedTest;
    }

    void assert(bool t, const char *m) {
        if(should(t)) {
            printf("[OK    ]");
        } else {
            printf("[FAILED]");
        }

        printf(" %s - \n", m);
    }

    void assert(bool t, const char *m, const char *m1) {
        if(should(t)) {
            printf("[OK    ]");
        } else {
            printf("[FAILED]");
        }

        printf(" %s %s - \n", m, m1);
    }

    void assertEquals(uint16_t a, uint16_t b, const char *m) {

        if(should(a == b)) {
            printf("[OK    ]");
            printf(" [%d == %d] %s\n", a, b, m);
        } else {
            printf("[FAILED]");
            printf(" [%d != %d] %s\n", a, b, m);
        }
    }

    void assertString(const char *a, const char *b, const char *m) {
        if(a == NULL && b == NULL) {
            printf("[OK    ] %s\n", m);
            should(true);
        } else if(a == NULL) {
            printf("[FAILED]");
            printf(" [NULL != %s] %s\n", b, m);
            should(false);
        } else if(b == NULL) {
            printf("[FAILED]");
            printf(" [%s != NULL] %s\n", a, m);
            should(false);
        } else if(strcmp(a,b) != 0) {
            printf("[FAILED]");
            printf(" [%s != %s] %s\n", a, b, m);
            should(false);
        } else {
            printf("[OK    ]");
            printf(" [%s == %s] %s\n", a, b, m);
            should(true);
        }
    }


};
#endif