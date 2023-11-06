#ifndef UKMAKER_UNSAFE_MEMORY_H
#define UKMAKER_UNSAFE_MEMORY_H

#include "Memory.h"

class UnsafeMemory : public Memory {

public:

    UnsafeMemory(uint8_t *ram, size_t ramSize, uint16_t ramStart,
        const uint8_t *rom, size_t romSize, uint16_t romStart,
        UART *uart, uint16_t uartStart, uint16_t uartEnd) :
        Memory(ram, ramSize, ramStart, rom, romSize, romStart, uart, uartStart, uartEnd) {}

    ~UnsafeMemory() {}

    void put(uint16_t addr, uint16_t w) {
        switch(addressDecode(addr)) {
            case MEMORY_RANGE_RAM:
                writeRAM(addr, w);
            break;

            case MEMORY_RANGE_ROM:
                writeROM(addr, w);
            break;

            case MEMORY_RANGE_UART:
                writeUART(addr, w);
            break;

            default:
                _writeUndefined(addr, w);
            break;
        }
    }

    void putC(uint16_t addr, uint8_t c) {
        switch(addressDecodeC(addr)) {
            case MEMORY_RANGE_RAM:
                writeCRAM(addr, c);
            break;

            case MEMORY_RANGE_UART:
                writeCUART(addr, c);
            break;

            default:
                _writeUndefined(addr, c);
            break;
        }
    }

    void writeRAM(uint16_t addr, uint16_t data) {
        *(uint16_t *)(_ram + addr - _ramStart) = data;
    }

    void writeCRAM(uint16_t addr, uint8_t data) {
        *(_ram + addr - _ramStart) = data;
    }

    void writeROM(uint16_t addr, uint16_t data) {

    }

    void writeUART(uint16_t addr, uint16_t data) {
       _uart->write(addr - _uartStart, data);
    }

    void writeCUART(uint16_t addr, uint8_t data) {
       _uart->write(addr - _uartStart, data);
    }

    void _writeUndefined(uint16_t addr, uint16_t data) {

    }

    uint16_t get(uint16_t addr) {
        switch(addressDecode(addr)) {
            case MEMORY_RANGE_RAM:
                return readRAM(addr);
            break;

            case MEMORY_RANGE_ROM:
                return readROM(addr);
            break;

            case MEMORY_RANGE_UART:
                return readUART(addr);
            break;

            default:
                return readUndefined(addr);
            break;
        }
    }

    uint8_t getC(uint16_t addr) {
        switch(addressDecodeC(addr)) {
            case MEMORY_RANGE_RAM:
                return readCRAM(addr);
            break;

            case MEMORY_RANGE_UART:
                return readCUART(addr);
            break;

            case MEMORY_RANGE_ROM:
                return readCROM(addr);
            break;

            default:
                return readUndefined(addr);
            break;
        }
    }

    uint8_t readCRAM(uint16_t addr) {
        return *(_ram + addr - _ramStart);
    }

    uint8_t readCUART(uint16_t addr) {
        return _uart->read(addr - _uartStart);
    }

    uint8_t readCROM(uint16_t addr) {
        return *(_rom + addr - _romStart);
    }

    uint16_t readRAM(uint16_t addr) {
        return *(uint16_t *)(_ram + addr - _ramStart);
    }

    uint16_t readROM(uint16_t addr) {
        return *(uint16_t *)(_rom + addr - _romStart);
    }

    uint16_t readUART(uint16_t addr) {
        return _uart->read(addr - _uartStart);
    }

    uint16_t readUndefined(uint16_t addr) {
        return 0;
    }
};
#endif