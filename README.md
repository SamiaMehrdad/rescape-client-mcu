# reEscape Client Device

**ESP32-based client controller for reEscape room automation system**

## Overview

This firmware runs on ESP32-C3 microcontrollers and provides a complete interface for escape room puzzles and automation. It supports keypad input, motor control, LED animations, audio synthesis, and RS-485 network communication.

## Features

### Hardware Support

-   **I/O Expansion:** PCF8575 16-bit I2C expander for keypad, motors, and switches
-   **Keypad:** 4x4 matrix with hardware debouncing (16 keys)
-   **Motors:** Dual H-bridge motor control (2 motors)
-   **Switches:** 4 digital inputs with pull-ups
-   **LEDs:** WS2812B RGB strip with animation system
-   **Audio:** PWM-based synthesizer with ADSR envelope
-   **Communication:** RS-485 Room Bus for network control
-   **Buttons:** 2 debounced push buttons

### Software Features

-   **Debounced Input:** Professional keypad debouncing (10ms scan, 3-read verification)
-   **Animation System:** Buffer-based LED animations with configurable timing
-   **Flexible Timers:** Easy-to-use hardware timer library (ESPTimer)
-   **Audio Synthesis:** Multiple waveforms (sine, square, triangle, sawtooth) with ADSR
-   **Network Protocol:** Room Bus communication for multi-device systems
-   **Modular Design:** Clean library structure for easy expansion

## Hardware Requirements

### Recommended Board

-   Seeed XIAO ESP32-C3 (primary target)
-   Alternative: ESP32-S2 Thing Plus or similar

### Peripherals

-   PCF8575 I2C I/O Expander (16-bit)
-   4x4 Matrix Keypad
-   WS2812B LED Strip
-   Speaker/Buzzer (PWM-driven)
-   RS-485 Transceiver Module
-   Optional: Motors, switches, sensors

## Pin Configuration

See `include/mcupins.h` for complete pin definitions.

**I2C Devices:**

-   PCF8575 I/O Expander: Address 0x20

**PCF8575 Pin Mapping:**

-   P00-P03: Keypad rows
-   P04-P07: Keypad columns
-   P10-P11: Motor 1 (MOT1A, MOT1B)
-   P12-P13: Motor 2 (MOT2A, MOT2B)
-   P14-P17: Switches 1-4

## Quick Start

### Installation

1. Install [PlatformIO](https://platformio.org/)
2. Clone this repository
3. Open project in PlatformIO
4. Build and upload:
    ```bash
    pio run -t upload
    ```

### Configuration

Edit `src/main.cpp` to configure:

-   LED strip size and grouping
-   Animation timing
-   ISR update rates
-   Room Bus address

**Timing Constants:**

```cpp
constexpr u8 ISR_INTERVAL_MS = 5;    // ISR frequency (200 Hz)
constexpr u8 ANIM_REFRESH_MS = 40;   // Display update rate (25 Hz)
constexpr u16 ANIM_STEP_MS = 50;     // Animation speed (20 Hz)
```

## Project Structure

```
rescape-client-device/
├── include/              # Header files
│   ├── buttons.h        # Button debouncing
│   ├── colors.h         # RGB color definitions
│   ├── esptimer.h       # Hardware timer library
│   ├── ioexpander.h     # PCF8575 I/O expander
│   ├── mcupins.h        # Pin definitions
│   ├── pixel.h          # WS2812B LED control
│   ├── roombus.h        # Room Bus protocol
│   ├── roomserial.h     # RS-485 communication
│   ├── synth.h          # Audio synthesizer
│   └── watchdog.h       # Watchdog timer
├── src/                 # Implementation files
│   ├── buttons.cpp
│   ├── esptimer.cpp
│   ├── ioexpander.cpp
│   ├── main.cpp         # Main application
│   ├── pixel.cpp
│   ├── roomserial.cpp
│   ├── synth.cpp
│   └── watchdog.cpp
├── lib/                 # External libraries
├── test/                # Unit tests
├── platformio.ini       # Build configuration
├── CHANGELOG.md         # Development history
└── README.md            # This file
```

## Library Documentation

### ESPTimer

Simplified hardware timer management:

```cpp
#include "esptimer.h"

void IRAM_ATTR myCallback() {
  // Timer ISR code
}

hw_timer_t *timer = ESPTimer::begin(0, 10, &myCallback);  // 10ms timer
ESPTimer::stop(timer);
ESPTimer::start(timer);
```

### IOExpander

I2C I/O expander with debouncing:

```cpp
#include "ioexpander.h"

IOExpander io(0x20, &Wire);
io.begin();

u8 key = io.scanKeypad();        // Returns 0-15 or 255
u16 bitmap = io.getKeypadBitmap(); // Multi-key detection
bool sw = io.readSwitch1();
io.setMotorA(MOTOR_FORWARD);
```

### PixelStrip

LED animation system:

```cpp
#include "pixel.h"

PixelStrip pixels(PIN, 10, 4, 50);  // 10 logical pixels, 4 LEDs each
pixels.begin();

// Direct control
pixels.setColor(0, CLR_RD);
pixels.show();

// Animation via buffer
u32 *buffer = pixels.getBuffer();
buffer[5] = CLR_BL;
pixels.applyBuffer();  // Called from ISR or loop
```

### Synth

Audio synthesis:

```cpp
#include "synth.h"

Synth synth(SPKR_PIN, PWM_CHANNEL);
synth.begin(20000);
synth.setWaveform(WAVE_SINE);
synth.setADSR(5, 120, 100, 180);
synth.playNote(NOTE_C4, 300, 100);
```

## Room Bus Protocol

The Room Bus is an RS-485 network protocol for coordinating multiple escape room devices.

**Frame Structure:**

-   Address: Device identifier
-   Command: Service/device command codes
-   Parameters: Up to 10 bytes

See `include/roombus.h` for protocol details.

## Performance

-   **ISR Execution:** < 100μs (button update + flag set)
-   **I2C Speed:** 100 kHz (reliable, no errors)
-   **Keypad Scan:** ~100 Hz with 30ms debounce
-   **LED Updates:** Configurable (default 25 Hz)
-   **Animation Rate:** Configurable (default 20 Hz)

## Development

### Building

```bash
# Build
pio run

# Upload
pio run -t upload

# Monitor
pio device monitor

# Clean
pio run -t clean
```

### Debugging

Serial output at 115200 baud. I2C debug spam is disabled via:

```cpp
esp_log_level_set("i2c", ESP_LOG_NONE);
```

## License

[Add your license here]

## Contributors

-   MSK - November 2025

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for detailed development history.

## Support

[Add support/contact information]

---

**Version:** 1.0.0  
**Last Updated:** November 29, 2025
