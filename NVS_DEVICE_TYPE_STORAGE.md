# NVS Device Type Storage

## Overview

The device type configuration is now stored persistently in **NVS (Non-Volatile Storage)** on the ESP32. This means the device "remembers" its configured type even after power cycles, eliminating the need to read the ADC trimmer pot on every boot.

## How It Works

### First Boot / Factory Reset

1. Device boots and checks NVS for stored device type
2. If **not found** (value = 0xFF):
    - Reads ADC trimmer pot (GPIO 2)
    - Saves detected type to NVS
    - Uses this type going forward

### Normal Boot

1. Device boots and checks NVS
2. If **found**:
    - Loads device type from NVS
    - Uses stored type (no ADC read)
    - Displays: `Device type loaded from NVS.`

### Type Detection Mode (Calibration)

1. Long-press boot button to enter calibration mode
2. Adjust trimmer pot to desired device type (0-31)
3. Long-press boot button again to **save and exit**
4. New type is **automatically saved to NVS**
5. Device will use this new type on all future boots

## Serial Output Examples

### First Boot (No Stored Type)

```
No stored device type found. Reading from ADC...
Config ADC: 1536 -> Type 12 (TYPE_12)
Device type saved to NVS.
Device Type: TYPE_12 (12)
Core initialized
```

### Normal Boot (Type Already Stored)

```
Device type loaded from NVS.
Device Type: TYPE_12 (12)
Core initialized
```

### Calibration Mode (Changing Type)

```
=== ENTERING TYPE DETECTION MODE ===
Adjust trimmer pot to select device type (0-31)
Type changes will be logged automatically.
LED flashes once per reading (every 1 second).
Long press button again to exit.

Type changed: TYPE_12 (12) -> TYPE_05 (5)
Type changed: TYPE_05 (5) -> TYPE_08 (8)

=== EXITING TYPE DETECTION MODE ===
Final Device Type: TYPE_08 (8)
Device type 8 saved to NVS.
```

## API Reference

### Core Class Methods

```cpp
// Load device type from NVS (returns 0xFF if not found)
u8 loadDeviceType();

// Save device type to NVS (automatically called when exiting calibration)
void saveDeviceType(u8 type);

// Clear stored device type (factory reset)
void clearStoredDeviceType();
```

### Factory Reset

To force the device to re-read from ADC on next boot:

```cpp
core.clearStoredDeviceType();
// Then reboot
ESP.restart();
```

Or manually clear NVS using serial commands (if you implement a command interface).

## Storage Details

### NVS Namespace

-   **Namespace**: `"core"`
-   **Key**: `"deviceType"`
-   **Type**: Unsigned 8-bit integer (0-31, or 0xFF for "not set")

### Memory Usage

-   NVS uses flash memory (non-volatile)
-   Very small footprint (~1 byte of data + metadata)
-   100,000+ write cycles lifetime
-   No wear concern for occasional reconfiguration

### Advantages Over ADC Reading

| Feature       | ADC Every Boot                     | NVS Storage                 |
| ------------- | ---------------------------------- | --------------------------- |
| Boot time     | Slower (ADC sampling)              | Faster (direct read)        |
| Configuration | Changes on pot movement            | Stable until calibration    |
| Accuracy      | Subject to noise                   | Perfect recall              |
| User intent   | Unclear (accidental pot movement?) | Explicit (calibration mode) |
| Power cycles  | Must re-read                       | Remembers setting           |

## Use Cases

### Production Deployment

1. Build firmware with generic Core
2. Flash to all devices
3. Use trimmer pot + calibration mode to set each device's type
4. Type is locked in NVS until you explicitly recalibrate

### Field Service

-   Technician can recalibrate device type on-site
-   No need to reflash firmware
-   Just enter calibration mode, adjust pot, exit

### Development

-   Quickly test different device types
-   No need to physically adjust pot between tests
-   Change type in calibration mode
-   Reboot preserves setting

## Important Notes

### Trimmer Pot Role

-   **Before**: Critical - device type read every boot
-   **Now**: Calibration only - used during type detection mode

The trimmer pot is now only read when:

1. First boot (NVS empty)
2. Explicitly entering type detection mode (long-press)
3. Factory reset (clearStoredDeviceType() called)

### Migration from Old Behavior

If you're upgrading from a firmware that always read the ADC:

-   First boot will act as calibration
-   Device reads pot once, saves to NVS
-   All future boots use saved value
-   No user action required

### Backup Strategy

Consider logging the device type and serial number during initial deployment:

```
Device Serial: ESP32-XXXXXX
Device Type: TYPE_12 (12)
Configured: 2025-12-01
```

This helps if you need to reconfigure a device after firmware update or factory reset.

## Summary

✅ **Persistent storage** - Device type survives reboots  
✅ **Faster boots** - No ADC sampling on normal startup  
✅ **Explicit configuration** - Type changes only in calibration mode  
✅ **User-friendly** - Clear feedback when saving/loading  
✅ **Factory reset option** - Can clear and recalibrate if needed

The device type is now a stable configuration parameter rather than a volatile ADC reading!
