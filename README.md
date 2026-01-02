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

## Configuration Model (New)

The system uses a split configuration model to support multiple devices of the same type in a single room.

### 1. Device Type (Factory Config)

-   **What:** Defines the hardware capabilities and application logic (e.g., Purger, Timer, Keypad).
-   **How:** Configured via the **Potentiometer** (Trimmer).
-   **Storage:** Saved in NVS key `deviceType`.
-   **Calibration:** Long-press the boot button to enter **Type Detection Mode**. Adjust the pot to select the type. The setting is permanent until recalibrated.

### 2. Device Address (Room Setup)

-   **What:** Unique logical identifier on the RS-485 bus.
-   **Range:** `0x02` - `0xFD` (252 possible devices).
-   **Reserved:**
    -   `0x00`: Unassigned (Factory Default)
    -   `0x01`: Server (Master Controller)
    -   `0xFE`: Broadcast (All Devices)
-   **How:** Assigned by the **Server** during the room setup phase.
-   **Storage:** Saved in NVS key `address`.
-   **Default:** `0x00` (ADDR_UNASSIGNED) on first boot or factory reset.

## Features

### Core Firmware Capabilities

-   **Device Type Configuration**: 32 device types (0-31) selectable via 20kΩ trimmer pot
-   **Calibration Mode**: Long-press boot button to enter type detection mode
-   **Persistent Storage**: Device type saved to NVS, survives reboots
-   **Audio Engine**:
    -   4-Voice Polyphonic Synthesizer.
    -   Integrated Echo/Delay effect.
    -   Polyphonic Music Sequencer with multi-instrument support.
-   **Status Monitoring**: LED indicator for system health
    -   Solid ON = All systems OK
    -   Fast blink (5 Hz) = I2C communication error
    -   Slow blink (1 Hz) = Invalid device type
-   **Boot Report**: Comprehensive device information display on startup

### Hardware Support

-   **I/O Expansion:** PCF8575 16-bit I2C expander for keypad, motors, and switches
-   **Keypad:** 4x4 matrix with hardware debouncing (16 keys)
-   **Motors:** Dual H-bridge motor control (up to 4 motors)
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
    -   Polyphonic software synth (default 4 voices) using DDS per voice and per-voice ADSR envelopes
    -   Default sample rate: 40 kHz; PWM carrier: 120 kHz (3x oversample) for improved noise shaping
    -   Secondary PWM output available on GPIO8 (software-controlled): complements main output while playing, both driven LOW when silent
    -   ISR-side smoothing applied to reduce stepping/quantization noise
-   **Network Protocol:** Room Bus communication for multi-device systems
-   **Modular Design:** Clean Core + App separation for easy expansion
-   **Error Recovery:** Robust edge case handling and graceful degradation

### Performance

-   **Loop Cycle Time:** ~13ms (~75Hz) typical App level loop execution rate.
    -   Includes Core overhead (Input polling, I2C communication, Status LED updates).
    -   Application logic runs within this cycle.
    -   **Note:** Blocking delays (`delay()`) in App code will increase this time and may trigger the Watchdog timer (1s timeout).

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
-   Motors (up to 4 via H-bridge)
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

-   P00-P07: Motor Control (8 pins for 4 motors via H-bridge)
    -   P00-P01: Motor A
    -   P02-P03: Motor B
    -   P04-P05: Motor C
    -   P06-P07: Motor D
-   P10-P17: Keypad Matrix (8 pins)
    -   P10-P13: Columns
    -   P14-P17: Rows

## Protocol Details (RS-485)

### Frame Format (28 bytes)

```
[0]      START (0xAA)
[1-24]   PAYLOAD (24 bytes):
         [0]     addr (Destination)
         [1]     cmd_srv (Server Command)
         [2]     cmd_dev (Device Event)
         [3-22]  p[0-19] (Parameters)
         [23]    reserved
[25-26]  CRC16-CCITT
[27]     END (0x55)
```

### Key Commands

-   **HELLO (0x01):** Device -> Server. Payload: `[Address, Type]`. Sent on boot.
-   **SET_ADDRESS (0x05):** Server -> Device. Payload: `[New Address]`. Assigns logical address.

## Getting Started

### Prerequisites

-   **VS Code**: [Download here](https://code.visualstudio.com/)
-   **PlatformIO Extension**: Install the "PlatformIO IDE" extension in VS Code.
-   **Git**: Ensure Git is installed on your system.

### Installation

1.  **Clone the Repository:**
    ```bash
    git clone <repository-url>
    cd client-mcu
    ```
2.  **Open in VS Code:**
    -   Open VS Code.
    -   File -> Open Folder -> Select the `client-mcu` folder.
    -   Wait for PlatformIO to initialize (it may download toolchains automatically).

### Configuration

The project uses `platformio.ini` for configuration. The default environment is set to `seeed_xiao_esp32c3`.

-   **Dependencies:** PlatformIO will automatically install required libraries listed in `lib_deps`.
-   **Upload Port:** PlatformIO usually auto-detects the port. If not, specify `upload_port` in `platformio.ini`.

## Development

### Adding a New Device App

1.  Create `src/apps/app_mydevice.h` inheriting from `AppBase`.
2.  Implement `setup()`, `loop()`, `handleInput()`, `handleCommand()`.
3.  Register in `src/apps/app_factory.cpp`.

### Building

Use PlatformIO:

```bash
pio run
pio run --target upload
```
