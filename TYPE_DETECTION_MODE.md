# Type Detection Mode - User Guide

## Overview

**Type Detection Mode** is a calibration/debugging feature that helps you configure the trimmer potentiometer for device type selection. When activated, the system continuously reads and displays the device type index, making it easy to adjust the trimmer to select the desired device type (0-31).

## How to Use

### Entering Type Detection Mode

1. **Long press** the boot button (hold for 1+ second)
2. Serial monitor will show:
    ```
    === ENTERING TYPE DETECTION MODE ===
    Device type will be read every second.
    LED will flash once per reading.
    Long press button again to exit.
    ```
3. The system enters calibration mode

### While in Type Detection Mode

**Every 1 second:**

-   System reads the ADC value from the trimmer pot
-   Calculates the current device type (0-31)
-   Displays on serial monitor:
    ```
    Config ADC: 1234 (1.001V) -> Type 9
    Config ADC: 1450 (1.175V) -> Type 11
    Config ADC: 2048 (1.658V) -> Type 16
    ```
-   **LED flashes once** (100ms pulse)
-   **Short beep** plays (audio feedback)

**Now you can:**

-   Adjust the trimmer potentiometer
-   Watch the device type change in real-time
-   Fine-tune to the exact type you want

### Exiting Type Detection Mode

1. **Long press** the boot button again (hold for 1+ second)
2. Serial monitor will show:
    ```
    === EXITING TYPE DETECTION MODE ===
    Config ADC: 1536 -> Type 12
    Final Device Type: 12
    ```
3. System returns to normal operation
4. Status LED returns to normal mode (solid ON or error blink)## Visual & Audio Feedback

| Feedback      | Behavior                   | Meaning                      |
| ------------- | -------------------------- | ---------------------------- |
| **LED Flash** | Single 100ms pulse         | Device type reading occurred |
| **LED Off**   | Solid OFF between readings | In type detection mode       |
| **Beep**      | Short 50ms tone (A4 note)  | Type read confirmation       |
| **Exit Tone** | 100ms C5 note              | Exiting type detection mode  |

## Serial Monitor Output Example

```
=== ENTERING TYPE DETECTION MODE ===
Adjust trimmer pot to select device type (0-31)
Type changes will be logged automatically.
LED flashes once per reading (every 1 second).
Long press button again to exit.

Type changed: 255 -> 4       ← First reading (255 = invalid initial value)
                             ← LED flashing, beeps every second
Type changed: 4 -> 6         ← Adjusting trimmer
Type changed: 6 -> 8         ← Still adjusting
Type changed: 8 -> 10        ← Getting closer
Type changed: 10 -> 12       ← Perfect! Stop here
                             ← LED keeps flashing, no more logs (type stable at 12)
                             ← Wait a few seconds to confirm stability...

=== EXITING TYPE DETECTION MODE ===
Config ADC: 1536 -> Type 12
Final Device Type: 12
```

**Note:** Voltage display removed for cleaner output. Only ADC value and type shown.

## Use Cases

### 1. Initial Setup

When first configuring a device, use this mode to:

-   Verify the trimmer pot is connected correctly
-   Set the device to the desired type
-   Confirm stable readings

### 2. Troubleshooting

If device type seems incorrect:

-   Enter type detection mode
-   Verify ADC readings are stable
-   Check for noise or intermittent connections
-   Adjust trimmer for better positioning

### 3. Multi-Device Configuration

When building multiple devices:

-   Enter type detection mode
-   Adjust trimmer to target type
-   Verify reading is stable
-   Exit and test functionality
-   Repeat for next device

## Tips for Best Results

### Stable Readings

-   ✅ Use multi-turn trimmer pot (10-25 turns)
-   ✅ Allow readings to stabilize before exiting
-   ✅ Check that adjacent readings don't fluctuate

### Troubleshooting Unstable Readings

If the type jumps between values:

-   Add 0.1µF capacitor from GPIO 2 to GND (noise filtering)
-   Check trimmer pot connections (loose wires?)
-   Use shielded cable if trimmer is remote
-   Verify power supply is clean (no voltage drops)

### Choosing Device Types

Each type spans ~128 ADC units (~103mV):

-   Aim for **middle of the range** for best stability
-   Example: For Type 10, target ~1280 ADC (1.04V)
-   Avoid extreme edges (0V or 3.3V)

## Device Type Reference Table

| Type | Voltage Range | ADC Range | Suggested Target |
| ---- | ------------- | --------- | ---------------- |
| 0    | 0.00-0.10V    | 0-127     | 0.05V (64)       |
| 1    | 0.10-0.21V    | 128-255   | 0.16V (192)      |
| 2    | 0.21-0.31V    | 256-383   | 0.26V (320)      |
| ...  | ...           | ...       | ...              |
| 10   | 1.03-1.14V    | 1280-1407 | 1.04V (1280)     |
| ...  | ...           | ...       | ...              |
| 31   | 3.20-3.30V    | 3968-4095 | 3.25V (4032)     |

_See DEVICE_TYPE_CONFIG.md for complete table_

## Technical Details

### Timing

-   **Reading interval:** 1000ms (1 second)
-   **LED flash duration:** 100ms
-   **Beep duration:** 50ms
-   **Long press threshold:** 1000ms (1 second)

### Implementation

```cpp
// Enter mode: Long press button
void Application::enterTypeDetectionMode()
{
    m_mode = MODE_TYPE_DETECTION;
    // Setup...
}

// Every second: Read & display
void Application::updateTypeDetectionMode()
{
    if (now - m_lastTypeRead >= 1000)
    {
        m_deviceType = readDeviceType();  // Re-read ADC
        // Flash LED, play beep, log to serial
    }
}

// Exit mode: Long press button again
void Application::exitTypeDetectionMode()
{
    m_mode = m_previousMode;  // Return to normal
    // Restore LED status
}
```

## Normal vs Type Detection Mode

| Feature                 | Normal Mode        | Type Detection Mode     |
| ----------------------- | ------------------ | ----------------------- |
| **Device type reading** | Once at startup    | Every 1 second          |
| **LED behavior**        | Status indication  | 100ms flash per reading |
| **Serial output**       | Initial value only | Continuous logging      |
| **Button short press**  | Cycle colors       | Disabled                |
| **Button long press**   | Enter detection    | Exit detection          |
| **Animations**          | Active if started  | Disabled                |
| **Audio**               | User feedback      | Reading beeps           |

## Integration with Other Features

### Animations

-   All animations are **stopped** when entering type detection mode
-   Pixel strip is **cleared** to avoid confusion
-   Animations can be restarted after exiting

### Status LED

-   Normal status (OK/ERROR) is **overridden** during detection
-   LED is used exclusively for reading flash indication
-   Status is **restored** when exiting detection mode

### Room Bus

-   Room Bus communication **continues** during type detection
-   Remote commands are still processed
-   Type detection is local debugging feature

## Safety & Limits

### ADC Limits

-   Input voltage: **0V to 3.3V** (do not exceed!)
-   Over-voltage may damage GPIO 2
-   Use proper trimmer pot wiring

### Operation Limits

-   Can stay in type detection mode indefinitely
-   No timeout (manual exit only)
-   Safe to adjust trimmer while in mode
-   Readings update automatically

## Quick Reference

```
┌─────────────────────────────────────────────┐
│  TYPE DETECTION MODE - QUICK GUIDE         │
├─────────────────────────────────────────────┤
│  ENTER: Long press boot button (1 sec)     │
│  EXIT:  Long press boot button again       │
│                                             │
│  WHILE ACTIVE:                              │
│  • LED flashes every 1 second               │
│  • Short beep plays every 1 second          │
│  • Serial logs ONLY when type changes       │
│  • Status LED errors suppressed             │
│  • Adjust trimmer to change type            │
│                                             │
│  SERIAL OUTPUT (only on changes):           │
│  Type changed: 8 -> 10                      │
│  Type changed: 10 -> 12                     │
│  (no output = stable reading)               │
│                                             │
│  GOAL: Adjust until desired type is stable │
└─────────────────────────────────────────────┘
```

## Example Workflow

1. **Power on device** → Reads type once at startup: `Config ADC: 1024 -> Type 8`
2. **Long press button** → Enter type detection mode
3. **Observe feedback** → LED flashing, beeps every second, no serial output (stable at Type 8)
4. **Adjust trimmer** → Turn clockwise to increase type
5. **Watch serial logs** → `Type changed: 8 -> 9` ... `Type changed: 9 -> 10` ... etc.
6. **Stop at target** → Type 12 - LED keeps flashing but no more serial (stable!)
7. **Wait for confirmation** → 3-5 seconds with no "Type changed" messages = stable
8. **Long press button** → Exit type detection mode
9. **Verify final** → Serial shows `Config ADC: 1536 -> Type 12` and `Final Device Type: 12`
10. **Done!** → Device configured for Type 12 operation

---

**Pro Tip:**

-   **No serial output = good!** It means your type is stable.
-   **LED flash + beep** confirm the system is still reading every second.
-   **Only "Type changed" logs** appear when you cross boundaries.
-   Much cleaner than continuous logging! 🎯
