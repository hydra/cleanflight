/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "platform.h"

#include "build_config.h"

#include "drivers/system.h"

#include "drivers/serial.h"
#include "drivers/serial_uart.h"
#include "io/serial.h"

#include "rx/rx.h"
#include "rx/exbus.h"

// driver for Jeti EX-Bus receiver using UART2

#define EXBUS_BAUD_LOW       125000
#define EXBUS_BAUD_HIGH      250000
#define EXBUS_FRAME_MINLEN   7
#define EXBUS_FRAME_MAXLEN   70
#define EXBUS_NUM_CHANNELS   16

#define EXBUS_BYTE_BEGIN  0x3E
#define EXBUS_BYTE_RC      0x31

static bool exbusFrameDone = false;
static uint8_t exbusBuf[EXBUS_FRAME_MAXLEN] = { 0, };
static uint16_t exbusJunkChars = 0;
static bool exbusBaudValid = false;
static serialPort_t *exbusPort;

static void exbusDataReceive(uint16_t c);
static uint16_t exbusReadRawRC(rxRuntimeConfig_t *rxRuntimeConfig, uint8_t chan);


static uint16_t exbusCrcUpdate(uint16_t crc, uint8_t data)
{
    data ^= (uint8_t)crc & (uint8_t)0xFF;
    data ^= data << 4;
    crc = ((((uint16_t)data << 8) | ((crc & 0xFF00) >> 8))
            ^ (uint8_t)(data >> 4) ^ ((uint16_t)data << 3));

    return crc;
}

// Calculates CRC16-CCIT as required by Jeti EX Bus protocol
static uint16_t exbusCrcGenerate(uint8_t *data, uint8_t len)
{
    uint16_t crc16 = 0;

    while(len--) {
        crc16 = exbusCrcUpdate(crc16, data[0]);
        data++;
    }

    return crc16;
}

void exbusUpdateSerialRxFunctionConstraint(functionConstraint_t *functionConstraint)
{
    functionConstraint->minBaudRate = EXBUS_BAUD_LOW;
    functionConstraint->maxBaudRate = EXBUS_BAUD_HIGH;
    functionConstraint->requiredSerialPortFeatures = SPF_SUPPORTS_CALLBACK;
    // TODO: half duplex
}

bool exbusInit(rxConfig_t *rxConfig, rxRuntimeConfig_t *rxRuntimeConfig, rcReadRawDataPtr *callback)
{
    UNUSED(rxConfig);
    exbusPort = openSerialPort(FUNCTION_SERIAL_RX, exbusDataReceive, EXBUS_BAUD_LOW, MODE_BIDIR | MODE_RX, SERIAL_NOT_INVERTED);
    if (callback)
        *callback = exbusReadRawRC;

    rxRuntimeConfig->channelCount = EXBUS_NUM_CHANNELS;

    return exbusPort != NULL;
}

// Receive ISR callback
static void exbusDataReceive(uint16_t c)
{
    static bool exbusFrameBegun = false;
    static uint8_t exbusFramePos = 0;
    static uint8_t exBusFrameLen;

    if(!exbusFrameBegun) {
        // check for frame begin byte
        if(c == EXBUS_BYTE_BEGIN) {
            exbusFrameBegun = true;
            exbusFrameDone = false;
            exbusFramePos = 0;
            // actual length isn't known until byte 3 arrives
            exBusFrameLen = EXBUS_FRAME_MINLEN;
        } else {
            exbusJunkChars++;
            return;
        }

    } else if(exbusFramePos == 2) {
        // capture byte 3, aka the frame length
        exBusFrameLen = (uint8_t)c;
        if(exBusFrameLen > EXBUS_FRAME_MAXLEN) // make sure frame isn't too long for buffer
            exbusFrameBegun = false;
    } else if((exbusFramePos == 4) && ((uint8_t)c != EXBUS_BYTE_RC)) {
        // ignore telemetry and jetibox packets
        exbusFrameBegun = false;
    }

    exbusBuf[exbusFramePos++] = (uint8_t)c;

    if(exbusFramePos == exBusFrameLen) {
        // end of frame, check crc16-ccit
        if(exbusCrcGenerate(exbusBuf, exBusFrameLen) == 0) {
            exbusBaudValid = true;
            exbusFrameDone = true;
        } else
            exbusJunkChars += exBusFrameLen;

        // allow to look for a new frame since this one is done
        exbusFrameBegun = false;
    }
}

bool exbusFrameComplete(void)
{
    if(exbusFrameDone) {
        return true;
    } else if(!exbusBaudValid && (exbusJunkChars > 1000)) {
        // If no valid channel data is received try switching baud
        exbusJunkChars = 0;
        // TODO: switch baudrate
    }
    return false;
}

static uint16_t exbusReadRawRC(rxRuntimeConfig_t *rxRuntimeConfig, uint8_t chan)
{
    UNUSED(rxRuntimeConfig);
    uint16_t data;
    uint8_t b;

    if(chan < EXBUS_NUM_CHANNELS && exbusFrameDone) {
        exbusFrameDone = false;
        // rc channel data starts at byte 7
        // each channel is 2 bytes in LSB, MSB order
        b = 6 + (chan * 2);
        data = (((uint16_t)exbusBuf[b+1]) << 8) | ((uint16_t)exbusBuf[b]);
        // 1 = 1/8 us
        data = (data + 4) / 8;
    } else {
        data = 0;
    }

    return data;
}
