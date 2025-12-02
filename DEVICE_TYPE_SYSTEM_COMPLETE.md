# Device Type Configuration System - Complete Implementation

## Overview

A robust device type configuration system for ESP32-C3 based escape room clients, supporting 32 different device types (0-31) configured via a trimmer potentiometer with persistent NVS storage.

## System Architecture

### Hardware

-   **MCU**: ESP32-C3 (Seeed XIAO)
-   **Configuration Input**: 10kΩ-25kΩ trimmer potentiometer on GPIO 2
-   **Status Indicator**: LED on GPIO 3
-   **User Interface**: Boot button on GPIO 9 (long-press for calibration)

### Firmware Layers

```
┌─────────────────────────────────────────┐
│  Application Layer (Future)             │  ← Device-specific behavior
│  - Door Controller, Window Sensor, etc. │
├─────────────────────────────────────────┤
│  Core Firmware (Current)                │  ← Generic device management
│  - Type detection & storage             │
│  - Hardware abstraction                 │
│  - Communication protocols              │
└─────────────────────────────────────────┘
```

## Key Features

### 1. Persistent Storage (NVS)

-   Device type stored in non-volatile memory
-   Survives reboots and power cycles
-   One-time configuration with manual calibration option
-   Factory reset capability

### 2. Calibration Mode

-   **Entry**: Long-press boot button (>1 second)
-   **Operation**: Adjust pot, see real-time type changes
-   **Feedback**: LED flash + beep on each reading
-   **Exit**: Long-press boot button again to save

### 3. Robust Edge Case Handling

-   ✅ Disconnected potentiometer detection
-   ✅ Invalid reading prevention
-   ✅ Type preservation during errors
-   ✅ Auto-recovery from transient issues
-   ✅ Clear error feedback (LED, serial, boot report)

### 4. Boot Report

-   Comprehensive device information on every restart
-   Shows device type, configuration status, hardware info
-   Firmware version and build timestamp
-   MAC address and system resources

## Implementation Details

### ADC Mapping (Offset Design)

**Valid Range**: ADC 200-4095 → TYPE_00 through TYPE_31

```
ADC Value     Device Type    Description
---------     -----------    -----------
0-199         INVALID        Disconnected (pull-down active)
200-321       TYPE_00        Valid minimum
322-443       TYPE_01
...           ...
3974-4095     TYPE_31        Valid maximum
```

**Formula**: `deviceType = (adcValue - 200) / 122`

**Why offset by 200?**

-   Disconnected pot (pull-down) reads ADC ~0-50
-   Creates clear separation between error and TYPE_00
-   TYPE_00 is now a valid configuration, not an error state

### Validation Logic

```cpp
bool isDisconnected = (adcValue < 200) ||   // Below valid range
                      (adcRange > 200);      // Too unstable

// Three protections:
1. ADC too low (<200) → Disconnected
2. ADC too noisy (range >200) → Bad connection
3. Invalid state (0xFF) → Error propagation
```

### State Preservation

```cpp
// On entering calibration:
m_typeBeforeCalibration = m_deviceType;  // Save current

// On exiting with error:
m_deviceType = m_typeBeforeCalibration;  // Restore original
```

**Result**: Device type NEVER changes unless explicitly calibrated with valid readings.

## Usage Scenarios

### First Boot (Pot Connected)

```
No stored device type found. Reading from ADC...
Config ADC: 1536 (range: 12) -> Type 12 (TYPE_12)
Device type 12 saved to NVS.

╔════════════════════════════════════════╗
║      ESCAPE ROOM CLIENT - BOOT REPORT  ║
╚════════════════════════════════════════╝
│ Device Type:       TYPE_12 (Index: 12)
│ Configuration:     Stored in NVS
```

### First Boot (Pot Disconnected)

```
⚠️  WARNING: Cannot read device type from ADC!
   Please check potentiometer connection.

╔════════════════════════════════════════╗
║      ESCAPE ROOM CLIENT - BOOT REPORT  ║
╚════════════════════════════════════════╝
│ Device Type:       TYPE_00 (Index: 0)
│ Configuration:     Not saved (temporary)
│ Status LED:        TYPE ERROR (slow blink)
```

### Normal Boot (Previously Configured)

```
╔════════════════════════════════════════╗
║      ESCAPE ROOM CLIENT - BOOT REPORT  ║
╚════════════════════════════════════════╝
│ Device Type:       TYPE_08 (Index: 8)
│ Configuration:     Stored in NVS
│ Operating Mode:    INTERACTIVE
│ Status LED:        OK (solid ON)
```

### Calibration Mode (Success)

```
=== ENTERING TYPE DETECTION MODE ===
Current type: TYPE_08 (8)
Type changed: TYPE_08 (8) -> TYPE_12 (12)
Type changed: TYPE_12 (12) -> TYPE_15 (15)

=== EXITING TYPE DETECTION MODE ===
Final Device Type: TYPE_15 (15)
Device type 15 saved to NVS.
```

### Calibration Mode (Pot Disconnected)

```
=== ENTERING TYPE DETECTION MODE ===
WARNING: Potentiometer disconnected or invalid reading!
Reconnect potentiometer to continue calibration.

=== EXITING TYPE DETECTION MODE ===
ERROR: Invalid device type detected!
Potentiometer may be disconnected.
Device type NOT CHANGED in NVS.
Restored previous type: TYPE_08 (8)
```

## Status LED Indicators

| Pattern                    | Meaning           | Condition                             |
| -------------------------- | ----------------- | ------------------------------------- |
| Solid ON                   | STATUS_OK         | Normal operation, all systems good    |
| Fast blink (5Hz)           | STATUS_I2C_ERROR  | I/O expander communication failed     |
| Slow blink (1Hz)           | STATUS_TYPE_ERROR | Device type configuration error       |
| Single flash (calibration) | Normal reading    | Valid ADC reading in calibration mode |
| Double flash (calibration) | Error reading     | Disconnected pot in calibration mode  |

## Audio Feedback

| Tone           | Duration        | Meaning                             |
| -------------- | --------------- | ----------------------------------- |
| NOTE_A4, 50ms  | Short high beep | Valid type reading in calibration   |
| NOTE_C4, 100ms | Longer low beep | Invalid/disconnected in calibration |
| NOTE_C5, 100ms | Medium beep     | Exiting calibration mode            |

## API Reference

### Core Class Methods

```cpp
// Get current device type (0-31)
u8 getDeviceType() const;

// Get device type name (e.g., "TYPE_12")
const char* getDeviceTypeName() const;

// Print comprehensive boot report
void printBootReport();

// Storage operations
void saveDeviceType(u8 type);
u8 loadDeviceType();
void clearStoredDeviceType();  // Factory reset
```

### Device Type Names

Default placeholder names at Core firmware level:

```cpp
const char* kDeviceTypeNames[32] = {
    "TYPE_00", "TYPE_01", ..., "TYPE_31"
};
```

Application layer can override with meaningful names:

```cpp
// In your App layer
const char* getAppTypeName(u8 type) {
    static const char* names[] = {
        "Door Controller",
        "Window Sensor",
        "RGB Light Strip",
        ...
    };
    return names[type];
}
```

## Error Handling Matrix

| Scenario                     | Detection              | Action                     | Result                      |
| ---------------------------- | ---------------------- | -------------------------- | --------------------------- |
| First boot, pot disconnected | ADC <200               | Use TYPE_00, don't save    | Error LED, temporary config |
| Normal boot, stored type OK  | NVS valid              | Load from NVS              | Normal operation            |
| Calibration, pot disconnects | ADC <200 or range >200 | Warning, don't update type | Type preserved              |
| Exit calibration with error  | m_deviceType == 0xFF   | Restore saved type         | Original config preserved   |
| Noisy connection             | Range >200             | Reject as invalid          | Type unchanged              |

## Technical Specifications

### ADC Configuration

-   **Resolution**: 12-bit (0-4095)
-   **Pin Mode**: INPUT_PULLDOWN (internal ~50kΩ)
-   **Sampling**: 32 readings averaged for stability
-   **Sample Interval**: 100μs between readings

### Valid Operating Range

-   **Minimum ADC**: 200 (disconnected if below)
-   **Maximum ADC**: 4095
-   **Step Size**: ~122 ADC units per type
-   **Maximum Noise**: 200 ADC units (rejected if higher)

### NVS Storage

-   **Namespace**: "core"
-   **Key**: "deviceType"
-   **Type**: uint8_t (0-31, or 0xFF for invalid)
-   **Lifetime**: 100,000+ write cycles

## Files Modified/Created

### Core Firmware Files

-   `include/core.h` - Core class interface
-   `src/core.cpp` - Core implementation
-   `src/main.cpp` - Boot sequence and hardware init

### Documentation

-   `DEVICE_TYPE_ARCHITECTURE.md` - System architecture and App integration
-   `NVS_DEVICE_TYPE_STORAGE.md` - Persistent storage details
-   `EDGE_CASE_HANDLING.md` - Error scenarios and handling
-   `BOOT_REPORT.md` - Boot report feature documentation
-   `ADC_VALIDATION_FIX.md` - TYPE_04 bug fix details
-   `ADC_OFFSET_MAPPING.md` - ADC offset design rationale

## Testing Checklist

-   [x] First boot with pot connected → Reads and saves type
-   [x] First boot with pot disconnected → Error state, not saved
-   [x] Normal boot with stored type → Loads from NVS
-   [x] Calibration mode entry/exit → Type changes saved
-   [x] Calibration with pot disconnect → Type preserved
-   [x] Exit calibration with invalid reading → Restores original
-   [x] Multiple calibration cycles → Type stable when pot disconnected
-   [x] TYPE_00 configuration → Valid, distinguishable from error
-   [x] All 32 types accessible → ADC 200-4095 maps correctly
-   [x] Boot report accuracy → Shows correct status

## Future Enhancements

### Planned

-   Application-specific type names via App layer
-   Multiple device profiles per type
-   Remote configuration via Room Bus
-   Type history logging for diagnostics

### Possible

-   Auto-detection based on hardware probing
-   Over-the-air type reconfiguration
-   Type-specific feature enabling/disabling
-   Configuration backup/restore via serial

## Lessons Learned

1. **Pull-down is essential** - Prevents floating pin false readings
2. **Offset mapping is critical** - Separates error state from TYPE_00
3. **State preservation matters** - Never auto-save invalid readings
4. **Delayed first read helps** - Allows pins to stabilize after mode change
5. **Multi-level validation** - Check value, range, and state
6. **Clear feedback is key** - LED + audio + serial for all conditions
7. **Boot report is invaluable** - Immediate visibility of configuration

## Summary

The device type configuration system is now **production-ready** with:

✅ Persistent NVS storage  
✅ Robust edge case handling  
✅ Clear error feedback  
✅ TYPE_00 as valid configuration  
✅ User-friendly calibration mode  
✅ Comprehensive boot reporting  
✅ Application layer extensibility

The system correctly handles all error scenarios while providing a clean, simple user experience for device configuration! 🎉

---

**Version**: 1.0.0  
**Date**: December 1, 2025  
**Status**: Production Ready ✅
