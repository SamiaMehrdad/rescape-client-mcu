/*
    re-escape room project
    roomserial.h - RS-485 framing helpers for Room Bus
    Created by MSK & ChatGPT, 2025
*/

#ifndef ROOM_BUS_SERIAL_H
#define ROOM_BUS_SERIAL_H

#include "roombus.h" // must define RoomFrame, u8, etc.
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

        // ---------- On-wire format ----------
        // Logical payload (24 bytes):
        //   [0]      addr
        //   [1]      cmd_srv
        //   [2]      cmd_dev
        //   [3..22]  p[0..19]
        //   [23]     reserved
        //
        // RS-485 frame:
        //   [0]          = START (0xAA)
        //   [1..24]      = payload (24 bytes above)
        //   [25..26]     = CRC16-CCITT over payload
        //   [27]         = END (0x55)

#define RB_START_BYTE 0xAA
#define RB_END_BYTE 0x55

#define ROOM_FRAME_SIZE 24                               // bytes in logical payload
#define RB_MAX_PACKET_SIZE (1 + ROOM_FRAME_SIZE + 2 + 1) // 28 bytes

        // ---------- CRC ----------

        /**
         * Compute CRC16-CCITT over buffer.
         * Polynomial: 0x1021, initial value: 0xFFFF, no final xor.
         */
        uint16_t calcCrc(const uint8_t *data, size_t length);

        // ---------- Encoder ----------

        /**
         * Encode a RoomFrame into an RS-485 packet.
         *
         * @param f          pointer to RoomFrame (addr/cmd_srv/cmd_dev/p[]/reserved filled)
         * @param outBuf     output buffer
         * @param outBufSize size of output buffer (must be >= RB_MAX_PACKET_SIZE)
         * @return number of bytes written to outBuf (0 on error)
         */
        size_t encodeFrame(const RoomFrame *f, uint8_t *outBuf, size_t outBufSize);

        // ---------- Streaming parser ----------

        typedef enum
        {
                RB_PSTATE_WAIT_START = 0,
                RB_PSTATE_READ_FRAME,
                RB_PSTATE_READ_CRC_LO,
                RB_PSTATE_READ_CRC_HI,
                RB_PSTATE_WAIT_END
        } RoomBusParserState;

        /**
         * Parser context (can have separate instances on server and devices).
         */
        typedef struct
        {
                RoomBusParserState state;
                uint8_t idx;                         // index into frameBytes
                uint8_t frameBytes[ROOM_FRAME_SIZE]; // raw payload bytes
                uint16_t rxCrc;                      // CRC received from wire
        } RoomBusParser;

        /**
         * Initialize a parser context.
         */
        void parserInit(RoomBusParser *parser);

        /**
         * Feed one byte into the parser.
         *
         * @param parser  pointer to parser context
         * @param byte    next byte from UART/RS-485 stream
         * @param out     pointer to RoomFrame to fill when a full valid frame is received
         * @return 1 if a complete, CRC-checked RoomFrame is decoded into *out, 0 otherwise
         */
        int parserFeed(RoomBusParser *parser, uint8_t byte, RoomFrame *out);

#ifdef __cplusplus
} // extern "C"

// ---------- C++ RS-485 Communication Wrapper ----------

#include <HardwareSerial.h>

/**
 * RS-485 communication manager for Room Bus
 * Handles UART communication, TX/RX enable, and frame parsing
 */
class RoomSerial
{
public:
        /**
         * Constructor
         * @param rxPin GPIO pin for RS-485 RX
         * @param txPin GPIO pin for RS-485 TX
         * @param dePin GPIO pin for RS-485 DE (Driver Enable) - optional, -1 if not used
         * @param baudRate Communication baud rate (default 9600)
         */
        RoomSerial(int rxPin, int txPin, int dePin = -1, unsigned long baudRate = 9600);

        /**
         * Initialize the RS-485 serial port
         */
        void begin();

        /**
         * Send a RoomFrame over RS-485
         * @param frame Pointer to the frame to send
         * @return true if sent successfully, false otherwise
         */
        bool sendFrame(const RoomFrame *frame);

        /**
         * Check for incoming frames and process them
         * Call this regularly in loop()
         * @param frame Pointer to RoomFrame to fill when a frame is received
         * @return true if a complete frame was received, false otherwise
         */
        bool receiveFrame(RoomFrame *frame);

        /**
         * Get access to the underlying serial port
         */
        HardwareSerial &getSerial() { return serial; }

private:
        HardwareSerial serial;
        RoomBusParser parser;
        int rxPin;
        int txPin;
        int dePin; // Driver Enable pin for RS-485 transceiver (-1 if not used)
        unsigned long baudRate;
        uint8_t txBuffer[RB_MAX_PACKET_SIZE];

        void enableTransmit();
        void enableReceive();
};

#endif // __cplusplus

#endif // ROOM_BUS_SERIAL_H
