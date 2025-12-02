# Edge Case Handling - Device Type Configuration

## Overview

The device type configuration system now includes robust edge case handling to prevent invalid configurations and provide clear feedback when hardware issues occur.

## Edge Cases Handled

### 1. Device Never Configured (Potentiometer Disconnected on First Boot)

**Scenario**: Device boots for the first time with no stored type in NVS, and the potentiometer is disconnected or damaged.

**Detection**:

-   ADC reads very high value (>4000 out of 4095)
-   ADC readings are unstable (range >500 between samples)

**Behavior**:

```
No stored device type found. Reading from ADC...
Config ADC: 4095 (range: 234) -> DISCONNECTED/INVALID
ERROR: Cannot read device type from ADC!
Please check potentiometer connection and enter calibration mode.
Using default type TYPE_00 (0) - NOT SAVED to NVS.
Device Type: TYPE_00 (0)
```

**Result**:

-   ✅ Device uses TYPE_00 (0) as temporary default
-   ✅ Invalid type is **NOT saved** to NVS
-   ✅ Status LED shows **TYPE_ERROR** (slow 1Hz blink)
-   ✅ User can enter calibration mode to fix

---

### 2. Potentiometer Disconnects During Calibration

**Scenario**: User is in calibration mode adjusting the device type, and the potentiometer wire disconnects or pot fails.

#### 2a. Disconnect Detected During Calibration

**Behavior**:

```
Type changed: TYPE_05 (5) -> TYPE_08 (8)
WARNING: Potentiometer disconnected or invalid reading!
Reconnect potentiometer to continue calibration.
WARNING: Potentiometer disconnected or invalid reading!  (repeats every 1 second)
```

**Feedback**:

-   ⚠️ Serial warning messages
-   💡 LED shows **double blink pattern** (error indicator)
-   🔊 Lower pitch beep (NOTE_C4 instead of NOTE_A4)
-   🚫 Device type does **NOT change** to invalid value

#### 2b. User Exits Calibration While Disconnected

**Behavior**:

```
=== EXITING TYPE DETECTION MODE ===
ERROR: Invalid device type detected!
Potentiometer may be disconnected.
Device type NOT CHANGED in NVS.
Restored previous type: TYPE_05 (5)
```

**Result**:

-   ✅ Previous valid type is **restored** from NVS
-   ✅ Invalid type is **NOT saved**
-   ✅ Device continues using last known good configuration
-   ✅ User can re-enter calibration mode after fixing hardware

#### 2c. Potentiometer Reconnects During Calibration

**Behavior**:

```
WARNING: Potentiometer disconnected or invalid reading!
Potentiometer reconnected. Type: TYPE_08 (8)
Type changed: TYPE_08 (8) -> TYPE_12 (12)
```

**Result**:

-   ✅ Automatic recovery when valid signal detected
-   ✅ User can continue calibration normally
-   ✅ LED returns to normal single blink pattern
-   ✅ Beep returns to normal pitch

---

## Validation Logic

### ADC Reading Validation (`readDeviceType()`)

```cpp
// Track min/max to detect instability
int minReading = 4095;
int maxReading = 0;

// Calculate range across 32 samples
int adcRange = maxReading - minReading;

// Detect disconnection
bool isDisconnected = (adcValue > 4000) || (adcRange > 500);

if (isDisconnected) {
    return 0xFF;  // Invalid marker
}
```

**Thresholds**:

-   `adcValue > 4000`: Floating pin reads near max (4095)
-   `adcRange > 500`: Excessive noise indicates bad connection

**Valid range**: 0-4000 with stability <500 units

---

## Serial Output Guide

### Normal First Boot

```
No stored device type found. Reading from ADC...
Config ADC: 1536 (range: 12) -> Type 12 (TYPE_12)
Device type saved to NVS.
Device Type: TYPE_12 (12)
```

### Disconnected First Boot

```
No stored device type found. Reading from ADC...
Config ADC: 4095 (range: 567) -> DISCONNECTED/INVALID
ERROR: Cannot read device type from ADC!
Please check potentiometer connection and enter calibration mode.
Using default type TYPE_00 (0) - NOT SAVED to NVS.
Device Type: TYPE_00 (0)
```

### Calibration with Disconnect Mid-Session

```
=== ENTERING TYPE DETECTION MODE ===
...
Type changed: TYPE_05 (5) -> TYPE_08 (8)
WARNING: Potentiometer disconnected or invalid reading!
Reconnect potentiometer to continue calibration.
Potentiometer reconnected. Type: TYPE_08 (8)
Type changed: TYPE_08 (8) -> TYPE_10 (10)

=== EXITING TYPE DETECTION MODE ===
Final Device Type: TYPE_10 (10)
Device type 10 saved to NVS.
```

### Exiting Calibration While Disconnected

```
=== EXITING TYPE DETECTION MODE ===
ERROR: Invalid device type detected!
Potentiometer may be disconnected.
Device type NOT CHANGED in NVS.
Restored previous type: TYPE_08 (8)
```

---

## LED Indicators

| State                     | LED Pattern              | Description                   |
| ------------------------- | ------------------------ | ----------------------------- |
| Normal boot (valid)       | Solid ON or fast blink   | STATUS_OK or STATUS_I2C_ERROR |
| First boot disconnected   | Slow 1Hz blink           | STATUS_TYPE_ERROR             |
| Calibration valid reading | Single 100ms flash       | Normal calibration feedback   |
| Calibration disconnected  | Double blink (50ms+50ms) | Error - reconnect pot         |

---

## Audio Feedback

| State                | Tone           | Description             |
| -------------------- | -------------- | ----------------------- |
| Valid type reading   | NOTE_A4, 50ms  | Short high beep         |
| Invalid/disconnected | NOTE_C4, 100ms | Longer low beep (error) |
| Exit calibration     | NOTE_C5, 100ms | Exit confirmation       |

---

## Troubleshooting

### Device shows TYPE_00 with slow blinking LED

**Problem**: Potentiometer disconnected or invalid on first boot

**Solution**:

1. Check potentiometer wiring to GPIO 2
2. Verify 10kΩ-25kΩ pot is connected correctly
3. Enter calibration mode (long-press boot button)
4. Adjust pot to desired type
5. Long-press boot button to save

### Device won't save new type in calibration mode

**Problem**: Potentiometer disconnects during calibration exit

**Solution**:

1. Re-enter calibration mode
2. Ensure potentiometer is firmly connected
3. Wait for stable reading (normal single beeps)
4. Exit calibration mode
5. Verify "Device type X saved to NVS" message

### Constant "WARNING: Potentiometer disconnected" messages

**Problem**: Bad connection or damaged potentiometer

**Solution**:

1. Exit calibration mode (device restores previous type)
2. Check physical connections
3. Test potentiometer with multimeter (should vary 10kΩ-25kΩ)
4. Replace potentiometer if damaged
5. Re-enter calibration after hardware fix

---

## Implementation Details

### Invalid Type Marker

-   Uses `0xFF` (255) as invalid type marker
-   Valid types are 0-31
-   `0xFF` never saved to NVS
-   `0xFF` triggers special handling logic

### State Tracking in Calibration

```cpp
m_lastDetectedType = loadDeviceType(); // Start from current stored value

// During calibration:
if (newType == 0xFF) {
    // Invalid - don't update m_deviceType
    // Show error feedback
} else {
    // Valid - update m_deviceType
    m_deviceType = newType;
}
```

### Exit Validation

```cpp
if (m_deviceType == 0xFF) {
    // Restore from NVS
    m_deviceType = loadDeviceType();
    // Don't save invalid value
} else {
    // Save valid value
    saveDeviceType(m_deviceType);
}
```

---

## Testing Recommendations

### Test Case 1: First Boot No Pot

1. Flash firmware to new device
2. Clear NVS: `m_preferences.clear()`
3. Disconnect potentiometer
4. Reboot
5. **Expected**: Default TYPE_00, error LED, not saved

### Test Case 2: Disconnect During Calibration

1. Enter calibration mode
2. Adjust pot to TYPE_05
3. Disconnect pot wire
4. **Expected**: Double blink LED, error beep, warnings
5. Reconnect pot
6. **Expected**: Recovery message, normal feedback
7. Exit calibration
8. **Expected**: TYPE saved successfully

### Test Case 3: Exit While Disconnected

1. Enter calibration mode with TYPE_05 stored
2. Disconnect pot
3. Long-press to exit
4. **Expected**: Error message, TYPE_05 restored, not changed

---

## Summary

✅ **Robust validation** - Detects floating/disconnected potentiometer  
✅ **Never saves invalid** - Protects NVS from bad data  
✅ **Clear feedback** - LED + audio + serial messages  
✅ **Auto recovery** - Restores last known good type  
✅ **User-friendly** - Guides user through hardware issues

The system is now production-ready with comprehensive edge case handling!
