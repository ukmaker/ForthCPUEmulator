#ifndef UKMAKER_VMTEST_H
#define UKMAKER_VMTESTS_H

#include "Test.h"

void VMTests_printC(ForthVM *vm) {
    // Syscall to print the char on the top of the stack
    // ( c - )
    uint16_t v = vm->pop();
    char c = (char) v;

    printf("%c", c);
}

class VMTests : public Test {

    public:
    VMTests(TestSuite *suite, ForthVM *fvm, Assembler *vmasm, Loader *loader) : Test(suite, fvm, vmasm, loader) {}

    void run() {

    testSuite->reset();

    shouldHalt();
    shouldAdd();
    shouldGenerateCarry();
    shouldSub();
    shouldSubL();
    shouldMulL();
    shouldAnd();
    shouldLDBI();
    shouldAddI();
    shouldCMP();
    printf("==============================\n");
    printf("TOTAL: %d  PASSED %d  FAILED %d\n", testSuite->tests, testSuite->passed, testSuite->failed);
    printf("==============================\n");
}


void shouldHalt() {

    vm->ram()->put(0, GEN_OP_HALT << GEN_OP_BITS_POS);
    vm->reset();
    vm->step();
    assert(vm->halted(), "VM should be halted");

}

void shouldAdd() {
    loader->reset();
    loader->loadALU(ALU_OP_MOV, ALU_MODE_REG_REG, REG_A,  3);
    loader->loadALU(ALU_OP_MOV, ALU_MODE_REG_REG, REG_B,  7);
    loader->loadALU(ALU_OP_ADD, ALU_MODE_REG_REG, REG_A, REG_B);
    loader->loadGen(GEN_OP_HALT);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    vm->step();
    assert(vm->get(REG_A) == 10, "Should add");    
}

void shouldGenerateCarry() {
    loader->reset();
    loader->loadLDS(LDS_OP_LD, LDS_MODE_REG_HERE, REG_0,0);
    loader->load(0x8000);
    loader->loadLDS(LDS_OP_LD, LDS_MODE_REG_HERE, REG_1,0);
    loader->load(0x8000);
    loader->loadALU(ALU_OP_ADD, ALU_MODE_REG_REG, REG_0, REG_1);
    loader->loadGen(GEN_OP_HALT);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    vm->step();
    assert(vm->get(REG_0) == 0, "Should add");    
    assert(vm->getC(), "Should generate carry");    
}

void shouldSub() {
    loader->reset();
    loader->loadALU(ALU_OP_MOV, ALU_MODE_REG_REG, REG_A,  3);
    loader->loadALU(ALU_OP_MOV, ALU_MODE_REG_REG, REG_B,  7);
    loader->loadALU(ALU_OP_SUB, ALU_MODE_REG_REG, REG_A, REG_B);
    loader->loadGen(GEN_OP_HALT);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    vm->step();
    assertEquals(vm->get(REG_A), -4, "SUB result"); 
    assert(vm->getC(), "SUB should set carry");   
}

void shouldSubL() {
    loader->reset();
    loader->loadLDS(LDS_OP_LD, LDS_MODE_REG_HERE, REG_A, 0xaaaa);
    loader->loadLDS(LDS_OP_LD, LDS_MODE_REG_HERE, REG_B, 0x1111);
    loader->loadALU(ALU_OP_SUB, ALU_MODE_REG_REG, REG_A, REG_B);
    loader->loadGen(GEN_OP_HALT);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    vm->step();
    assertEquals(vm->get(REG_A), 0xaaaa - 0x1111, "SUBL result"); 
    assert(!vm->getC(), "SUB should not set carry");   
}

void shouldMulL() {
    loader->reset();
    loader->loadLDS(LDS_OP_LD, LDS_MODE_REG_HERE, REG_A, 0);
    loader->load(0x1111);
    loader->loadLDS(LDS_OP_LD, LDS_MODE_REG_HERE, REG_B, 0);
    loader->load(4);
    loader->loadALU(ALU_OP_MUL, ALU_MODE_REG_REG, REG_A, REG_B);
    loader->loadGen(GEN_OP_HALT);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    vm->step();
    assertEquals(vm->get(REG_A), 0x4444, "MUL result"); 
    assert(!vm->getC(), "MUL should not set carry");   
}

void shouldAnd() {
    loader->reset();
    loader->loadLDS(LDS_OP_LD, LDS_MODE_REG_HERE, REG_A, 0);
    loader->load(0x1111);
    loader->loadLDS(LDS_OP_LD, LDS_MODE_REG_HERE, REG_B, 0);
    loader->load(0x1010);
    loader->loadALU(ALU_OP_AND, ALU_MODE_REG_REG, REG_A, REG_B);
    loader->loadGen(GEN_OP_HALT);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    vm->step();
    assertEquals(vm->get(REG_A), 0x1010, "AND result"); 
    assert(!vm->getC(), "AND should not set carry");   
}


void shouldLDBI() {
    loader->reset();
    loader->loadALU(ALU_OP_MOV, ALU_MODE_REG_REG, REG_B,  5);
    vm->reset();
    vm->step();

    assertEquals(vm->get(REG_B), 5, "B should be 5");
}

void shouldAddI() {
    loader->reset();
    loader->loadALU(ALU_OP_MOV, ALU_MODE_REGA_S8, 5);
    loader->loadALU(ALU_OP_ADD, ALU_MODE_REGA_S8, -2);
    vm->reset();
    vm->step();
    vm->step();

    assertEquals(vm->get(REG_A), 3, "A should be 3 after add");
}

void shouldCMP() {
    loader->reset();
    loader->loadALU(ALU_OP_MOV, ALU_MODE_REG_REG, REG_A,  0);
    loader->loadALU(ALU_OP_MOV, ALU_MODE_REG_REG, REG_B,  0);
    loader->loadALU(ALU_OP_CMP, REG_A, REG_B);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    assertEquals(vm->getC(),0, "No carry on equals");    
    assertEquals(vm->getZ(),1, "Z set on equals");    
    assertEquals(vm->getO(),0, "No signed-carry on equals");   

    loader->reset();
    loader->loadALU(ALU_OP_MOV, ALU_MODE_REG_REG, REG_A,  1);
    loader->loadALU(ALU_OP_MOV, ALU_MODE_REG_REG, REG_B,  0);
    loader->loadALU(ALU_OP_CMP, REG_A, REG_B);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    assertEquals(vm->getC(),0, "No carry on >");    
    assertEquals(vm->getZ(),0, "Z not set on >");    
    assertEquals(vm->getO(),0, "No signed-carry on >");   


    loader->reset();
    loader->loadALU(ALU_OP_MOV, ALU_MODE_REG_REG, REG_A,  0);
    loader->loadALU(ALU_OP_MOV, ALU_MODE_REG_REG, REG_B,  1);
    loader->loadALU(ALU_OP_CMP, REG_A, REG_B);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    assertEquals(vm->getC(),1, "Carry on <");    
    assertEquals(vm->getZ(),0, "Z not set on <");    
    assertEquals(vm->getO(),1, "Signed-carry on <");   


    loader->reset();
    loader->loadALU(ALU_OP_MOV, ALU_MODE_REG_REG, REG_A,  1);
    loader->loadALU(ALU_OP_MOV, ALU_MODE_REG_REG, REG_B,  -1);
    loader->loadALU(ALU_OP_CMP, REG_A, REG_B);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    assertEquals(vm->getC(),1, "Carry on <");    
    assertEquals(vm->getZ(),0, "Z not set on <");    
    assertEquals(vm->getO(),0, "No signed-carry on <");   
}

};
#endif