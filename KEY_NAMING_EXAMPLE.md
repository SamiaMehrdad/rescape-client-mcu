# Key Naming in DeviceConfig

## Overview

The DeviceConfig structure now supports optional custom names for all 16 keypad keys. If no custom name is provided (nullptr), the system uses default names based on the physical keypad layout.

## Default Key Names

```
Row 0: "1"  "2"  "3"  "A"
Row 1: "4"  "5"  "6"  "B"
Row 2: "7"  "8"  "9"  "C"
Row 3: "*"  "0"  "#"  "D"
```

Key indices: 0-15 (row-major order)

## DeviceConfig Structure

```cpp
struct DeviceConfig
{
    u8 matrixCellCount;          // Number of keys used (0-16)
    NamedComponent keys[16];     // Optional custom names for each key
    NamedComponent motors[4];    // Motor names (4 motors max)
    bool isConfigured;
};
```

## Example Configurations

### Example 1: Use Default Names (Terminal)

```cpp
{
    .matrixCellCount = 16,
    .keys = {},           // All nullptr - use defaults
    .motors = {},
    .isConfigured = true
}
```

Result: Keys show as "1", "2", "3", "A", "4", "5", "6", "B", etc.

### Example 2: Single Custom Key (GlowButton)

```cpp
{
    .matrixCellCount = 1,
    .keys = {
        {"Activate"}      // Key 0: "Activate" instead of "1"
    },
    .motors = {},
    .isConfigured = true
}
```

Result: Key 0 shows as "Activate"

### Example 3: Numeric Keypad (NumBox)

```cpp
{
    .matrixCellCount = 10,
    .keys = {
        {"0"},  // Key 0
        {"1"},  // Key 1
        {"2"},  // Key 2
        {"3"},  // Key 3
        {"4"},  // Key 4
        {"5"},  // Key 5
        {"6"},  // Key 6
        {"7"},  // Key 7
        {"8"},  // Key 8
        {"9"}   // Key 9
    },
    .motors = {},
    .isConfigured = true
}
```

### Example 4: Custom Function Keys (Timer)

```cpp
{
    .matrixCellCount = 4,
    .keys = {
        {"Start"},    // Key 0
        {"Stop"},     // Key 1
        {"Reset"},    // Key 2
        {"Pause"}     // Key 3
    },
    .motors = {},
    .isConfigured = true
}
```

### Example 5: Bomb Defusal (Full Matrix with Custom Names)

```cpp
{
    .matrixCellCount = 16,
    .keys = {
        {"Red Wire"},     // Key 0
        {"Blue Wire"},    // Key 1
        {"Green Wire"},   // Key 2
        {"Yellow Wire"},  // Key 3
        // ... etc for all 16 keys
    },
    .motors = {},
    .isConfigured = true
}
```

### Example 6: QB Puzzle (Partial Custom Names)

```cpp
{
    .matrixCellCount = 8,
    .keys = {
        {"Button 1"},
        {"Button 2"},
        {"Button 3"},
        {"Button 4"},
        {"Button 5"},
        {"Button 6"},
        {"Button 7"},
        {"Button 8"}
    },
    .motors = {
        {"Lock Servo"}    // Motor 0
    },
    .isConfigured = true
}
```

## Benefits

✅ **Flexible**: Each device can define meaningful key names ✅ **Optional**: nullptr = use default name (saves memory) ✅ **Self-documenting**: Code shows what each key does ✅ **Runtime accessible**: `DeviceConfigurations::getKeyName(deviceType, keyIndex)` ✅ **Display friendly**: Custom names appear in Serial output and UI

## Usage in Code

```cpp
// Get custom or default key name
const char* keyName = DeviceConfigurations::getKeyName(deviceType, 5);
if (keyName != nullptr) {
    Serial.print("Key pressed: ");
    Serial.println(keyName);
} else {
    // Use default name from KEYPAD_KEYS array
    Serial.println("Using default key name");
}
```

## Hardware Changes

With this update, we're also changing from:

-   **Old**: 2 motors + 4 switches
-   **New**: 4 motors + 16 keys (switches are now part of keypad)

This makes better use of the PCF8575 I/O expander pins.
