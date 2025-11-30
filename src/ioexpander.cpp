/************************* ioexpander.cpp **********************
 * I2C I/O Expander Library Implementation (PCF8575-based)
 * 16-bit I/O expander for keypad matrix, motors, and sensors
 * Created by MSK, November 2025
 * I2C 16-bit GPIO expander with interrupt support
 ***************************************************************/

#include "ioexpander.h"

/**
 * Constructor
 */
IOExpander::IOExpander(u8 address, TwoWire *wire)
    : _address(address), _outputState(0xFFFF) // All pins high by default (pull-ups)
      ,
      _inputState(0x0000), _wire(wire), _isPresent(false), _lastKeyIndex(255), _lastKeyRow(0xFF), _lastKeyCol(0xFF), _lastKey(0), _keyPressed(false),
      _stableKeyIndex(255), _rawKeyIndex(255), _debounceCount(0), _lastScanTime(0)
{
}

/**
 * Initialize the I/O Expander
 * Note: Expects I2C bus to be initialized at 100 kHz (default Arduino Wire speed)
 * PCF8575 supports up to 400 kHz, but 100 kHz provides excellent reliability
 */
bool IOExpander::begin()
{
        // Set all pins high initially (keypad columns, motor off, switches as inputs)
        // PCF8575 pins are quasi-bidirectional: write HIGH to configure as input
        _outputState = 0xFFFF;

        // Try to write initial state to verify device presence
        if (!writePort(_outputState))
        {
                _isPresent = false;
                return false; // Device not found
        }

        // Read back to verify communication
        u16 readback;
        if (!readPort(readback))
        {
                _isPresent = false;
                return false; // Communication failed
        }

        _isPresent = true;
        return true;
}

/**
 * Write to I/O Expander port
 */
bool IOExpander::writePort(u16 value)
{
        _wire->beginTransmission(_address);
        _wire->write(value & 0xFF);        // Low byte (P00-P07)
        _wire->write((value >> 8) & 0xFF); // High byte (P10-P17)
        return (_wire->endTransmission() == 0);
}

/**
 * Read from I/O Expander port
 */
bool IOExpander::readPort(u16 &value)
{
        u8 bytesRead = _wire->requestFrom(_address, (u8)2);
        if (bytesRead != 2)
        {
                return false;
        }

        u8 lowByte = _wire->read();
        u8 highByte = _wire->read();
        value = (u16)lowByte | ((u16)highByte << 8);
        _inputState = value;

        return true;
}

/**
 * Set a single pin state
 */
void IOExpander::digitalWrite(u8 pin, u8 value)
{
        if (pin > 15)
                return;

        if (value == HIGH)
        {
                _outputState |= (1 << pin); // Set bit
        }
        else
        {
                _outputState &= ~(1 << pin); // Clear bit
        }

        writePort(_outputState);
}

/**
 * Read a single pin state
 */
u8 IOExpander::digitalRead(u8 pin)
{
        if (pin > 15)
                return LOW;

        u16 value;
        if (!readPort(value))
        {
                return LOW; // Return LOW on error
        }

        return (value & (1 << pin)) ? HIGH : LOW;
}

/**
 * Set all 16 pins at once
 */
void IOExpander::write(u16 value)
{
        _outputState = value;
        writePort(_outputState);
}

/**
 * Read all 16 pins at once
 */
u16 IOExpander::read()
{
        u16 value;
        readPort(value);
        return value;
}

/**
 * Scan keypad matrix for key presses with debouncing
 *
 * Keypad matrix scanning algorithm with debouncing:
 * 1. Rate limit scans to every 10ms for slower, more reliable operation
 * 2. Perform matrix scan (row-by-row column detection)
 * 3. Require 3 consecutive identical readings before accepting new key state
 * 4. Only report new key press when previous key was released (prevents repeat)
 *
 * Debouncing strategy:
 * - Scan rate: ~10ms (slower than typical bounce duration)
 * - Consecutive reads: 3 (total ~30ms settle time)
 * - State machine: Only trigger on press->release->press transitions
 *
 * @return Key index (0-15 for keys 0-15), or 255 if no key pressed
 */
u8 IOExpander::scanKeypad()
{
        // Rate limiting: Only scan every 10ms
        unsigned long currentTime = millis();
        if (currentTime - _lastScanTime < 10)
        {
                return _stableKeyIndex; // Return last stable key
        }
        _lastScanTime = currentTime;

        // Perform raw matrix scan
        u8 detectedKeyIndex = 255;
        bool keyFound = false;

        // Set all columns HIGH (inactive) and all rows HIGH (ready to scan)
        u16 baseState = _outputState;

        // Set all keypad pins HIGH initially
        for (u8 row = 0; row < KEYPAD_ROWS; row++)
        {
                baseState |= (1 << (KEYPAD_ROW_START + row));
        }
        for (u8 col = 0; col < KEYPAD_COLS; col++)
        {
                baseState |= (1 << (KEYPAD_COL_START + col));
        }

        // Scan each row
        for (u8 row = 0; row < KEYPAD_ROWS; row++)
        {
                // Set current row LOW, others HIGH
                u16 scanState = baseState;
                scanState &= ~(1 << (KEYPAD_ROW_START + row)); // Current row LOW

                writePort(scanState);
                delayMicroseconds(10); // Allow signals to settle

                // Read the port
                u16 readValue;
                if (!readPort(readValue))
                {
                        continue; // Skip on read error
                }

                // Check each column
                for (u8 col = 0; col < KEYPAD_COLS; col++)
                {
                        u8 colPin = KEYPAD_COL_START + col;
                        if ((readValue & (1 << colPin)) == 0)
                        {                                                   // Column is LOW (key pressed)
                                detectedKeyIndex = row * KEYPAD_COLS + col; // Calculate linear index (0-15)
                                keyFound = true;
                                break; // Found a key, stop scanning
                        }
                }

                if (keyFound)
                        break;
        }

        // Restore all rows HIGH
        writePort(baseState);

        // Debouncing state machine
        if (detectedKeyIndex == _rawKeyIndex)
        {
                // Same key as last scan - increment stability counter
                _debounceCount++;

                if (_debounceCount >= 3)
                {
                        // Key is stable (3+ consecutive reads)
                        if (detectedKeyIndex != _stableKeyIndex)
                        {
                                // Key state changed and is stable
                                if (detectedKeyIndex == 255)
                                {
                                        // Key released
                                        _stableKeyIndex = 255;
                                        _keyPressed = false;
                                }
                                else if (_stableKeyIndex == 255)
                                {
                                        // New key pressed (only trigger if previous was released)
                                        _stableKeyIndex = detectedKeyIndex;
                                        _lastKeyIndex = detectedKeyIndex;

                                        // Update row/col/char for compatibility
                                        _lastKeyRow = detectedKeyIndex / KEYPAD_COLS;
                                        _lastKeyCol = detectedKeyIndex % KEYPAD_COLS;
                                        _lastKey = KEYPAD_KEYS[_lastKeyRow][_lastKeyCol];
                                        _keyPressed = true;
                                }
                        }
                }
        }
        else
        {
                // Different key detected - reset debounce counter
                _rawKeyIndex = detectedKeyIndex;
                _debounceCount = 1;
        }

        return _stableKeyIndex;
}

/**
 * Get all keypad keys as a 16-bit bitmap
 * Each bit represents a key state (1 = pressed, 0 = released)
 */
u16 IOExpander::getKeypadBitmap()
{
        u16 bitmap = 0;

        // Set all columns HIGH (inactive) and all rows HIGH (ready to scan)
        u16 baseState = _outputState;

        // Set all keypad pins HIGH initially
        for (u8 row = 0; row < KEYPAD_ROWS; row++)
        {
                baseState |= (1 << (KEYPAD_ROW_START + row));
        }
        for (u8 col = 0; col < KEYPAD_COLS; col++)
        {
                baseState |= (1 << (KEYPAD_COL_START + col));
        }

        // Scan each row
        for (u8 row = 0; row < KEYPAD_ROWS; row++)
        {
                // Set current row LOW, others HIGH
                u16 scanState = baseState;
                scanState &= ~(1 << (KEYPAD_ROW_START + row)); // Current row LOW

                writePort(scanState);
                delayMicroseconds(10); // Allow signals to settle

                // Read the port
                u16 readValue;
                if (!readPort(readValue))
                {
                        continue; // Skip on read error
                }

                // Check each column and set bitmap bits
                for (u8 col = 0; col < KEYPAD_COLS; col++)
                {
                        u8 colPin = KEYPAD_COL_START + col;
                        if ((readValue & (1 << colPin)) == 0)
                        { // Column is LOW (key pressed)
                                u8 keyIndex = row * KEYPAD_COLS + col;
                                bitmap |= (1 << keyIndex); // Set bit for this key
                        }
                }
        }

        // Restore all rows HIGH
        writePort(baseState);

        return bitmap;
}

/**
 * Set motor A direction
 */
void IOExpander::setMotorA(MotorDirection direction)
{
        u16 state = _outputState;

        // Clear motor A bits
        state &= ~((1 << MOT1A) | (1 << MOT1B));

        switch (direction)
        {
        case MOTOR_FORWARD:
                state |= (1 << MOT1A); // IN1=HIGH, IN2=LOW
                break;
        case MOTOR_REVERSE:
                state |= (1 << MOT1B); // IN1=LOW, IN2=HIGH
                break;
        case MOTOR_BRAKE:
                state |= (1 << MOT1A) | (1 << MOT1B); // Both HIGH
                break;
        case MOTOR_STOP:
        default:
                // Both LOW (already cleared)
                break;
        }

        _outputState = state;
        writePort(_outputState);
}

/**
 * Set motor B direction
 */
void IOExpander::setMotorB(MotorDirection direction)
{
        u16 state = _outputState;

        // Clear motor B bits
        state &= ~((1 << MOT2A) | (1 << MOT2B));

        switch (direction)
        {
        case MOTOR_FORWARD:
                state |= (1 << MOT2A); // IN1=HIGH, IN2=LOW
                break;
        case MOTOR_REVERSE:
                state |= (1 << MOT2B); // IN1=LOW, IN2=HIGH
                break;
        case MOTOR_BRAKE:
                state |= (1 << MOT2A) | (1 << MOT2B); // Both HIGH
                break;
        case MOTOR_STOP:
        default:
                // Both LOW (already cleared)
                break;
        }

        _outputState = state;
        writePort(_outputState);
}

/**
 * Stop all motors
 */
void IOExpander::stopAllMotors()
{
        setMotorA(MOTOR_STOP);
        setMotorB(MOTOR_STOP);
}

/**
 * Read switch 1 state
 */
bool IOExpander::readSwitch1()
{
        return digitalRead(SW_1) == HIGH;
}

/**
 * Read switch 2 state
 */
bool IOExpander::readSwitch2()
{
        return digitalRead(SW_2) == HIGH;
}

/**
 * Read switch 3 state
 */
bool IOExpander::readSwitch3()
{
        return digitalRead(SW_3) == HIGH;
}

/**
 * Read switch 4 state
 */
bool IOExpander::readSwitch4()
{
        return digitalRead(SW_4) == HIGH;
}
