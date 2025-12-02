# Boot Report Feature

## Overview

The system now displays a comprehensive boot report on every restart, showing the device configuration, hardware info, and firmware details. This makes it easy to identify devices and diagnose configuration issues.

## Boot Report Format

### Complete Example Output

```
=== ESP32 Escape Room Client Starting ===
I2C initialized
I/O Expander initialized
Pixel strip initialized
Button initialized
Timer initialized
Watchdog initialized
Synthesizer initialized
Room Bus initialized
Status LED: OK mode (solid ON)
=== System Ready ===

╔════════════════════════════════════════════════════════════╗
║           ESCAPE ROOM CLIENT - BOOT REPORT                ║
╚════════════════════════════════════════════════════════════╝

┌─ DEVICE CONFIGURATION ─────────────────────────────────────┐
│ Device Type:       TYPE_12 (Index: 12)
│ Configuration:     Stored in NVS
│ Operating Mode:    INTERACTIVE (manual control)
│ Status LED:        OK (solid ON)
└────────────────────────────────────────────────────────────┘

┌─ HARDWARE INFO ────────────────────────────────────────────┐
│ Chip:              ESP32-C3 @ 160 MHz
│ Flash:             4 MB
│ Free Heap:         285472 bytes
│ MAC Address:       34:85:18:ab:cd:ef
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

## Report Sections

### 1. Device Configuration

**Device Type**:

-   Shows human-readable name (e.g., `TYPE_12`)
-   Shows numeric index (0-31)
-   Makes it easy to identify device purpose

**Configuration Status**:

-   `Stored in NVS` - Type is saved and will persist across reboots
-   `Not saved (temporary)` - Type couldn't be read from ADC (potentiometer issue)
-   `(warning: mismatch!)` - Current type doesn't match stored (shouldn't happen)

**Operating Mode**:

-   `INTERACTIVE` - Manual button/keypad control
-   `ANIMATION` - Running automated sequences
-   `REMOTE` - Controlled via Room Bus
-   `TYPE_DETECTION` - In calibration mode

**Status LED**:

-   `OK (solid ON)` - All systems normal
-   `I2C ERROR (fast blink)` - I/O expander communication failed
-   `TYPE ERROR (slow blink)` - Device type couldn't be configured

---

### 2. Hardware Info

**Chip Information**:

-   ESP32 variant (C3, S2, etc.)
-   CPU frequency in MHz
-   Helps verify board type

**Flash Size**:

-   Total flash memory available
-   Useful for capacity planning

**Free Heap**:

-   Available RAM at boot
-   Useful for debugging memory issues
-   Typical value: ~280KB-320KB on ESP32-C3

**MAC Address**:

-   Unique hardware identifier
-   Useful for network configuration
-   Can be used to track specific devices

---

### 3. Firmware Info

**Name & Version**:

-   Clear identification of firmware
-   Version tracking for updates

**Build Date/Time**:

-   When firmware was compiled
-   Helps verify you're running latest version
-   Format: `MMM DD YYYY HH:MM:SS`

**Architecture**:

-   Shows firmware design philosophy
-   "Core + App separation" indicates modular design

---

## Use Cases

### 1. Device Identification

When managing multiple devices:

```
Device Type:       TYPE_12 (Index: 12)
MAC Address:       34:85:18:ab:cd:ef
```

Easy to identify which physical device you're connected to.

---

### 2. Configuration Verification

After calibrating device type:

```
Device Type:       TYPE_08 (Index: 8)
Configuration:     Stored in NVS
```

Confirms the new type was saved successfully.

---

### 3. Troubleshooting I2C Issues

When I/O expander fails:

```
Status LED:        I2C ERROR (fast blink)
```

Combined with hardware section showing system is otherwise healthy.

---

### 4. Potentiometer Problems

When ADC can't read device type:

```
Device Type:       TYPE_00 (Index: 0)
Configuration:     Not saved (temporary)
Status LED:        TYPE ERROR (slow blink)
```

Clear indication of hardware problem.

---

### 5. Firmware Version Tracking

When deploying updates:

```
Version:           1.0.0
Build Date:        Dec  1 2025 14:23:45
```

Verify you're running the expected firmware version.

---

### 6. Memory Debugging

When investigating crashes:

```
Free Heap:         285472 bytes
```

Low heap might indicate memory leak or excessive allocation.

---

## Boot Report Timing

The report is printed **after** all hardware initialization completes:

1. Arduino `setup()` runs
2. All peripherals initialize (I2C, SPI, UART, etc.)
3. Core firmware initializes
4. Status LED configured
5. **Boot report printed** ← Final step before entering main loop
6. Main `loop()` begins

This ensures the report shows the actual configured state, not initialization-in-progress.

---

## Customization for App Layer

High-level application code can extend the boot report:

```cpp
// In your App layer, after core.printBootReport()
void App::printAppInfo() {
    Serial.println("┌─ APPLICATION INFO ─────────────────────────────────────────┐");

    switch(core.getDeviceType()) {
        case 0:
            Serial.println("│ Application:       Door Controller v2.1");
            Serial.println("│ Features:          Magnetic lock, LED status, RFID");
            break;
        case 1:
            Serial.println("│ Application:       Window Sensor v1.5");
            Serial.println("│ Features:          Reed switch, tamper detect");
            break;
        // ... etc
    }

    Serial.println("└────────────────────────────────────────────────────────────┘");
}
```

---

## Reducing Serial Spam

The boot report replaces multiple verbose initialization messages:

**Before** (verbose):

```
No stored device type found. Reading from ADC...
Config ADC: 1536 -> Type 12 (TYPE_12)
Device type saved to NVS.
Device Type: TYPE_12 (12)
Device type loaded from NVS.
Core initialized
```

**After** (clean):

```
⚠️  WARNING: Cannot read device type from ADC!
   Please check potentiometer connection.

[Then comprehensive boot report shows everything]
```

Only critical errors shown during init; detailed info in structured report.

---

## Boot Report API

### Method

```cpp
void Core::printBootReport() const;
```

### Usage

```cpp
void setup() {
    // ... hardware initialization ...

    core.init();

    // Print boot report (shows device type, hardware, firmware info)
    core.printBootReport();
}
```

### Thread Safety

-   Safe to call from main thread only
-   Uses `Serial.print()` - not interrupt-safe
-   Should only be called once at boot

---

## Error Indicators in Report

### Configuration Not Saved

```
Configuration:     Not saved (temporary)
Status LED:        TYPE ERROR (slow blink)
```

**Meaning**: Potentiometer disconnected or invalid reading  
**Action**: Check hardware, enter calibration mode

---

### I2C Communication Failed

```
Status LED:        I2C ERROR (fast blink)
```

**Meaning**: I/O expander not responding  
**Action**: Check I2C wiring, verify I2C address

---

### Type Mismatch (Rare)

```
Configuration:     Stored in NVS (warning: mismatch!)
```

**Meaning**: Internal inconsistency (shouldn't happen)  
**Action**: Factory reset, recalibrate device type

---

## Serial Output Best Practices

### During Development

Keep boot report enabled - helps track device state and debug issues.

### Production Deployment

Boot report is still useful for:

-   Initial device setup verification
-   Field service diagnostics
-   Remote debugging via serial console
-   Logging to file for audit trail

If serial output is a concern, you can:

1. Reduce baud rate after boot report
2. Disable serial after report completes
3. Buffer report to send over network instead

---

## Example Scenarios

### Scenario 1: Fresh Device First Boot

```
╔════════════════════════════════════════════════════════════╗
║           ESCAPE ROOM CLIENT - BOOT REPORT                ║
╚════════════════════════════════════════════════════════════╝

┌─ DEVICE CONFIGURATION ─────────────────────────────────────┐
│ Device Type:       TYPE_15 (Index: 15)
│ Configuration:     Stored in NVS                  ← Just saved
│ Operating Mode:    INTERACTIVE (manual control)
│ Status LED:        OK (solid ON)
└────────────────────────────────────────────────────────────┘

ℹ️  To reconfigure device type: Long-press boot button
```

---

### Scenario 2: Device with Disconnected Potentiometer

```
⚠️  WARNING: Cannot read device type from ADC!
   Please check potentiometer connection.

╔════════════════════════════════════════════════════════════╗
║           ESCAPE ROOM CLIENT - BOOT REPORT                ║
╚════════════════════════════════════════════════════════════╝

┌─ DEVICE CONFIGURATION ─────────────────────────────────────┐
│ Device Type:       TYPE_00 (Index: 0)            ← Default
│ Configuration:     Not saved (temporary)         ← Warning!
│ Operating Mode:    INTERACTIVE (manual control)
│ Status LED:        TYPE ERROR (slow blink)       ← Error indicator
└────────────────────────────────────────────────────────────┘
```

---

### Scenario 3: Normal Boot (Previously Configured)

```
╔════════════════════════════════════════════════════════════╗
║           ESCAPE ROOM CLIENT - BOOT REPORT                ║
╚════════════════════════════════════════════════════════════╝

┌─ DEVICE CONFIGURATION ─────────────────────────────────────┐
│ Device Type:       TYPE_08 (Index: 8)
│ Configuration:     Stored in NVS                  ← Loaded from storage
│ Operating Mode:    INTERACTIVE (manual control)
│ Status LED:        OK (solid ON)
└────────────────────────────────────────────────────────────┘

┌─ HARDWARE INFO ────────────────────────────────────────────┐
│ Chip:              ESP32-C3 @ 160 MHz
│ Flash:             4 MB
│ Free Heap:         285472 bytes
│ MAC Address:       34:85:18:ab:cd:ef
└────────────────────────────────────────────────────────────┘
```

---

## Summary

✅ **Clear device identification** - Know what device you're working with  
✅ **Configuration verification** - Confirm device type is saved  
✅ **Hardware diagnostics** - See chip info, memory, MAC address  
✅ **Firmware tracking** - Version and build timestamp  
✅ **Error visibility** - Immediate notification of configuration issues  
✅ **User-friendly** - Structured, easy-to-read format  
✅ **Extensible** - App layer can add custom sections

The boot report makes device management, deployment, and troubleshooting much easier!
