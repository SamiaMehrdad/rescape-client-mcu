/************************* roomserial.cpp **********************
 * Room Bus RS-485 Communication Implementation
 * RS-485 framing and protocol handling for Room Bus
 * Created by MSK, November 2025
 * Implements CRC16-CCITT with state machine parser
 ***************************************************************/

#include "roomserial.h"
#include <string.h> // memcpy

// ---------- CRC16-CCITT implementation ----------
//
// Polynomial: 0x1021
// Initial value: 0xFFFF
// No final XOR
//
uint16_t calcCrc(const uint8_t *data, size_t length)
{
        uint16_t crc = 0xFFFF;
        const uint16_t poly = 0x1021;

        for (size_t i = 0; i < length; ++i)
        {
                crc ^= (uint16_t)data[i] << 8;
                for (int bit = 0; bit < 8; ++bit)
                {
                        if (crc & 0x8000)
                                crc = (crc << 1) ^ poly;
                        else
                                crc = (crc << 1);
                }
        }
        return crc;
}

// ---------- Encoder ----------

size_t encodeFrame(const RoomFrame *f, uint8_t *outBuf, size_t outBufSize)
{
        if (!f || !outBuf)
                return 0;
        if (outBufSize < RB_MAX_PACKET_SIZE)
                return 0;

        uint8_t payload[ROOM_FRAME_SIZE];
        size_t pi = 0;

        // Logical payload layout:
        // [0]      addr
        // [1]      cmd_srv
        // [2]      cmd_dev
        // [3..22]  p[0..19]
        // [23]     reserved

        payload[pi++] = f->addr;
        payload[pi++] = f->cmd_srv;
        payload[pi++] = f->cmd_dev;

        for (int k = 0; k < 20; ++k)
                payload[pi++] = f->p[k];

        payload[pi++] = f->reserved;

        if (pi != ROOM_FRAME_SIZE)
                return 0;

        size_t idx = 0;

        // Start byte
        outBuf[idx++] = RB_START_BYTE;

        // Copy payload
        memcpy(&outBuf[idx], payload, ROOM_FRAME_SIZE);
        idx += ROOM_FRAME_SIZE;

        // CRC over payload
        uint16_t crc = calcCrc(payload, ROOM_FRAME_SIZE);
        outBuf[idx++] = (uint8_t)(crc & 0xFF);        // CRC low
        outBuf[idx++] = (uint8_t)((crc >> 8) & 0xFF); // CRC high

        // End byte
        outBuf[idx++] = RB_END_BYTE;

        return idx; // should be RB_MAX_PACKET_SIZE (28)
}

// ---------- Parser ----------

void parserInit(RoomBusParser *parser)
{
        if (!parser)
                return;
        parser->state = RB_PSTATE_WAIT_START;
        parser->idx = 0;
        parser->rxCrc = 0;
}

/**
 * State machine:
 *
 * WAIT_START:
 *   wait for RB_START_BYTE (0xAA)
 *
 * READ_FRAME:
 *   read ROOM_FRAME_SIZE bytes into frameBytes[]
 *
 * READ_CRC_LO / READ_CRC_HI:
 *   read 2-byte CRC from wire
 *
 * WAIT_END:
 *   expect RB_END_BYTE (0x55)
 *   if ok and CRC matches -> unpack into RoomFrame and return 1
 *   else reset to WAIT_START
 */
int parserFeed(RoomBusParser *parser, uint8_t byte, RoomFrame *out)
{
        if (!parser || !out)
                return 0;

        switch (parser->state)
        {
        case RB_PSTATE_WAIT_START:
                if (byte == RB_START_BYTE)
                {
                        parser->idx = 0;
                        parser->rxCrc = 0;
                        parser->state = RB_PSTATE_READ_FRAME;
                }
                break;

        case RB_PSTATE_READ_FRAME:
                parser->frameBytes[parser->idx++] = byte;
                if (parser->idx >= ROOM_FRAME_SIZE)
                {
                        parser->idx = 0;
                        parser->state = RB_PSTATE_READ_CRC_LO;
                }
                break;

        case RB_PSTATE_READ_CRC_LO:
                parser->rxCrc = byte;
                parser->state = RB_PSTATE_READ_CRC_HI;
                break;

        case RB_PSTATE_READ_CRC_HI:
                parser->rxCrc |= ((uint16_t)byte << 8);
                parser->state = RB_PSTATE_WAIT_END;
                break;

        case RB_PSTATE_WAIT_END:
                if (byte == RB_END_BYTE)
                {
                        // Verify CRC
                        uint16_t calc = calcCrc(parser->frameBytes, ROOM_FRAME_SIZE);
                        if (calc == parser->rxCrc)
                        {
                                // Unpack payload into RoomFrame
                                const uint8_t *b = parser->frameBytes;
                                size_t i = 0;

                                out->addr = b[i++];
                                out->cmd_srv = b[i++];
                                out->cmd_dev = b[i++];

                                for (int k = 0; k < 20; ++k)
                                        out->p[k] = b[i++];

                                out->reserved = b[i++];

                                // Reset for next frame
                                parser->state = RB_PSTATE_WAIT_START;
                                return 1; // one complete frame decoded
                        }
                }
                // CRC or end byte failed → reset
                parser->state = RB_PSTATE_WAIT_START;
                break;

        default:
                parser->state = RB_PSTATE_WAIT_START;
                break;
        }

        return 0; // no complete frame yet
}

// ---------- C++ RS-485 Communication Wrapper ----------

#ifdef __cplusplus

RoomSerial::RoomSerial(int rxPin, int txPin, int dePin, unsigned long baudRate)
    : serial(1), // Use UART1
      rxPin(rxPin),
      txPin(txPin),
      dePin(dePin),
      baudRate(baudRate)
{
        parserInit(&parser);
}

void RoomSerial::begin()
{
        // Initialize UART1 with specified pins
        serial.begin(baudRate, SERIAL_8N1, rxPin, txPin);

        // Configure DE pin if provided
        if (dePin >= 0)
        {
                pinMode(dePin, OUTPUT);
                enableReceive(); // Default to receive mode
        }

        parserInit(&parser);
}

void RoomSerial::enableTransmit()
{
        if (dePin >= 0)
        {
                digitalWrite(dePin, HIGH); // DE high = transmit mode
                delayMicroseconds(10);     // Small delay for transceiver switching
        }
}

void RoomSerial::enableReceive()
{
        if (dePin >= 0)
        {
                digitalWrite(dePin, LOW); // DE low = receive mode
        }
}

bool RoomSerial::sendFrame(const RoomFrame *frame)
{
        if (!frame)
                return false;

        // Encode the frame
        size_t len = encodeFrame(frame, txBuffer, sizeof(txBuffer));
        if (len == 0)
                return false;

        // Enable transmit mode
        enableTransmit();

        // Send the frame
        size_t written = serial.write(txBuffer, len);

        // Wait for transmission to complete
        serial.flush();

        // Return to receive mode
        enableReceive();

        return (written == len);
}

bool RoomSerial::receiveFrame(RoomFrame *frame)
{
        if (!frame)
                return false;

        // Process all available bytes
        while (serial.available())
        {
                uint8_t byte = serial.read();
                if (parserFeed(&parser, byte, frame))
                {
                        return true; // Complete frame received
                }
        }

        return false; // No complete frame yet
}

#endif // __cplusplus
