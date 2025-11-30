# Escape Room Controller - Changelog & Progress

## Project Overview

ESP32-based escape room controller with keypad, motors, LEDs, audio, and RS-485 communication.

**Hardware:**

-   Board: Seeed XIAO ESP32-C3
-   I/O Expander: PCF8575 (16-bit I2C)
-   Keypad: 4x4 matrix (16 keys)
-   Motors: 2x DC motors via H-bridge
-   Switches: 4x digital inputs
-   LEDs: WS2812B RGB strip (configurable groups)
-   Audio: PWM synthesizer with ADSR envelope
-   Communication: RS-485 Room Bus

---

## November 29, 2025 - Major Architecture Refactoring

### 🎉 Complete Code Modernization

**Status:** ✅ Complete

**Summary:** Transformed monolithic 300-line main.cpp into clean, modular architecture with proper separation of concerns, event-driven design, and professional code organization.

---

### New Modular Architecture

#### 1. Animation Module (`animation.h/cpp`)

**Purpose:** Encapsulate all LED animation logic

**Features:**

-   Multiple animation types: Red Dot Chase, Rainbow Cycle, Breathing, Sparkle
-   Clean API: `start()`, `stop()`, `update()`, `isActive()`
-   Frame-based timing system
-   Easy to extend with new patterns

**Benefits:**

-   All animation logic in one place
-   No animation state in main.cpp
-   Simple to add new effects

---

#### 2. InputHandler Module (`inputhandler.h/cpp`)

**Purpose:** Unified abstraction for all input sources

**Features:**

-   Event-driven callback pattern
-   Handles buttons, keypad (4x4), switches
-   Proper short/long press detection
-   Graceful degradation if I/O expander missing

**Implementation:**

-   Uses `keyReleased()` to detect short press (not `keyPressed()`)
-   Tracks `wasLongPress` flag to prevent conflicts
-   Short press triggers only on button release if not long press
-   Long press triggers when threshold reached while held

**Benefits:**

-   Clean separation of input detection and action
-   Easy to add new input types
-   No input logic in main.cpp

---

#### 3. Application Module (`app.h/cpp`)

**Purpose:** High-level application logic coordinator

**Features:**

-   Application mode management (Interactive, Animation, Remote)
-   Event handlers for all user interactions
-   Color palette management
-   Room Bus command processing
-   Sound feedback coordination

**Benefits:**

-   All business logic in one place
-   Clean event routing
-   Easy to modify behavior

---

#### 4. Refactored Main (`main.cpp`)

**Before:** ~300 lines of mixed hardware and logic code  
**After:** ~160 lines of clean initialization and delegation

**Structure:**

-   Hardware objects section
-   Application modules section
-   ISR section
-   Setup: Initialize all modules
-   Loop: Delegate to modules (15 lines)

**Benefits:**

-   Clean, readable, self-documenting
-   Easy to understand flow
-   Minimal cognitive load

---

### Critical Bug Fixes

#### 1. Pixel Update Flag Bug

**Issue:** `if (~ pixelUpdateFlag)` should be `if (pixelUpdateFlag)`  
**Fix:** Changed bitwise NOT to proper boolean check  
**Impact:** Pixel updates now work correctly

#### 2. Button Press Conflicts

**Issue:** Short press and long press both triggered  
**Root Cause:** `keyPressed()` fires immediately on press, before long press threshold  
**Fix:** Use `keyReleased()` with `wasLongPress` tracking  
**Impact:** Clean separation of short/long press events

#### 3. Missing Pixel Display Updates

**Issue:** Colors set but not displayed  
**Root Cause:** Missing `pixels.show()` calls after manual color changes  
**Fix:** Added `pixels.show()` in button handlers  
**Impact:** Color changes now visible immediately

#### 4. I2C Error Spam

**Issue:** Continuous I2C errors when expander not connected  
**Root Cause:** Polling I/O expander every 10ms even when not present  
**Fix:** Check I/O expander presence at startup, disable polling if absent  
**Impact:** Clean serial output, graceful degradation

---

### API Improvements

#### Static Array Definitions

**Issue:** `static constexpr` arrays in headers caused linker errors  
**Fix:** Moved to source files with proper external linkage  
**Files:** `app.cpp` - kColors[], kNoteMap[]

#### Include Dependencies

**Issue:** Missing color constant definitions  
**Fix:** Added `#include "colors.h"` to app.h

---

### Performance & Behavior

#### Current System Behavior

**Short Press (BTN1):** Cycle color backward + sound  
**Short Press (BTN2):** Cycle rainbow pattern forward + sound  
**Long Press (either):** Toggle animation on/off  
**Keypad:** Play musical notes (if I/O expander present)  
**Room Bus:** Remote color control

#### Timing Characteristics

-   ISR frequency: 200 Hz (5ms interval)
-   Button update: 200 Hz (in ISR)
-   Pixel refresh: 25 Hz (40ms interval)
-   Animation step: 20 Hz (50ms per step)
-   Main loop: ~100 Hz (10ms delay)
-   I/O expander poll: 100 Hz (when present)

---

### Code Quality Improvements

**Modularity:**

-   Each module has single, clear responsibility
-   Well-defined interfaces
-   Low coupling, high cohesion

**Extensibility:**

-   New animation: Add to Animation module
-   New input: Extend InputHandler
-   New behavior: Add handler in Application
-   New hardware: Initialize in main.cpp

**Maintainability:**

-   Changes localized to specific modules
-   Easy to understand code flow
-   Self-documenting structure

**Reliability:**

-   Graceful degradation (I/O expander optional)
-   Proper ISR safety (IRAM_ATTR, volatile)
-   Watchdog protection
-   Clean error handling

---

### Documentation

**New Files:**

-   `REFACTORING.md` - Complete architecture documentation
-   Updated `CHANGELOG.md` - This document

**Code Comments:**

-   Section headers in main.cpp
-   Function documentation
-   Architecture explanations

---

### Testing Results

✅ Compiles without errors or warnings  
✅ Buttons trigger color changes correctly  
✅ Long press toggles animation  
✅ Short/long press properly separated  
✅ Animation runs smoothly  
✅ Watchdog doesn't trigger  
✅ Clean serial output (no I2C spam)  
✅ Works with or without I/O expander  
✅ Keypad plays notes (when expander present)  
✅ Room Bus commands work

---

## Previous Updates - November 29, 2025

### Major Improvements

#### 1. I/O Expander Library Refactoring

**Status:** ✅ Complete

**Changes:**

-   Renamed from `PCF8575` to `IOExpander` for generic naming
-   Changed API to return key index (0-15) instead of character
-   Added `getKeypadBitmap()` for multi-key detection
-   Renamed motor pins: `MOTOR_A/B_IN1/IN2` → `MOT1A/MOT1B/MOT2A/MOT2B`
-   Renamed sensor pins: `SENSOR_1-4` → `SW_1-4`
-   Added `readSwitch3()` and `readSwitch4()` methods

**Benefits:**

-   More flexible key handling (index-based instead of char-based)
-   Clearer hardware naming conventions
-   Support for simultaneous key detection via bitmap

---

#### 2. Keypad Debouncing Implementation

**Status:** ✅ Complete

**Implementation:**

-   Rate limiting: 10ms between scans (slower, more reliable)
-   Stability verification: 3 consecutive identical reads (~30ms total)
-   State machine: Only triggers on press→release→press transitions
-   No auto-repeat: Prevents accidental double-presses

**Technical Details:**

```cpp
// Debounce state tracking (private members)
u8 _stableKeyIndex;    // Last confirmed stable key
u8 _rawKeyIndex;       // Current raw reading
u8 _debounceCount;     // Consecutive stable reads
unsigned long _lastScanTime;  // Rate limiting
```

**Performance:**

-   I2C frequency: 100 kHz (default Wire speed, excellent reliability)
-   Scan rate: ~100 Hz (limited to 10ms intervals)
-   Debounce settle time: ~30ms (3 reads × 10ms)

---

#### 3. Pixel Strip Animation System

**Status:** ✅ Complete

**Features:**

-   Color buffer system for smooth animations
-   ISR-triggered display updates (configurable rate)
-   Compile-time timing calculations
-   Zero runtime overhead

**Architecture:**

```cpp
// Timing hierarchy
ISR_INTERVAL_MS = 5      // ISR runs at 200 Hz
ANIM_REFRESH_MS = 40     // Buffer applied at 25 Hz
ANIM_STEP_MS = 50        // Animation step at 20 Hz

// Automatic calculation
pixelUpdateFlag set every (ANIM_REFRESH_MS / ISR_INTERVAL_MS) ISR ticks
animationPosition advances every (ANIM_STEP_MS / ANIM_REFRESH_MS) buffer updates
```

**API Changes:**

-   Constructor now takes logical pixel count, not physical LED count
-   `count` parameter = number of logical groups you control
-   Physical LEDs = `count × groupSize`
-   Added `getBuffer()` for direct buffer access
-   Added `applyBuffer()` for ISR-safe display updates

**Example:**

```cpp
PixelStrip pixels(PIXEL_PIN, 10, 4, 5);  // 10 logical pixels, 4 LEDs each = 40 physical
u32 *buffer = pixels.getBuffer();
buffer[0] = CLR_RD;  // Modify buffer
// ISR automatically applies at ANIM_REFRESH_MS rate
```

---

#### 4. ESPTimer Library

**Status:** ✅ Complete

**Purpose:** Flexible hardware timer management for ESP32

**Features:**

-   Support for up to 4 hardware timers
-   Millisecond and microsecond precision
-   Auto-reload or one-shot modes
-   Clean callback-based API

**API:**

```cpp
// Initialize timer
hw_timer_t *timer = ESPTimer::begin(0, 10, &callback);  // Timer 0, 10ms, auto-reload

// Control
ESPTimer::stop(timer);           // Pause
ESPTimer::start(timer);          // Resume
ESPTimer::setInterval(timer, 20); // Change to 20ms
ESPTimer::end(timer);            // Stop and free

// Microsecond precision
timer = ESPTimer::beginMicros(1, 500, &callback);  // 500μs
```

**Files:**

-   `include/esptimer.h` - Header with API documentation
-   `src/esptimer.cpp` - Implementation

---

#### 5. I2C Debug Spam Fix

**Status:** ✅ Complete

**Problem:** Wire library was flooding Serial with I2C error messages during normal operation

**Solution:**

```cpp
#include "esp_log.h"
esp_log_level_set("i2c", ESP_LOG_NONE);  // Disable I2C logging
```

**Options:**

-   `ESP_LOG_NONE` - No logging (current)
-   `ESP_LOG_ERROR` - Only errors
-   `ESP_LOG_WARN` - Warnings and errors
-   `ESP_LOG_INFO` - Info level and above

---

#### 6. Code Quality Improvements

**Naming Conventions:**

-   ISR renamed: `onTimer()` → `refreshTimer()`
-   More descriptive constant names
-   Self-documenting timing relationships

**Timing Constants:**

```cpp
constexpr u8 ISR_INTERVAL_MS = 5;    // How often ISR runs
constexpr u8 ANIM_REFRESH_MS = 40;   // How often to update display
constexpr u16 ANIM_STEP_MS = 50;     // How often to advance animation

// Relationships calculated at compile time
ANIM_REFRESH_MS / ISR_INTERVAL_MS    // ISR ticks per display update
ANIM_STEP_MS / ANIM_REFRESH_MS       // Display updates per animation step
```

**Benefits:**

-   Easy to understand and modify
-   No magic numbers
-   Compile-time verification
-   Zero runtime cost

---

### File Structure

```
esp32-hello/
├── include/
│   ├── buttons.h          - Debounced button input
│   ├── colors.h           - RGB color definitions
│   ├── esptimer.h         - ✨ NEW: Hardware timer helper
│   ├── ioexpander.h       - 🔄 REFACTORED: Generic I/O expander
│   ├── mcupins.h          - Pin definitions
│   ├── pixel.h            - 🔄 ENHANCED: Animation buffer system
│   ├── roombus.h          - Room Bus protocol
│   ├── roomserial.h       - RS-485 communication
│   ├── synth.h            - Audio synthesizer
│   └── watchdog.h         - Watchdog timer
├── src/
│   ├── buttons.cpp
│   ├── esptimer.cpp       - ✨ NEW: Timer implementation
│   ├── ioexpander.cpp     - 🔄 REFACTORED: With debouncing
│   ├── main.cpp           - 🔄 UPDATED: Uses new APIs
│   ├── pixel.cpp          - 🔄 ENHANCED: Buffer system
│   ├── roomserial.cpp
│   ├── synth.cpp
│   └── watchdog.cpp
└── platformio.ini
```

---

### Testing Notes

**Tested Features:**

-   ✅ Keypad scanning with debouncing (no double-presses)
-   ✅ Pixel animation system (smooth, precise timing)
-   ✅ ESPTimer library (200 Hz ISR, 25 Hz display, 20 Hz animation)
-   ✅ I2C communication (clean Serial output, no spam)
-   ✅ Button responsiveness (immediate trigger on press)

**Timing Verification:**

-   Animation step timing: 250ms measured vs 250ms expected ✅
-   No jitter or drift observed
-   Compile-time calculations verified

**Performance:**

-   ISR execution time: Minimal (button update + flag set)
-   No blocking in ISR (NeoPixel update moved to main loop)
-   System remains responsive during animations

---

### Known Issues

None currently identified.

---

### Next Steps / Ideas

**Potential Improvements:**

1. Add more animation patterns (fade, rainbow chase, etc.)
2. Implement interrupt-based keypad reading (using INT pin)
3. Add configuration system (EEPROM/Flash storage)
4. Create animation library with predefined effects
5. Add motor control examples (stepper motors, servos)
6. Implement Room Bus command handling
7. Add real-time clock support (RTC module)
8. Create web interface for configuration

**Hardware Expansion:**

-   RFID reader integration
-   LCD/OLED display support
-   More sensor types (ultrasonic, PIR, etc.)
-   Additional I/O expanders for more inputs

---

### Development Environment

**Platform:** PlatformIO **Board:** Seeed XIAO ESP32-C3 **Framework:** Arduino **Compiler:** ESP32 Arduino Core **Upload:** USB (CDC)

---

### Notes

-   All timing constants use compile-time calculations for efficiency
-   ISR functions marked with `IRAM_ATTR` for performance
-   Buffer-based animation allows complex effects without blocking
-   Debouncing hidden at library level for clean main code
-   Generic naming improves code maintainability and reusability

---

### Contributors

-   MSK - November 2025

---

**Last Updated:** November 29, 2025
