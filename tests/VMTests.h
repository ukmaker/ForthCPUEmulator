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
    shouldPrintC();
    shouldLDBI();
    shouldAddI();
    shouldCMP();
    printf("==============================\n");
    printf("TOTAL: %d  PASSED %d  FAILED %d\n", testSuite->tests, testSuite->passed, testSuite->failed);
    printf("==============================\n");
}


void shouldHalt() {

    vm->ram()->put(0, OP_HALT << OP_BITS);
    vm->reset();
    vm->step();
    assert(vm->halted(), "VM should be halted");

}

void shouldAdd() {
    loader->reset();
    loader->load(0,0,OP_MOVAI, 3);
    loader->load(0,0,OP_MOVBI, 7);
    loader->load(0,0,OP_ADD, REG_A, REG_B);
    loader->load(0,0,OP_HALT,0);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    vm->step();
    assert(vm->get(REG_A) == 10, "Should add");    
}

void shouldGenerateCarry() {
    loader->reset();
    loader->load(0,0,OP_MOVIL, REG_0,0);
    loader->load(0x8000);
    loader->load(0,0,OP_MOVIL, REG_1,0);
    loader->load(0x8000);
    loader->load(0,0,OP_ADD, REG_0, REG_1);
    loader->load(0,0,OP_HALT,0);
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
    loader->load(0,0,OP_MOVAI, 3);
    loader->load(0,0,OP_MOVBI, 7);
    loader->load(0,0,OP_SUB, REG_A, REG_B);
    loader->load(0,0,OP_HALT,0);
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
    loader->load(0,0,OP_MOVIL, REG_A, 0);
    loader->load(0xaaaa);
    loader->load(0,0,OP_MOVIL, REG_B, 0);
    loader->load(0x1111);
    loader->load(0,0,OP_SUB, REG_A, REG_B);
    loader->load(0,0,OP_HALT,0);
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
    loader->load(0,0,OP_MOVIL, REG_A, 0);
    loader->load(0x1111);
    loader->load(0,0,OP_MOVIL, REG_B, 0);
    loader->load(4);
    loader->load(0,0,OP_MUL, REG_A, REG_B);
    loader->load(0,0,OP_HALT,0);
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
    loader->load(0,0,OP_MOVIL, REG_A, 0);
    loader->load(0x1111);
    loader->load(0,0,OP_MOVIL, REG_B, 0);
    loader->load(0x1010);
    loader->load(0,0,OP_AND, REG_A, REG_B);
    loader->load(0,0,OP_HALT,0);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    vm->step();
    assertEquals(vm->get(REG_A), 0x1010, "AND result"); 
    assert(!vm->getC(), "AND should not set carry");   
}

void shouldPrintC() {
    vm->addSyscall(0,VMTests_printC);
    loader->reset();
    loader->load(0,0,OP_MOVIL, REG_SP, 0);
    loader->load((uint16_t)0);
    loader->load(0,0,OP_MOVIL, REG_A, 0);
    loader->load('F');
    loader->load(0,0,OP_PUSHD, REG_A, 0);
    loader->load(0,0,OP_SYSCALL, 0);
    loader->load(0,0,OP_HALT,0);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    vm->step(); 
    vm->step(); 

    assertEquals(vm->get(REG_SP), 0, "SP should be zero after printC");
}

void shouldLDBI() {
    loader->reset();
    loader->load(0,0,OP_MOVBI, 5);
    vm->reset();
    vm->step();

    assertEquals(vm->get(REG_B), 5, "B should be 5");
}

void shouldAddI() {
    loader->reset();
    loader->load(0,0,OP_MOVBI, 5);
    loader->load(0,0,OP_ADDI, REG_B, -2);
    vm->reset();
    vm->step();
    vm->step();

    assertEquals(vm->get(REG_B), 3, "B should be 3 after add");
}

void shouldCMP() {
    loader->reset();
    loader->load(0,0,OP_MOVAI, 0);
    loader->load(0,0,OP_MOVBI, 0);
    loader->load(0,0,OP_CMP, REG_A, REG_B);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    assertEquals(vm->getC(),0, "No carry on equals");    
    assertEquals(vm->getZ(),1, "Z set on equals");    
    assertEquals(vm->getO(),0, "No signed-carry on equals");   

    loader->reset();
    loader->load(0,0,OP_MOVAI, 1);
    loader->load(0,0,OP_MOVBI, 0);
    loader->load(0,0,OP_CMP, REG_A, REG_B);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    assertEquals(vm->getC(),0, "No carry on >");    
    assertEquals(vm->getZ(),0, "Z not set on >");    
    assertEquals(vm->getO(),0, "No signed-carry on >");   


    loader->reset();
    loader->load(0,0,OP_MOVAI, 0);
    loader->load(0,0,OP_MOVBI, 1);
    loader->load(0,0,OP_CMP, REG_A, REG_B);
    vm->reset();
    vm->step();
    vm->step();
    vm->step();
    assertEquals(vm->getC(),1, "Carry on <");    
    assertEquals(vm->getZ(),0, "Z not set on <");    
    assertEquals(vm->getO(),1, "Signed-carry on <");   


    loader->reset();
    loader->load(0,0,OP_MOVAI, 1);
    loader->load(0,0,OP_MOVBI, -1);
    loader->load(0,0,OP_CMP, REG_A, REG_B);
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