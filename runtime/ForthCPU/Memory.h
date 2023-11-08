#ifndef UKMAKER_MEMORY_H
#define UKMAKER_MEMORY_H
#include "FArduino.h"
#include "UART.h"
/**
 * Abstractts two blocks of memory - ROM and RAM
 * In an MCU ROM will be implemented in Flash
*/

#define MEMORY_RANGE_UNKNOWN 0
#define MEMORY_RANGE_RAM     1
#define MEMORY_RANGE_ROM     2
#define MEMORY_RANGE_UART    3

class Memory {

    public:

    Memory(uint8_t *ram, size_t ramSize, uint16_t ramStart,
        const uint8_t *rom, size_t romSize, uint16_t romStart,
        UART *uart, uint16_t uartStart, uint16_t uartEnd) :
        _ram(ram), _ramSize(ramSize), _ramStart(ramStart),
        _rom(rom), _romSize(romSize), _romStart(romStart),
        _uart(uart), _uartStart(uartStart), _uartEnd(uartEnd)
         {
            _ramEnd = _ramStart + _ramSize;
            _romEnd = _romStart + _romSize;
         }

    ~Memory() {}

    virtual void put(uint16_t addr, uint16_t w)=0;

    virtual uint16_t get(uint16_t addr)=0;

    virtual void putC(uint16_t addr, uint8_t c)=0;

    virtual uint8_t getC(uint16_t addr)=0;

    virtual void clk()=0;

    uint8_t addressDecode(uint16_t addr) {
        if(addr >= _ramStart  && addr <= _ramEnd -1)  return MEMORY_RANGE_RAM;
        if(addr >= _romStart  && addr <= _romEnd -1)  return MEMORY_RANGE_ROM;
        if(addr >= _uartStart && addr <= _uartEnd -1) return MEMORY_RANGE_UART;
        return MEMORY_RANGE_UNKNOWN;
    }

    uint8_t addressDecodeC(uint16_t addr) {
        if(addr >= _ramStart  && addr <= _ramEnd)  return MEMORY_RANGE_RAM;
        if(addr >= _romStart  && addr <= _romEnd)  return MEMORY_RANGE_ROM;
        if(addr >= _uartStart && addr <= _uartEnd) return MEMORY_RANGE_UART;
        return MEMORY_RANGE_UNKNOWN;
    }

    protected:

    uint8_t *_ram;
    uint16_t _ramSize;
    uint16_t _ramStart;
    uint16_t _ramEnd;

    const uint8_t *_rom;
    uint16_t _romSize;
    uint16_t _romStart;
    uint16_t _romEnd;

    UART *_uart;
    uint16_t _uartStart;
    uint16_t _uartEnd;

};
#endif