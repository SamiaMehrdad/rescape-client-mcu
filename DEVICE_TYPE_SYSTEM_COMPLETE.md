# Device Type System - Complete Guide

## Overview

The device type configuration system allows each ESP32-C3 client to be configured as one of **64 device types** (0-63) using a trimmer potentiometer. Currently, types 0-31 are active, with types 32-63 reserved for future expansion.

**Key Features:**

-   ✅ 64-type capacity (32 active, 32 reserved)
-   ✅ Persistent storage (NVS - survives reboots)
-   ✅ Real-time calibration mode with feedback
-   ✅ Robust error handling (disconnected pot, noise, etc.)
-   ✅ Boot report showing device configuration
-   ✅ Status LED for system health

---

## Hardware Setup

### Components Required

| Component   | Specification   | Connection                                            |
| ----------- | --------------- | ----------------------------------------------------- |
| Trimmer Pot | 20kΩ multi-turn | Pin 1 → GND<br>Pin 2 (wiper) → GPIO 2<br>Pin 3 → 3.3V |
| Status LED  | Any color       | Anode → GPIO 3<br>Cathode → GND (via resistor)        |
| Boot Button | Built-in        | GPIO 9 (on-board)                                     |

### ADC Configuration

-   **GPIO 2** (ADC1_CH2) reads potentiometer voltage
-   **ADC Range**: 0-4095 (12-bit resolution)
-   **Step Size**: 64 ADC units per device type
-   **Disconnect Threshold**: ADC < 30 = disconnected
-   **Noise Threshold**: ADC range > 200 = bad connection

---

## Device Type Mapping

### Current Implementation (32 Types)

```
ADC Range      → Device Type  → Status
──────────────────────────────────────────
0-29           → DISCONNECTED → Error (pull-down detection)
30-63          → TYPE_00      → ✅ Active
64-127         → TYPE_01      → ✅ Active
128-191        → TYPE_02      → ✅ Active
...
1920-1983      → TYPE_30      → ✅ Active
1984-2047      → TYPE_31      → ✅ Active
2048-2111      → TYPE_32      → 🔒 Reserved
2112-2175      → TYPE_33      → 🔒 Reserved
...
4032-4095      → TYPE_63      → 🔒 Reserved
```

### Formula

```cpp
deviceType = adcValue / 64;  // Calculate 0-63

// Current clamp (remove in future for 64 types)
if (deviceType > 31)
    deviceType = 31;
```

### Future Expansion (64 Types)

Simply remove the clamp to enable all 64 types:

```cpp
deviceType = adcValue / 64;  // Already supports 0-63!

// Safety clamp at maximum
if (deviceType > 63)
    deviceType = 63;
```

See [`64_TYPE_EXPANSION.md`](64_TYPE_EXPANSION.md) for complete expansion strategy.

---

## Calibration Mode

### How to Enter

1. **Long-press** boot button (GPIO 9) for >1 second
2. System enters type detection mode
3. Serial output shows:
    ```
    === ENTERING TYPE DETECTION MODE ===
    Adjust trimmer pot to select device type (0-31)
    Type changes will be logged automatically.
    LED flashes once per reading (every 0.5 seconds).
    Long press button again to exit.
    ```

### During Calibration

**Every 0.5 seconds:**

-   Reads ADC value from potentiometer
-   Calculates current device type
-   Shows on serial monitor:
    ```
    Config ADC: 1234 (range: 12) -> Type 19 (TYPE_19)
    ```
-   **LED flashes** once (100ms)
-   **Beep** plays (50ms, A4 note)

**Type changes are logged:**

```
Type changed: TYPE_18 (18) -> TYPE_19 (19)
Type changed: TYPE_19 (19) -> TYPE_20 (20)
```

### How to Exit

1. **Long-press** boot button again (>1 second)
2. System saves to NVS and exits:
    ```
    === EXITING TYPE DETECTION MODE ===
    Config ADC: 1234 -> Type 19
    Final Device Type: 19
    Device type 19 saved to NVS.
    ```
3. Status LED returns to normal mode

### Visual Feedback

| LED Pattern       | Meaning                            |
| ----------------- | ---------------------------------- |
| Flash every 0.5s  | Normal calibration (valid reading) |
| Solid OFF         | Invalid/disconnected pot           |
| Solid ON          | Exited - all OK                    |
| Fast blink (5 Hz) | I2C communication error            |
| Slow blink (1 Hz) | Invalid device type stored         |

---

## Error Handling

### Edge Cases Handled

| Scenario              | Detection                   | Behavior                                      |
| --------------------- | --------------------------- | --------------------------------------------- |
| **Disconnected Pot**  | ADC < 30 with pull-down     | Returns 0xFF (invalid), shows "DISCONNECTED"  |
| **Noisy Connection**  | ADC range > 200             | Returns 0xFF (invalid), shows "NOISY/INVALID" |
| **Calibration Error** | Invalid reading during exit | Restores previous type, does NOT save         |
| **First Boot**        | No NVS entry                | Reads ADC, saves initial type                 |
| **Corrupt NVS**       | Type > 31                   | Treats as unconfigured, re-reads ADC          |

### Two-Phase Detection

To prevent pull-down from interfering with ADC readings:

**Phase 1: Disconnect Check**

```cpp
pinMode(CONFIG_ADC_PIN, INPUT_PULLDOWN);
if (analogRead(CONFIG_ADC_PIN) < 30) {
    return INVALID;  // Disconnected
}
```

**Phase 2: Accurate Reading**

```cpp
pinMode(CONFIG_ADC_PIN, INPUT);  // No pull-down
adcValue = averageOf32Samples();
deviceType = adcValue / 64;
```

This allows:

-   ✅ TYPE_00 (ADC 0-63) is valid and accessible
-   ✅ Disconnected pot properly detected (ADC < 30)
-   ✅ No voltage divider interference from pull-down

---

## Persistent Storage (NVS)

### Namespace and Key

```cpp
Preferences preferences;
preferences.begin("core", false);  // namespace: "core"

// Save
preferences.putUChar("deviceType", type);

// Load (returns 0xFF if not found)
u8 type = preferences.getUChar("deviceType", 0xFF);
```

### Boot Sequence

```
1. Initialize NVS
2. Try loading type from NVS
   ├─ Found (0-31) → Use stored type ✅
   ├─ Not found (0xFF) → Read from ADC, save to NVS
   └─ Invalid (>31) → Treat as unconfigured
3. Print boot report
4. Continue normal operation
```

### Factory Reset

Clear stored device type:

```cpp
core.clearStoredDeviceType();
// Reboots and re-reads from ADC
```

---

## Boot Report

On every startup, device displays comprehensive information:

```
╔════════════════════════════════════════════════════════════╗
║           ESCAPE ROOM CLIENT - BOOT REPORT                ║
╚════════════════════════════════════════════════════════════╝

┌─ DEVICE CONFIGURATION ─────────────────────────────────────┐
│ Device Type:       TYPE_19 (19)
│ Config Source:     NVS (stored configuration)
│ Config ADC:        1234
│ Status:            ✓ Configuration valid
└────────────────────────────────────────────────────────────┘

┌─ HARDWARE INFO ────────────────────────────────────────────┐
│ Board:             Seeed XIAO ESP32-C3
│ Chip Model:        ESP32-C3
│ Chip Revision:     0.3
│ CPU Frequency:     160 MHz
│ Flash Size:        4 MB
│ Free Heap:         312 KB
│ MAC Address:       AA:BB:CC:DD:EE:FF
└────────────────────────────────────────────────────────────┘

┌─ FIRMWARE INFO ────────────────────────────────────────────┐
│ Name:              Escape Room Client Core Firmware
│ Version:           1.0.0
│ Build Date:        Dec  1 2025 14:23:45
│ Architecture:      Core + App separation
└────────────────────────────────────────────────────────────┘

ℹ️  To reconfigure device type: Long-press boot button
ℹ️  Device ready for operation
```

---

## API Reference

### Core Class Methods

```cpp
// Get current device type (0-31, or 0xFF if invalid)
u8 getDeviceType() const;

// Get device type name ("TYPE_00" through "TYPE_63")
const char* getDeviceTypeName() const;

// Read device type from ADC (bypasses NVS)
u8 readDeviceType(bool verbose = true);

// Save device type to NVS
void saveDeviceType(u8 type);

// Load device type from NVS (returns 0xFF if not found)
u8 loadDeviceType();

// Clear stored type (factory reset)
void clearStoredDeviceType();

// Print boot report
void printBootReport();
```

### Usage Example

```cpp
// In your application code:
u8 type = core.getDeviceType();

if (type == 5) {
    // Custom behavior for TYPE_05
    Serial.println("Door Lock Controller");
} else if (type == 10) {
    // Custom behavior for TYPE_10
    Serial.println("Window Sensor");
}
```

---

## Type Naming Convention

### Firmware Level (Core)

Generic placeholder names:

```cpp
"TYPE_00", "TYPE_01", ... "TYPE_63"
```

### Application Level (App)

Override with meaningful names:

```cpp
const char* getAppTypeName(u8 type) {
    switch(type) {
        case 0:  return "DOOR_LOCK_MAIN";
        case 1:  return "DOOR_LOCK_BACKUP";
        case 5:  return "WINDOW_SENSOR_NORTH";
        case 10: return "MOTION_DETECTOR_HALL";
        // ... etc
    }
}
```

---

## Troubleshooting

### Problem: Can't reach TYPE_00, TYPE_01, TYPE_02

**Cause:** Disconnect threshold too high  
**Solution:** Already fixed - threshold is 30 (well below TYPE_00 range 0-63)

### Problem: Shows TYPE_04 when pot disconnected

**Cause:** Floating pin reads mid-range (~512)  
**Solution:** Already fixed - two-phase detection with pull-down check

### Problem: Can't reach TYPE_31 (stops at TYPE_30)

**Cause:** Pull-down resistor limits max voltage  
**Solution:** Already fixed - disable pull-down for accurate reading

### Problem: Device type keeps changing

**Cause:** Noisy connection or bad solder joint  
**Fix:** Check wiring, resolder connections, use shielded cable

### Problem: Boot report shows "INVALID" type

**Causes:**

1. Potentiometer not connected
2. Wiper not making contact
3. Extreme noise on ADC line

**Fix:** Enter calibration mode, adjust pot, verify stable readings

---

## Architecture Notes

### Core vs App Separation

**Core Firmware** (current implementation):

-   Hardware abstraction
-   Device type management
-   Generic I/O, communication, etc.
-   Uses placeholder type names

**App Layer** (reserved for future):

-   Application-specific logic
-   Custom type definitions
-   Puzzle behavior
-   Game state management

This architecture allows:

-   ✅ Core firmware remains stable
-   ✅ App logic can change independently
-   ✅ Same hardware, multiple applications
-   ✅ Easy to add new device types

---

## Related Documentation

-   **[64_TYPE_EXPANSION.md](64_TYPE_EXPANSION.md)** - Future expansion to 64 types
-   **[NVS_DEVICE_TYPE_STORAGE.md](NVS_DEVICE_TYPE_STORAGE.md)** - NVS implementation details
-   **[EDGE_CASE_HANDLING.md](EDGE_CASE_HANDLING.md)** - Error scenarios and solutions
-   **[BOOT_REPORT.md](BOOT_REPORT.md)** - Boot report feature details
-   **[STATUS_LED.md](STATUS_LED.md)** - Status LED modes
-   **[HARDWARE_DESIGN.md](HARDWARE_DESIGN.md)** - Hardware requirements

---

## Quick Reference

| Action             | Method                         |
| ------------------ | ------------------------------ |
| Enter calibration  | Long-press boot button         |
| Exit calibration   | Long-press boot button again   |
| Check current type | `core.getDeviceType()`         |
| Get type name      | `core.getDeviceTypeName()`     |
| Factory reset      | `core.clearStoredDeviceType()` |
| Read from pot      | `core.readDeviceType()`        |

| ADC Value | Type       | Status       |
| --------- | ---------- | ------------ |
| 0-29      | INVALID    | Disconnected |
| 30-63     | TYPE_00    | ✅ Active    |
| 1984-2047 | TYPE_31    | ✅ Active    |
| 2048+     | TYPE_32-63 | 🔒 Reserved  |

---

**Last Updated:** December 1, 2025  
**Version:** 1.0 (64-type ready, 32 active)  
**Status:** ✅ Production ready
