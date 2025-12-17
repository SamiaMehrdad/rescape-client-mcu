# Escape Room Client - Core Firmware

**ESP32-based client controller for escape room automation system**

## Overview

This firmware runs on ESP32-C3 microcontrollers and provides a complete, production-ready interface for escape room puzzles and automation. It features a **Core + App architecture** where the Core firmware handles hardware abstraction and device configuration, while reserving the App layer for high-level, application-specific logic.

### Key Architecture

-   **Core Firmware**: Hardware drivers, device type management, I/O abstraction
-   **App Layer** (Reserved): High-level puzzle logic, game state, application-specific features
-   **Device Type System**: 64-type capacity (0-31 active, 32-63 reserved) configured via trimmer pot
-   **Persistent Storage**: NVS (Non-Volatile Storage) for device configuration
-   **Status LED**: Visual feedback for system health (OK/I2C-ERROR/TYPE-ERROR)

## Features

### Core Firmware Capabilities

-   **Device Type Configuration**: 32 device types (0-31) selectable via 20kΩ trimmer pot
-   **Calibration Mode**: Long-press boot button to enter type detection mode
-   **Persistent Storage**: Device type saved to NVS, survives reboots
-   **Status Monitoring**: LED indicator for system health
    -   Solid ON = All systems OK
    -   Fast blink (5 Hz) = I2C communication error
    -   Slow blink (1 Hz) = Invalid device type
-   **Boot Report**: Comprehensive device information display on startup

### Hardware Support

-   **I/O Expansion:** PCF8575 16-bit I2C expander for keypad, motors, and switches
-   **Keypad:** 4x4 matrix with hardware debouncing (16 keys)
-   **Motors:** Dual H-bridge motor control (2 motors)
-   **Switches:** 4 digital inputs with pull-ups
-   **LEDs:** WS2812B RGB strip with animation system
-   **Audio:** PWM-based synthesizer with ADSR envelope
-   **Communication:** RS-485 Room Bus for network control
-   **Configuration:** ADC-based device type selection (trimmer pot)
-   **Status LED:** Visual system health indicator

### Software Features

-   **Debounced Input:** Professional keypad debouncing (10ms scan, 3-read verification)
-   **Animation System:** Buffer-based LED animations with configurable timing
-   **Flexible Timers:** Easy-to-use hardware timer library (ESPTimer)
-   **Audio Synthesis:** Multiple waveforms (sine, square, triangle, sawtooth) with ADSR
-   **Network Protocol:** Room Bus communication for multi-device systems
-   **Modular Design:** Clean Core + App separation for easy expansion
-   **Error Recovery:** Robust edge case handling and graceful degradation

## Hardware Requirements

### Recommended Board

-   **Seeed XIAO ESP32-C3** (primary target, RISC-V architecture)
-   Alternative: ESP32-S2 Thing Plus or similar

### Required Components

-   PCF8575 I2C I/O Expander (16-bit, address 0x20)
-   20kΩ multi-turn trimmer potentiometer (device type configuration)
-   Status LED (connected to GPIO 3)
-   WS2812B LED Strip
-   Speaker/Buzzer (PWM-driven)
-   RS-485 Transceiver Module

### Optional Components

-   4x4 Matrix Keypad
-   Motors (up to 2 via H-bridge)
-   Switches/Sensors (up to 4)

## Pin Configuration

**ESP32-C3 (Seeed XIAO) Pin Assignments:**

| Function            | GPIO | Notes                                  |
| ------------------- | ---- | -------------------------------------- |
| Boot Button (BTN_1) | 9    | On-board boot button, only button used |
| Status LED          | 3    | System health indicator                |
| Config ADC          | 2    | Device type selection (trimmer pot)    |
| I2C SDA             | 6    | I/O expander communication             |
| I2C SCL             | 7    | I/O expander communication             |
| UART TX             | 21   | RS-485 communication                   |
| UART RX             | 20   | RS-485 communication                   |
| RS-485 DE           | 8    | Driver enable (auto-direction mode)    |
| Speaker             | 5    | PWM audio output                       |
| WS2812B             | 4    | LED strip data line                    |
| I/O Expander INT    | 10   | Interrupt pin (optional)               |

**I2C Devices:**

-   PCF8575 I/O Expander: Address 0x20

**PCF8575 Pin Mapping:**

-   P00-P03: Keypad rows
-   P04-P07: Keypad columns
-   P10-P11: Motor 1 (MOT1A, MOT1B)
-   P12-P13: Motor 2 (MOT2A, MOT2B)
-   P14-P17: Switches 1-4

See `include/mcupins.h` for complete pin definitions.

## Quick Start

### Installation

1. Install [PlatformIO](https://platformio.org/)
2. Clone this repository:
    ```bash
    git clone https://github.com/SamiaMehrdad/rescape-client-mcu.git
    cd rescape-client-mcu
    ```
3. Build and upload:
    ```bash
    platformio run --target upload
    ```
4. Monitor serial output:
    ```bash
    platformio device monitor
    ```

### First Boot Configuration

1. **Hardware Setup:**

    - Connect 20kΩ trimmer pot between 3.3V and GND
    - Connect wiper to GPIO 2 (CONFIG_ADC_PIN)
    - Connect status LED to GPIO 3 (STATUS_LED_PIN)

2. **Device Type Calibration:**

    - Power on device - it will read ADC and assign initial type
    - Long-press boot button (GPIO 9) to enter calibration mode
    - Adjust trimmer pot to select desired type (0-31)
    - LED flashes every 0.5 seconds with beep
    - Serial monitor shows type changes in real-time
    - Long-press boot button again to save and exit

3. **Boot Report:**
    - Device displays comprehensive boot report showing:
        - Device type and configuration status
        - Hardware info (board, chip, MAC address)
        - Firmware version and build date

### Configuration

Edit `src/main.cpp` to configure hardware parameters:

```cpp
// LED strip configuration
PixelStrip pixels(PIXEL_PIN, 8, 1, 5);  // pin, count, groupSize, brightness

// Timer configuration
constexpr u8 ISR_INTERVAL_MS = 5;    // ISR frequency (200 Hz)
constexpr u8 ANIM_REFRESH_MS = 40;   // Display update rate (25 Hz)
```

Device type behavior is defined in `src/core.cpp` - override in App layer for custom logic.

## Project Structure

```
rescape-client-mcu/
├── include/              # Header files
│   ├── core.h           # Core firmware (device type, modes, NVS)
│   ├── app.h            # Application layer (reserved for high-level logic)
│   ├── animation.h      # LED animation system
│   ├── inputmanager.h   # Unified input event handling
│   ├── buttons.h        # Button debouncing
│   ├── colors.h         # RGB color definitions
│   ├── esptimer.h       # Hardware timer library
│   ├── ioexpander.h     # PCF8575 I/O expander
│   ├── mcupins.h        # Pin definitions (board-specific)
│   ├── pixel.h          # WS2812B LED control
│   ├── roombus.h        # Room Bus protocol
│   ├── roomserial.h     # RS-485 communication
│   ├── synth.h          # Audio synthesizer
│   └── watchdog.h       # Watchdog timer
├── src/                 # Implementation files
│   ├── core.cpp         # Core firmware implementation
│   ├── app.cpp          # Application layer
│   ├── animation.cpp    # Animation engine
│   ├── inputmanager.cpp # Input handling
│   ├── buttons.cpp      # Button logic
│   ├── esptimer.cpp     # Timer implementation
│   ├── ioexpander.cpp   # I/O expander driver
│   ├── main.cpp         # Arduino entry point
│   ├── pixel.cpp        # LED control
│   ├── roomserial.cpp   # Communication driver
│   ├── synth.cpp        # Audio synthesis
│   └── watchdog.cpp     # Watchdog implementation
├── platformio.ini       # PlatformIO configuration
├── README.md            # This file
└── docs/                # Documentation
    ├── 64_TYPE_EXPANSION.md              # Future 64-type expansion
    ├── DEVICE_TYPE_SYSTEM_COMPLETE.md    # Complete type system docs
    ├── STATUS_LED.md                     # Status LED feature
    ├── TYPE_DETECTION_MODE.md            # Calibration mode
    ├── HARDWARE_DESIGN.md                # Hardware requirements
    ├── BUILD_FIXES.md                    # Build troubleshooting
    ├── DEVICE_TYPE_ARCHITECTURE.md       # Core + App architecture
    ├── NVS_DEVICE_TYPE_STORAGE.md        # NVS persistent storage
    ├── EDGE_CASE_HANDLING.md             # Error scenarios
    ├── BOOT_REPORT.md                    # Boot report feature
    └── RS485_USAGE.md                    # RS-485 communication
```

## Core Firmware Modes

The system operates in four distinct modes:

1. **INTERACTIVE** - Direct user control via buttons/keypad
2. **ANIMATION** - Automated LED animations
3. **REMOTE** - Control via Room Bus network commands
4. **TYPE_DETECTION** - Device type calibration mode

Switch between modes using button presses or Room Bus commands.

## Device Type System

### Overview

Each device can be configured as one of 32 types (0-31, expandable to 64) using a trimmer potentiometer:

-   **TYPE_00 to TYPE_31**: Currently implemented
-   **TYPE_32 to TYPE_63**: Reserved for future expansion

### Hardware Configuration

1. Connect 20kΩ multi-turn trimmer pot:

    - Pin 1 → GND
    - Pin 2 (wiper) → GPIO 2 (CONFIG_ADC_PIN)
    - Pin 3 → 3.3V

2. ADC Mapping:
    - ADC 0-63 → TYPE_00
    - ADC 64-127 → TYPE_01
    - ...
    - ADC 1984-2047 → TYPE_31
    - Step size: 64 ADC units per type

### Calibration Mode

Enter calibration mode by long-pressing the boot button:

1. LED turns off, system ready for configuration
2. Adjust trimmer pot to select type
3. LED flashes + beeps every 0.5 seconds
4. Serial monitor shows current type
5. Long-press boot button to save to NVS

### Edge Cases Handled

✅ Disconnected potentiometer detection (ADC < 30)  
✅ Noisy connection detection (ADC range > 200)  
✅ Invalid readings don't overwrite stored type  
✅ Type preserved if calibration fails  
✅ Automatic restore on error

## Development Guide

### Adding New Device Types

Device types are defined in `src/core.cpp`:

```cpp
const char *Core::kDeviceTypeNames[64] = {
    "TYPE_00", "TYPE_01", ... "TYPE_63"
};
```

To add application-specific behavior:

1. Check device type in App layer:

    ```cpp
    u8 type = core.getDeviceType();
    if (type == 5) {
        // Custom logic for TYPE_05
    }
    ```

2. Override type names in App layer:
    ```cpp
    const char* getCustomTypeName(u8 type) {
        switch(type) {
            case 0: return "PUZZLE_LOCK";
            case 1: return "DOOR_SENSOR";
            ...
        }
    }
    ```

### Core + App Architecture

**Core Layer** (`core.h/cpp`):

-   Hardware abstraction
-   Device type management
-   NVS storage
-   Status monitoring
-   Low-level I/O

**App Layer** (`app.h/cpp`, reserved):

-   Puzzle-specific logic
-   Game state management
-   High-level behavior
-   Custom type definitions

This separation allows Core firmware to remain stable while App logic evolves. │ ├── pixel.cpp │ ├── roomserial.cpp │ ├── synth.cpp │ └── watchdog.cpp ├── lib/ # External libraries ├── test/ # Unit tests ├── platformio.ini # Build configuration ├── CHANGELOG.md # Development history └── README.md # This file

````

## Library Documentation

### ESPTimer

Simplified hardware timer management:

```cpp
#include "esptimer.h"

void IRAM_ATTR myCallback() {
  // Timer ISR code
}

hw_timer_t *timer = ESPTimer::begin(0, 10, &myCallback);  // 10ms timer
// Use `ESPTimer` returned handle as needed. start/stop APIs removed.
````

### IOExpander

I2C I/O expander with debouncing:

```cpp
#include "ioexpander.h"

IOExpander io(0x20, &Wire);
io.begin();

u8 key = io.scanKeypad();        // Returns 0-15 or 255
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
