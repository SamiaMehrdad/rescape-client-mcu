# 64-Type Expansion Strategy

## Overview

The device type system is designed with forward compatibility to support up to **64 device types** (0-63) using 6-bit addressing, while currently implementing 32 types (0-31) using 5-bit addressing.

## ADC Mapping Design

### Step Size: 64 ADC Units per Type

```
ADC Range: 0-4095 (12-bit)
Step Size: 64 ADC units
Max Types: 4096 / 64 = 64 types (0-63)
```

### Current Implementation (32 Types)

```
ADC Range    → Device Type  → Status
0-63         → TYPE_00       → Active
64-127       → TYPE_01       → Active
128-191      → TYPE_02       → Active
...
1984-2047    → TYPE_31       → Active ✅ (Currently clamped here)
2048-2111    → TYPE_32       → Reserved 🔒
2112-2175    → TYPE_33       → Reserved 🔒
...
4032-4095    → TYPE_63       → Reserved 🔒
```

## Why 64 Steps Instead of 128?

### Old Design (128 Steps)

```
4096 / 128 = 32 types maximum
No room for expansion without breaking existing configs
```

### New Design (64 Steps)

```
4096 / 64 = 64 types possible
Lower half (0-31): Currently active ✅
Upper half (32-63): Reserved for future 🔒
```

### Advantages

| Aspect           | 128 Steps    | 64 Steps          |
| ---------------- | ------------ | ----------------- |
| Current types    | 32 (0-31)    | 32 (0-31)         |
| Future expansion | None         | +32 types (32-63) |
| ADC resolution   | ~3% per type | ~1.5% per type    |
| Noise tolerance  | Higher       | Adequate          |
| Future-proof     | ❌ No        | ✅ Yes            |

## ADC Range Breakdown

### Currently Active Types (0-31)

| Type    | ADC Min | ADC Max | Pot Position  |
| ------- | ------- | ------- | ------------- |
| TYPE_00 | 0       | 63      | Minimum (~0Ω) |
| TYPE_01 | 64      | 127     | ~3%           |
| TYPE_16 | 1024    | 1087    | Mid (~50%)    |
| TYPE_31 | 1984    | 2047    | ~50%          |

### Reserved for Future (32-63)

| Type    | ADC Min | ADC Max | Pot Position    |
| ------- | ------- | ------- | --------------- |
| TYPE_32 | 2048    | 2111    | ~50%            |
| TYPE_48 | 3072    | 3135    | ~75%            |
| TYPE_63 | 4032    | 4095    | Maximum (~25kΩ) |

## Implementation Details

### Current Code (Clamped to 31)

```cpp
u8 deviceType = adcValue / 64;  // Calculate 0-63

// Currently clamp to 0-31 range (5-bit addressing)
if (deviceType > 31)
    deviceType = 31;
```

### Future Expansion (Remove Clamp)

```cpp
u8 deviceType = adcValue / 64;  // Calculate 0-63

// No clamp - full 0-63 range enabled (6-bit addressing)
// Allows access to all 64 device types
```

## Enabling 64 Types in Future

### Step 1: Remove Clamp

```cpp
// In readDeviceType():
u8 deviceType = adcValue / 64;

// OLD: if (deviceType > 31) deviceType = 31;  ← Remove this line
// NEW: // Full 64-type range enabled

if (deviceType > 63)  // Safety clamp at maximum
    deviceType = 63;
```

### Step 2: Update Validation

```cpp
// In getDeviceTypeName():
if (m_deviceType > 63)  // Was: > 31
    return "INVALID";
```

### Step 3: Update Documentation

-   Update all references from "0-31" to "0-63"
-   Document new type ranges 32-63
-   Update boot report and calibration messages

## Migration Path

### Existing Devices (Types 0-31)

-   ✅ No changes needed
-   ✅ Continue working with current configuration
-   ✅ Stored NVS values remain valid

### New Devices (Types 32-63)

-   Requires firmware update to remove clamp
-   Can use upper half of pot range
-   Backward compatible with devices using 0-31

## Use Cases for 64 Types

### Scenario 1: Regional Variants

```
Types 0-31:  North America devices
Types 32-63: Europe/Asia devices
```

### Scenario 2: Hardware Revisions

```
Types 0-31:  Hardware v1.0
Types 32-63: Hardware v2.0 with enhanced features
```

### Scenario 3: Feature Tiers

```
Types 0-15:  Basic devices
Types 16-31: Standard devices
Types 32-47: Advanced devices
Types 48-63: Premium devices
```

### Scenario 4: Protocol Versions

```
Types 0-31:  Room Bus v1.0
Types 32-63: Room Bus v2.0
```

## Potentiometer Range Distribution

With 20kΩ pot and 64-step resolution:

```
Pot Position    ADC Value    Type Range    Status
────────────────────────────────────────────────────
0% (0Ω)         ~0           TYPE_00       Active
25%             ~1024        TYPE_16       Active
50%             ~2048        TYPE_31/32    Active/Reserved
75%             ~3072        TYPE_48       Reserved
100% (20kΩ)     ~4095        TYPE_63       Reserved
```

**Current Usage**: Lower 50% of pot (0-50%) → Types 0-31  
**Future Expansion**: Upper 50% of pot (50-100%) → Types 32-63

## Disconnect Detection

With pull-down check:

```cpp
// Phase 1: Check for disconnect
pinMode(CONFIG_ADC_PIN, INPUT_PULLDOWN);
if (analogRead(CONFIG_ADC_PIN) < 200) {
    return 0xFF;  // Disconnected
}

// Phase 2: Accurate reading
pinMode(CONFIG_ADC_PIN, INPUT);
int adcValue = analogRead(CONFIG_ADC_PIN);
u8 type = adcValue / 64;  // 0-63 range
```

**Note**: Disconnection detection (<200 ADC) is below TYPE_00 range (0-63 ADC), ensuring TYPE_00 remains valid.

## Benefits Summary

✅ **Future-proof**: Ready for 64 types without breaking changes  
✅ **Backward compatible**: Existing 0-31 configs continue working  
✅ **Better resolution**: Finer granularity (64 vs 128 steps)  
✅ **Clean migration**: Simply remove clamp to enable  
✅ **Documented types**: All 64 names already defined  
✅ **Flexible**: Many expansion strategies possible

## Technical Specifications

| Parameter        | Value                           |
| ---------------- | ------------------------------- |
| ADC Resolution   | 12-bit (0-4095)                 |
| Step Size        | 64 ADC units                    |
| Current Types    | 32 (0-31, 5-bit)                |
| Future Types     | 64 (0-63, 6-bit)                |
| ADC per Type     | 64 units (~1.56% of full range) |
| Noise Tolerance  | ±32 ADC units (~50% margin)     |
| Type 0-31 Range  | ADC 0-2047 (lower 50%)          |
| Type 32-63 Range | ADC 2048-4095 (upper 50%)       |

## Example: Enabling Full 64 Types

```cpp
// Current implementation (src/core.cpp, line ~198)
u8 deviceType = adcValue / 64;

if (deviceType > 31)        // ← Current: Clamp to 31
    deviceType = 31;

// Future implementation (when ready for 64 types)
u8 deviceType = adcValue / 64;

if (deviceType > 63)        // ← Future: Clamp to 63
    deviceType = 63;
```

**Just change one number!** That's the power of good architecture. 🎉

## Summary

The system is designed for seamless expansion from 32 to 64 device types:

-   **Now**: Using lower half (0-31) with pot at 0-50%
-   **Future**: Enable upper half (32-63) with pot at 50-100%
-   **Migration**: Change one clamp value from 31 → 63
-   **Compatibility**: Existing devices unaffected

This gives you double the device types when you need them, with zero impact on current deployments! 🚀
