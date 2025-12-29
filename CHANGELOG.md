# Escape Room Controller - Changelog & Progress

## Project Overview

ESP32-based escape room controller with keypad, motors, LEDs, audio, and RS-485 communication.

**Hardware:**

-   Board: Seeed XIAO ESP32-C3
-   I/O Expander: PCF8575 (16-bit I2C)
-   Keypad: 4x4 matrix (16 keys)
-   Motors: 4x DC motors via H-bridge (2x L293D or similar)
-   Switches: 4x digital inputs (via Keypad matrix or direct)
-   LEDs: WS2812B RGB strip (configurable groups)
-   Audio: PWM synthesizer with ADSR envelope
-   Communication: RS-485 Room Bus

---

## December 26, 2025 - Device Addressing & Multi-Device Support

### 🚀 Dynamic Addressing System

**Status:** ✅ Complete

**Summary:** Implemented a split configuration model to support multiple devices of the same type in a single room.

**Changes:**

1.  **Split Configuration Model:**

    -   **Device Type (Factory):** Configured via Potentiometer. Stored in NVS `deviceType`.
    -   **Device Address (Room Setup):** Assigned by Server. Stored in NVS `address`.

2.  **Protocol Updates:**

    -   **HELLO Command:** Now sends `[Address, Type]` payload.
    -   **SET_ADDRESS (0x05):** New command for server to assign addresses.

3.  **Core Logic:**
    -   `init()` loads Type and Address separately.
    -   `handleRoomBusFrame()` filters based on dynamic address.
    -   `enterTypeDetectionMode()` only calibrates hardware type.

---

**Summary:** Refactored the device configuration system to replace the complex X-Macro pattern with a simpler, more explicit struct-based approach. This improves readability and maintainability.

**Changes:** 2. **Unified Definitions**: All device properties (ID, Name, Hardware Config, Commands) are now defined in a single `ALL_DEVICES` array in `src/deviceconfig.cpp`. 4. **Updated API**: `DeviceConfigurations` class now uses the new `ALL_DEVICES` array for lookups.

---

## December 2025 - Codebase Improvements

### 🔧 Refactoring & Optimization

**Status:** ✅ Complete

**Summary:** Major cleanup and modularization of the codebase.

**Changes:**

1.  **MatrixPanel Class:**

    -   Separated low-level hardware mapping from Core logic.
    -   Added high-level methods (`clear`, `fill`, `setCell`).

2.  **Status LED Standardization:**

    -   Implemented standard status patterns:
        -   **OK:** Solid ON.
        -   **I2C Error:** Fast Blink (5Hz).
        -   **Type Error:** Slow Blink (1Hz).

3.  **NVS Storage:**

    -   Implemented persistent storage for Device Type and Address using ESP32 Preferences (NVS).
    -   Added `loadDeviceType`, `saveDeviceType`, `loadAddress`, `saveAddress`.

4.  **Hardware Design:**
    -   Finalized pinout for Seeed XIAO ESP32-C3.
    -   Documented minimal external component requirements.

---

## December 16, 2025 - Protocol Synchronization & Cleanup

### 🔄 Protocol Alignment & Code Cleanup

**Status:** ✅ Complete

**Summary:** Synchronized the Room Bus protocol definitions between firmware (C++) and server (TypeScript), cleaned up unused code, and improved test mode behavior.

**Changes:**

1.  **Protocol Synchronization**

    -   Aligned command ranges: Server (0x40-0x7F), Event (0x80-0xFF).
    -   Documented the "Address + Command" dispatch model in `roombus.h`.

2.  **Code Cleanup**

    -   Removed unused functions: `getKeypadBitmap`, `getCellCount`, and unused `ESPTimer` helpers.
    -   Updated `RS485_USAGE.md` to reflect current command ranges.

3.  **Behavior Improvements**
    -   Updated `enterKeypadTestMode` in `core.cpp` to stop any running animations and clear pixels before starting the test.

---

## December 15, 2025 - Core device type guidance

### 📝 Added inline guidance for device types

**Status:** ✅ Complete

**Summary:** Added a maintenance note near the core type limits to remind developers where to register new device types and when to bump the potentiometer mapping limit.

**Changes:**

-   Added instruction comment near `TypeLimits` in `src/core.cpp` covering `DEVICE_TYPE_LIST`, `kConfigs`, and `MAX_CURRENT_TYPE` updates when exposing IDs above 31.

## December 9, 2025 - Device Configuration System (X-Macro Pattern)

### 🎯 Centralized Hardware Configuration with Single Source of Truth

**Status:** ✅ Complete

**Summary:** Created comprehensive device configuration system using X-Macro pattern for maintaining device types and hardware configurations in a single location. All 64 device types compiled into firmware with runtime selection via potentiometer.

**Changes:**

    - Automatically generates both `DeviceType` enum and `DEVICE_TYPE_NAMES` array
    - Eliminates duplication between enum values and display names
    - Easy maintenance: add new device by editing one line

2. **Device Configuration Database**
    - `DeviceConfig` struct defines hardware per device type:
        - Matrix cell count (0-16)
        - Motor names (0-2 motors)
        - Switch names (0-4 switches)
    - 14 device types fully configured (Terminal, GlowButton, NumBox, Timer, GlowDots, QB, RGBMixer, Purger, FinalOrder, BallGate, Actuator, TheWall, Scores, BallBase)
