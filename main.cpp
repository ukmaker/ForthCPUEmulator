
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "runtime/ForthCPU/ForthVM.h"
#include "runtime/ForthCPU/UnsafeMemory.h"
#include "runtime/ForthCPU/FSerial.h"
#include "runtime/ForthCPU/UART.h"
#include "tools/Assembler.h"
#include "tools/Loader.h"
#include "tools/Dumper.h"
#include "tools/Debugger.h"

#include "tests/Test.h"
#include "tests/VMTests.h"
#include "tests/SlurpTests.h"
#include "tests/RangeTests.h"
#include "tests/LabelTests.h"
#include "tests/LDSTests.h"

#define GENERATE_328P

/*
* core.asm defines
*
* RAMSTART 0x1000
* RAMSIZE  0x3000
*
* So here we define RAM to be sized 0x4000 to hold everything
*/
uint8_t ram[16384];

uint8_t rom[32];

UART uart;

UnsafeMemory mem(ram, 16384, 0, rom, 64, 16384, &uart, 0xffe0, 0xffe7);


ForthVM vm(&mem);
Loader loader(&mem);

Assembler fasm;
Dumper dumper;
Debugger debugger;

TestSuite *testSuite = new TestSuite();
VMTests vmTests(testSuite, &vm, &fasm, &loader);
RangeTests rangeTests(testSuite, &vm, &fasm, &loader);
LabelTests labelTests(testSuite, &vm, &fasm, &loader);
SlurpTests slurpTests(testSuite, &vm, &fasm, &loader);
LDSTests ldsTests(testSuite, &vm, &fasm, &loader);

int tests = 0;
int passed = 0;
int failed = 0;

bool runInstructionTests;
bool runAssemblerTests;
bool verbose;

#define MODE_ATMEGA 0
#define MODE_STM32 1
int mode = MODE_ATMEGA;
bool loaded = false;

char getCommand() {
  int c = getchar();
  return (char)c;
}

void modeAtmega328() {
  fasm.clearOptions();
  fasm.setOption("#RAMSTART", 0x2400); 
  fasm.setOption("#VARSTART", 0x2600); 
  fasm.setOption("#SPTOP", 0x2780);    
  fasm.setOption("#RSTOP", 0x27fe);   
  fasm.setOption("#LBUF_LEN", 80); 
  mode = MODE_ATMEGA; 
  loaded = false;
}

void modeSTM32() {
  fasm.clearOptions();
  fasm.setOption("#RAMSTART", 0x2000); 
  fasm.setOption("#VARSTART", 0x3000); 
  fasm.setOption("#SPTOP", 0x3f80);    
  fasm.setOption("#RSTOP", 0x3ffe); 
  fasm.setOption("#LBUF_LEN", 80); 
  mode = MODE_STM32; 
  loaded = false;
}

void setMode() {
  if(mode == MODE_ATMEGA) 
  { 
    modeAtmega328();
  } else {
    modeSTM32();
  }
}

bool loadFasmFile(const char* filename)
{
  setMode();
  //fasm.slurp("fasm/core.fasm");
  fasm.slurp(filename);
  fasm.pass1();
  fasm.pass2();
  fasm.pass3();

  if(!fasm.hasErrors()) 
  {
    // For development purposes we set the DP to just after the last
    // defined word sine everything is in RAM
    // When deployed to a board DP will point to the start of RAM
    fasm.writeMemory(&mem);
    Symbol *sym = fasm.getSymbol("DICTIONARY_END");
    //vm.ram()->put(10, sym->token->address);
    loaded = true;
  }

  if(fasm.hasErrors()) {
    printf("Assembly errors:\n");
    printf("Phase 1 - %s\n", fasm.phase1Error ? "FAILED" : "OK");
    printf("Phase 2 - %s\n", fasm.phase2Error ? "FAILED" : "OK");
    printf("Phase 3 - %s\n", fasm.phase3Error ? "FAILED" : "OK");
  }
}

bool loadTestCase() {
  return loadFasmFile("tests/current-test-case.fasm");
}

bool loadInnerInterpreter()
{
  return loadFasmFile("fasm/core.fasm");
}

void generateCPP() {
  if(loaded) 
  {
    uint16_t romsize = vm.read(10);

    Symbol *sym = fasm.getSymbol("#RAMSTART");
    vm.ram()->put(10, sym->token->value);

    switch(mode) {
      case MODE_ATMEGA:
        dumper.writeCPP("ForthImage_ATMEGA328.h", &fasm, &mem, 0, romsize, true);
        break;

      case MODE_STM32:
        dumper.writeCPP("ForthImage_STM32.h", &fasm, &mem, 0, romsize, true); 
        break;

      default: break;
    }

    sym = fasm.getSymbol("DICTIONARY_END");
    vm.ram()->put(10, sym->token->address);

  }
  else
  {
    Serial.println("Image not loaded. Did you change the target?");
  }
}

void generateMemFile() {
  if(loaded) 
  {
    uint16_t romsize = 10000; //vm.read(10);
    uint16_t ramstart;
    uint16_t patch;

    Symbol *sym = fasm.getSymbol("#RAMSTART");
    ramstart = sym->token->value;
    sym = fasm.getSymbol("CORE_PATCH_DP");
    patch = sym->token->address;

    vm.ram()->put(patch, ramstart);

    dumper.writeMEM("ForthImage.mem", &fasm, &mem, 0, romsize, true);
    // restore the current DP
    sym = fasm.getSymbol("DICTIONARY_END");
    vm.ram()->put(patch, sym->token->address);

  }
  else
  {
    Serial.println("Image not loaded. Did you change the target?");
  }
}

void signalHandler(int signum) {
  debugger.setBreak();
}

void prompt() {
  if(mode == MODE_ATMEGA) 
  { 
    Serial.print("ATMEGA >>"); 
  } 
  else 
  { 
    Serial.print("STM32 >>"); 
  }
}

bool commandLine() {
  char c;
  while((char)(c = getCommand()) == '\n') {
    prompt();
  }
  switch(c) {
    case 'A': modeAtmega328(); break;
    case 'G': generateCPP(); break;
    case 'S': modeSTM32(); break; 
    case 'M': generateMemFile(); break;
    case 'c': vm.reset(); vm.run(); prompt(); break;
    case 'w': vm.warm();  vm.run(); prompt(); break;
    case 'l': loadInnerInterpreter(); break;
    case 't': loadTestCase(); break;
    case 'e': return false;
    case 'd': vm.warm();  while(debugger.commandLine()); prompt(); break;
    case 'D': dumper.dump(&fasm); break;
    case '?': 
      Serial.println("Commands:");
      Serial.println("A - Set mode ATMEGA328");
      Serial.println("S - Set mode STM32");
      Serial.println("G - Generate ForthImage_xxx.h");
      Serial.println("M - Generate ROM *.mem file");
      Serial.println("l - (Re)Load the minimal image");
      Serial.println("t - (Re)Load the current test case");
      Serial.println("c - Cold start the Forth VM");
      Serial.println("w - Warm start the Forth VM");
      Serial.println("d - Run the debugger");
      Serial.println("e - Exit");
      break;
    default: break;
  }
  return true;
}

int main(int argc, char **argv)
{

  vmTests.run();
  rangeTests.run();
  labelTests.run();
  slurpTests.run();
  ldsTests.run();
/**
  struct sigaction handler;
  handler.sa_handler = signalHandler;
  sigemptyset(&handler.sa_mask);
  handler.sa_flags = 0;
  sigaction(SIGINT, &handler, NULL);
**/

//signal(SIGABRT, &signalHandler);
//signal(SIGINT, &signalHandler);

  debugger.setAssembler(&fasm);
  debugger.setVM(&vm);
  debugger.reset();

  // ram.setWatch(0x5d4);
  debugger.setShowForthWordsOnly();
  debugger.setVerbose(true);
  // debugger.setBump(10);
  // debugger.writeProtect("DICTIONARY_END");
  //debugger.run();

  prompt();
 
      while(commandLine()) ;
  

  return 0;
}