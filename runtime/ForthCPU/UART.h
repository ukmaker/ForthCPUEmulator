#ifndef UKMAKER_UART_H
#define UKMAKER_UART_H

#include "FSerial.h"
#include <stdint.h>

/******************************************************
 * Emulate the UART in the FPGA
 *
 * Addr  -  Read             Write
 *   0   -  Status           Any write clears TXI
 *   2   -  Rx Data          Tx Data
 *   4   -  RX CLK DIV       RX CLK DIV
 *   6   -  TX CLK DIV       TX CLK DIV
 * 
 * Status bits
 *   0   - Rx Data available
 *   1   - Tx Active
 *   2   - Tx Done
 *   3   - TXInt          Cleared on write to Status reg
 *   4   - RXInt          Cleared on read from Rx Data reg
*/

#define UART_STATUS_RX_DATA_AVAILABLE 1
#define UART_STATUS_TX_ACTIVE         2
#define UART_STATUS_TX_DONE           4
#define UART_STATUS_TX_INT            8
#define UART_STATUS_RX_INT           16

class UART {

    public:
    UART() {}
    ~UART() {}

    void tick() {
        if(_txBits > 0) {
            _txTicks--;
            if(_txTicks == 0) {
                _txBits--;
                if(_txBits > 0) {
                    _txTicks = _txClkDiv;
                    _txActive = true;
                } else {
                    _txInt = true;
                    _txDone = true;
                    _txActive = false;
                }
            }
        } else {
            _txActive = false;
        }

        bool now = Serial.available();

        if(now && now != _then) {
            // Rising edge of available
            _rxInt = true;
            _rxAvailable = true;
        }

        _then = now;
    }

    void write(uint16_t addr, uint16_t data) {
        switch(addr) {
            case 0: writeStatus(data); break;
            case 2: writeTxData(data); break;
            case 3: writeRxClkDiv(data); break;
            case 4: writeTxClkDiv(data); break;
            default: break;
        }
    }

    void writeStatus(uint16_t data) {
        _txInt = false;
    }

    void writeTxData(uint16_t data) {
        Serial.putc((char)data);
        _txDone   = false;
        _txActive = true;
        _txTicks  = _txClkDiv;
        _txBits   = 10;
    }

    void writeRxClkDiv(uint16_t data) {
        _rxClkDiv = data;
    }

    void writeTxClkDiv(uint16_t data) {
       _txClkDiv = data;
    }

   uint16_t read(uint16_t addr) {
        switch(addr) {
            case 0: return readStatus(); break;
            case 2: return readRxData(); break;
            case 3: return readRxClkDiv(); break;
            case 4: return readTxClkDiv(); break;
            default: break;
        }
    }

    uint16_t readStatus() {
        uint16_t status = 0;
        status |= _txActive    ? UART_STATUS_TX_ACTIVE : 0;
        status |= _txDone      ? UART_STATUS_TX_DONE : 0;
        status |= _txInt       ? UART_STATUS_TX_INT : 0;
        status |= _rxInt       ? UART_STATUS_RX_INT : 0;
        status |= _rxAvailable ? UART_STATUS_RX_DATA_AVAILABLE : 0;
        return status;
    }

    uint16_t readRxData() {
        if(Serial.available()) return Serial.getc();
    }

    uint16_t readRxClkDiv() {
        return _rxClkDiv;
    }

    uint16_t readTxClkDiv() {
        return _txClkDiv;
    }

    protected:

    bool _txInt = false;
    bool _rxInt = false;
    bool _txDone = true;
    bool _txActive = false;
    bool _rxAvailable = false;

    bool _then = false;

    uint16_t _rxClkDiv = 104;
    uint16_t _txClkDiv = 104;

    uint32_t _txTicks = 0;
    uint32_t _txBits  = 0;

};
#endif