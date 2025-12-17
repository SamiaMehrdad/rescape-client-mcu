/************************* ioexpander.h ************************
 * I2C I/O Expander Library (PCF8575-based)
 * 16-bit I/O expander for keypad matrix, motors, and sensors
 * Created by MSK, November 2025
 * I2C 16-bit GPIO expander with interrupt support
 ***************************************************************/

#ifndef IOEXPANDER_H
#define IOEXPANDER_H

#include <Arduino.h>
#include <Wire.h>
#include "msk.h"

// I/O Expander I2C address range: 0x20-0x27 (based on A0, A1, A2 pins)
constexpr u8 IO_EXPANDER_BASE_ADDR = 0x20;

// Pin mapping for I/O Expander (16 pins: P00-P07, P10-P17)
// Pin numbering: 0-15 (P00-P07 = 0-7, P10-P17 = 8-15)

// Keypad matrix configuration (4x4 matrix)
constexpr u8 KEYPAD_ROWS = 4;
constexpr u8 KEYPAD_COLS = 4;
constexpr u8 KEYPAD_SIZE = KEYPAD_ROWS * KEYPAD_COLS; // Total keys (16)
constexpr u8 KEYPAD_ROW_START = 12;                   // P14-P17 (pins 12-15)
constexpr u8 KEYPAD_COL_START = 8;                    // P10-P13 (pins 8-11)

// Keypad scanning configuration
namespace KeypadConfig
{
        constexpr unsigned long SCAN_RATE_MS = 10;           // Scan every 10ms
        constexpr u8 DEBOUNCE_COUNT = 3;                     // 3 stable reads required
        constexpr unsigned long MIN_PRESS_INTERVAL_MS = 200; // Minimum 200ms between events
}

// Motor control pins (H-bridge control for 4 motors)
constexpr u8 MOT1A = 0; // P00 - Motor A forward
constexpr u8 MOT1B = 1; // P01 - Motor A reverse
constexpr u8 MOT2A = 2; // P02 - Motor B forward
constexpr u8 MOT2B = 3; // P03 - Motor B reverse
constexpr u8 MOT3A = 4; // P04 - Motor C forward
constexpr u8 MOT3B = 5; // P05 - Motor C reverse
constexpr u8 MOT4A = 6; // P06 - Motor D forward
constexpr u8 MOT4B = 7; // P07 - Motor D reverse

// Keypad key definitions (4x4 matrix)
constexpr char KEYPAD_KEYS[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

// Motor direction constants
enum MotorDirection
{
        MOTOR_STOP = 0,
        MOTOR_FORWARD,
        MOTOR_REVERSE,
        MOTOR_BRAKE
};

/**
 * I/O Expander Class
 * Manages 16-bit I/O expander via I2C for keypad, motors, and sensors
 * Hardware: PCF8575 or compatible 16-bit I2C I/O expander
 */
class IOExpander
{
private:
        u8 _address;      // I2C address
        u16 _outputState; // Current output state (cached)
        u16 _inputState;  // Current input state (last read)
        TwoWire *_wire;   // I2C interface
        bool _isPresent;  // Device presence flag

        // Keypad scanning state
        u8 _lastKeyIndex;    // Last key that triggered an event (0-15, 255 if none)
        u8 _lastKeyRow;      // Last detected key row
        u8 _lastKeyCol;      // Last detected key column
        char _lastKey;       // Last detected key character (for convenience)
        bool _keyPressed;    // Key press state
        u8 _pressedKeyIndex; // Index of currently pressed key (255 if none)

        // Debouncing state
        u8 _stableKeyIndex;              // Last stable key (after debounce)
        u8 _rawKeyIndex;                 // Current raw scan result
        u8 _debounceCount;               // Consecutive matching reads
        unsigned long _lastScanTime;     // Time of last scan (for rate limiting)
        unsigned long _lastKeyPressTime; // Time of last key press event (for minimum response time)

        // I2C communication helpers
        bool writePort(u16 value);
        bool readPort(u16 &value);

public:
        /**
         * Constructor
         * @param address I2C address (0x20-0x27, default 0x20)
         * @param wire I2C interface (default &Wire)
         */
        IOExpander(u8 address = IO_EXPANDER_BASE_ADDR, TwoWire *wire = &Wire);

        /**
         * Initialize the I/O Expander
         * Sets up I2C communication and configures initial pin states
         * @return true if successful, false if device not found
         */
        bool begin();

        /**
         * Check if device is present and responding
         * @return true if device is present
         */
        bool isPresent() const { return _isPresent; }

        // ========== Pin Control Methods ==========

        /**
         * Set a single pin state
         * @param pin Pin number (0-15)
         * @param value HIGH or LOW
         */
        void digitalWrite(u8 pin, u8 value);

        /**
         * Read a single pin state
         * @param pin Pin number (0-15)
         * @return HIGH or LOW
         */
        u8 digitalRead(u8 pin);

        /**
         * Set all 16 pins at once
         * @param value 16-bit value (bit 0 = pin 0, bit 15 = pin 15)
         */
        void write(u16 value);

        /**
         * Read all 16 pins at once
         * @return 16-bit value (bit 0 = pin 0, bit 15 = pin 15)
         */
        u16 read();

        // ========== Keypad Matrix Methods ==========

        /**
         * Scan keypad matrix for key presses (row-column scanning)
         * Call this regularly (e.g., in loop()) to detect key presses
         * @return Key index (0-15 for keys 0-15), or 255 if no key pressed
         */
        u8 scanKeypad();

        /* getKeypadBitmap removed: unused API (cleaned from code/docs) */

        /**
         * Get the last detected key index
         * @return Last key index (0-15), or 255 if no key detected
         */
        u8 getLastKeyIndex() const { return _lastKeyIndex; }

        /**
         * Get the last detected key character (for convenience)
         * Uses the KEYPAD_KEYS mapping to convert index to character
         * @return Last key character, or 0 if no key detected
         */
        char getLastKey() const { return _lastKey; }

        /**
         * Check if a key is currently pressed
         * @return true if key is pressed, false otherwise
         */
        bool isKeyPressed() const { return _keyPressed; }

        // ========== Motor Control Methods ==========

        /**
         * Set motor A direction
         * @param direction MOTOR_STOP, MOTOR_FORWARD, MOTOR_REVERSE, or MOTOR_BRAKE
         */
        void setMotorA(MotorDirection direction);

        /**
         * Set motor B direction
         * @param direction MOTOR_STOP, MOTOR_FORWARD, MOTOR_REVERSE, or MOTOR_BRAKE
         */
        void setMotorB(MotorDirection direction);

        /**
         * Set motor C direction
         * @param direction MOTOR_STOP, MOTOR_FORWARD, MOTOR_REVERSE, or MOTOR_BRAKE
         */
        void setMotorC(MotorDirection direction);

        /**
         * Set motor D direction
         * @param direction MOTOR_STOP, MOTOR_FORWARD, MOTOR_REVERSE, or MOTOR_BRAKE
         */
        void setMotorD(MotorDirection direction);

        /**
         * Stop all motors (A, B, C, D)
         */
        void stopAllMotors();
};

#endif // IOEXPANDER_H
