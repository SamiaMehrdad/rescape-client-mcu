# Status LED Implementation

This document describes the hardware status LED feature added to the ESP32 client MCU.

## Overview

A **status LED** on **GPIO 3 (D1/A1)** provides visual feedback about the system's health status without requiring serial monitor access.

## LED Status Modes

| Mode | LED Behavior | Description | When It Occurs |
| --- | --- | --- | --- |
| **OK** | Solid ON | System healthy, all components working | I2C devices present and device type valid |
| **I2C-ERR** | Fast blink (5 Hz) | I2C communication failure | I/O Expander (PCF8575) not detected at startup |
| **TYPE-ERR** | Slow blink (1 Hz) | Invalid device type | Device type reading out of expected range (reserved for future use) |

## Hardware Connection

### Pin Assignment

-   **GPIO 3** (D1/A1 on XIAO ESP32-C3)
-   Active HIGH (LED ON when pin is HIGH)
-   Defined in `mcupins.h` as `STATUS_LED_PIN`

### Circuit

```
GPIO 3 ────┬──── LED Anode (+)
           │
           └──── 220Ω Resistor ──── LED Cathode (-) ──── GND
```

**Typical values:**

-   LED: Any standard 3mm or 5mm LED (red, green, or blue)
-   Resistor: 220Ω - 470Ω (current limiting)
-   Current: ~10-15mA @ 3.3V

## Software Implementation

### Status LED Control Functions

```cpp
// Set LED mode (in app.h/app.cpp)
void Application::setStatusLed(StatusLedMode mode);

// Update LED state - call frequently from main loop
void Application::updateStatusLed();
```

### Usage Example

```cpp
// In setup (main.cpp)
if (!ioExpander.begin())
{
    app.setStatusLed(STATUS_I2C_ERROR);  // Fast blink
}
else
{
    app.setStatusLed(STATUS_OK);  // Solid ON
}

// In loop (main.cpp)
void loop()
{
    app.update();  // Internally calls updateStatusLed()
}
```

### Timing Specifications

| Mode     | Period | ON Time    | OFF Time | Frequency |
| -------- | ------ | ---------- | -------- | --------- |
| OK       | -      | Continuous | 0ms      | -         |
| I2C-ERR  | 200ms  | 100ms      | 100ms    | 5 Hz      |
| TYPE-ERR | 1000ms | 500ms      | 500ms    | 1 Hz      |

## Button Changes

### Single Button Configuration

The system uses **only the built-in boot button** - no external buttons needed!

| Button | GPIO | Hardware | Function |
| --- | --- | --- | --- |
| **BTN1** | GPIO 9 (D9) | **XIAO ESP32-C3 on-board boot button** | Cycle colors / Long press: Start animation |
| ~~BTN2~~ | ~~GPIO 10~~ | **Removed** - GPIO 10 now available for other uses |

**Hardware Advantage:** The XIAO ESP32-C3's built-in boot button is perfect for this application:

-   ✅ High-quality tactile button already on the board
-   ✅ No external components needed (saves cost and board space)
-   ✅ Well-designed with proper pull-up resistor
-   ✅ Active-low operation (pressed = LOW, released = HIGH)
-   ✅ Dual purpose: Programming mode during boot, user input during runtime

### Button Functionality

-   **Short Press (BTN1):** Cycle rainbow color pattern forward
-   **Long Press (BTN1):** Start/stop rainbow animation

## Troubleshooting

### LED Not Working

1. **LED always OFF:**

    - Check LED polarity (anode to GPIO, cathode to GND)
    - Verify resistor is present (220Ω-470Ω)
    - Confirm GPIO 3 is not used by another peripheral

2. **LED always ON (not blinking during I2C error):**

    - Verify `app.update()` is being called in main loop
    - Check serial output for I2C status messages
    - Confirm I/O Expander is truly absent

3. **Wrong blink rate:**
    - Check timer implementation in `updateStatusLed()`
    - Verify `millis()` is working correctly

### I2C Error Detection

The I2C error is detected at startup when the I/O Expander (PCF8575) fails to respond:

```cpp
// In main.cpp setup()
if (!ioExpander.begin())
{
    Serial.println("WARNING: I/O Expander not found");
    app.setStatusLed(STATUS_I2C_ERROR);  // Fast blink
}
```

To test I2C error mode:

-   Disconnect the I/O Expander from I2C bus
-   Power cycle the device
-   LED should fast blink (5 Hz)

## Future Enhancements

Possible additions to status LED functionality:

1. **Additional error modes:**

    - WiFi connection status
    - Room Bus communication errors
    - Memory/flash errors

2. **Multi-color LED:**

    - Use RGB LED for more status indication
    - Green = OK, Red = Error, Yellow = Warning

3. **Brightness control:**

    - PWM dimming for nighttime operation
    - User-adjustable brightness levels

4. **Status code sequences:**
    - Brief blink patterns to indicate specific error codes
    - Example: 3 fast blinks = specific I2C address conflict

## Related Files

-   `include/mcupins.h` - STATUS_LED_PIN definition
-   `include/app.h` - StatusLedMode enum, status LED methods
-   `src/app.cpp` - setStatusLed(), updateStatusLed() implementation
-   `src/main.cpp` - Status LED initialization in setup()
-   `include/buttons.h` - Single button configuration (NUM_BUTTONS = 1)
-   `src/buttons.cpp` - Single button implementation

## Hardware Summary

### Before Changes

-   2 buttons (GPIO 9 on-board, GPIO 10 external)
-   No status LED
-   Required external push button

### After Changes

-   1 button (GPIO 9 - **on-board boot button only**)
-   1 status LED (GPIO 3)
-   GPIO 10 available for future use
-   **No external buttons needed** - cleaner, simpler hardware!
