# Device Configuration System

## Overview

The Device Configuration System provides a centralized, maintainable way to define hardware requirements for each device type in the escape room system. It specifies which matrix cells (keypad + LED), motors, and switches each device uses.

## Architecture

### Data Structures

#### `MatrixConfig`

Defines which of the 16 matrix cells (4x4 grid) a device uses:

-   `usesMatrix`: Boolean flag if device uses any matrix cells
-   `enabledCells[16]`: Array indicating which cells are active
-   `cellCount`: Total number of active cells
-   `layoutDescription`: Human-readable description

#### `MotorConfig`

Defines motor usage (2 motors available):

-   `usesMotors`: Boolean flag if device uses any motors
-   `useMotor1`, `useMotor2`: Individual motor enable flags
-   `motor1Purpose`, `motor2Purpose`: Description strings

#### `SwitchConfig`

Defines digital input usage (4 switches available):

-   `usesSwitches`: Boolean flag if device uses any switches
-   `useSwitch1-4`: Individual switch enable flags
-   `switch1-4Purpose`: Description strings for each input

#### `DeviceConfig`

Complete hardware configuration for a device type:

```cpp
struct DeviceConfig {
    u8 deviceType;           // Type ID (0-63)
    const char* name;        // Device name
    MatrixConfig matrix;     // Matrix configuration
    MotorConfig motors;      // Motor configuration
    SwitchConfig switches;   // Switch configuration
    bool isConfigured;       // Configuration status
};
```

### Static Class: `DeviceConfigurations`

Provides query and utility functions:

-   `getConfig(deviceType)` - Get full configuration
-   `usesMatrix(deviceType)` - Check if uses matrix
-   `usesMotors(deviceType)` - Check if uses motors
-   `usesSwitches(deviceType)` - Check if uses switches
-   `getMatrixCellCount(deviceType)` - Get number of active cells
-   `isCellEnabled(deviceType, cellIndex)` - Check specific cell
-   `printConfig(deviceType)` - Print configuration to Serial

## Configured Device Types

### Type 0: Terminal

-   **Matrix**: Full 4x4 keypad (16 cells)
-   **Motors**: None
-   **Switches**: None
-   **Purpose**: General-purpose terminal with full keypad

### Type 1: GlowButton

-   **Matrix**: Single cell at position 0
-   **Motors**: None
-   **Switches**: None
-   **Purpose**: Simple illuminated button

### Type 2: NumBox

-   **Matrix**: 10 cells (numeric pad 0-9)
-   **Motors**: None
-   **Switches**: None
-   **Purpose**: Numeric code entry

### Type 3: Timer

-   **Matrix**: 4 cells (top row)
-   **Motors**: None
-   **Switches**: None
-   **Purpose**: Timer controls

### Type 4: GlowDots

-   **Matrix**: Full 4x4 grid (16 cells)
-   **Motors**: None
-   **Switches**: None
-   **Purpose**: LED display/indicator grid

### Type 5: QB (Puzzle Box)

-   **Matrix**: 8 cells (2x4 array)
-   **Motors**: Motor 1 (lock mechanism)
-   **Switches**: Switch 1 (lock position sensor)
-   **Purpose**: Interactive puzzle with lock

### Type 6: RGBMixer

-   **Matrix**: 3 cells (RGB controls)
-   **Motors**: None
-   **Switches**: None
-   **Purpose**: Color mixing interface

### Type 7: Bomb

-   **Matrix**: Full 4x4 keypad (16 cells)
-   **Motors**: None
-   **Switches**: Switches 1-2 (wire status)
-   **Purpose**: Bomb defusal interface

### Type 8: FinalOrder

-   **Matrix**: 12 cells (3x4 array)
-   **Motors**: None
-   **Switches**: None
-   **Purpose**: Sequence puzzle

### Type 9: BallGate

-   **Matrix**: None
-   **Motors**: Motor 1 (gate servo)
-   **Switches**: Switches 1-2 (open/closed sensors)
-   **Purpose**: Motorized ball release

### Type 10: Actuator

-   **Matrix**: None
-   **Motors**: Motors 1-2 (dual actuators)
-   **Switches**: Switches 1-4 (limit switches)
-   **Purpose**: Dual motor control with feedback

### Type 11: TheWall

-   **Matrix**: Full 4x4 grid (16 cells)
-   **Motors**: None
-   **Switches**: None
-   **Purpose**: Grid-based puzzle

### Type 12: Scores

-   **Matrix**: 2 cells (player buttons)
-   **Motors**: None
-   **Switches**: None
-   **Purpose**: Score tracking interface

### Type 13: BallBase

-   **Matrix**: 4 cells (position indicators)
-   **Motors**: None
-   **Switches**: Switches 1-4 (ball sensors)
-   **Purpose**: Ball detection system

### Types 14-31

Reserved for future device types (currently unconfigured)

### Types 32-63

Reserved for expansion (unconfigured)

## Usage Examples

### Query Device Capabilities

```cpp
#include "deviceconfig.h"

// Check if device uses matrix
if (DeviceConfigurations::usesMatrix(DeviceType::TERMINAL)) {
    u8 cellCount = DeviceConfigurations::getMatrixCellCount(DeviceType::TERMINAL);
    Serial.print("Terminal uses ");
    Serial.print(cellCount);
    Serial.println(" matrix cells");
}

// Check specific cell
if (DeviceConfigurations::isCellEnabled(DeviceType::NUM_BOX, 5)) {
    // Cell 5 is enabled for NumBox
}
```

### Get Full Configuration

```cpp
const DeviceConfig* config = DeviceConfigurations::getConfig(deviceType);
if (config && config->isConfigured) {
    // Access matrix, motors, switches configuration
    if (config->matrix.usesMatrix) {
        for (u8 i = 0; i < KEYPAD_SIZE; i++) {
            if (config->matrix.enabledCells[i]) {
                // Enable this cell
            }
        }
    }
}
```

### Print Configuration

```cpp
// Print detailed configuration to Serial
DeviceConfigurations::printConfig(DeviceType::QB);
```

Output:

```
========================================
Device Type: 5 (QB)
Configured: YES
========================================
MATRIX:
  Cells used: 8/16
  Layout: 2x4 button array
  Enabled cells:
    Row 0: [X] [X] [X] [X]
    Row 1: [X] [X] [X] [X]
    Row 2: [ ] [ ] [ ] [ ]
    Row 3: [ ] [ ] [ ] [ ]

MOTORS:
  Motor 1: Lock mechanism

SWITCHES:
  Switch 1: Lock position sensor
========================================
```

## Adding New Device Types

To add a new device type:

1. **Define constant** in `deviceconfig.h`:

```cpp
namespace DeviceType {
    constexpr u8 NEW_DEVICE = 14;
}
```

2. **Update device name** in `core.cpp`:

```cpp
const char *Core::kDeviceTypeNames[64] = {
    // ...
    "NewDevice",  // Type 14
    // ...
};
```

3. **Add configuration** in `deviceconfig.cpp`:

```cpp
{
    .deviceType = DeviceType::NEW_DEVICE,
    .name = "NewDevice",
    .matrix = []() {
        bool cells[KEYPAD_SIZE] = {
            true, true, false, false,  // Define your pattern
            // ...
        };
        return createPartialMatrix(cells, "Description");
    }(),
    .motors = {
        .usesMotors = true,
        .useMotor1 = true,
        .useMotor2 = false,
        .motor1Purpose = "Main motor",
        .motor2Purpose = nullptr
    },
    .switches = createNoSwitches(),  // Or configure switches
    .isConfigured = true
}
```

## Benefits

### Centralized Configuration

-   All hardware definitions in one place
-   Easy to see what each device uses
-   No scattered magic numbers

### Type Safety

-   Compile-time constants via namespace
-   Clear data structures
-   Validated queries

### Maintainability

-   Add new devices without touching core logic
-   Update configurations independently
-   Self-documenting with descriptions

### Debugging

-   Print full configuration with one call
-   Visual grid display for matrix cells
-   Clear purpose descriptions

## Integration Points

The configuration system can be integrated with:

-   **Core class**: Auto-configure hardware based on device type
-   **MatrixPanel**: Enable/disable cells per device
-   **IOExpander**: Initialize only needed motors/switches
-   **RoomBus**: Report device capabilities
-   **UI/App**: Display device-specific interfaces

## Future Enhancements

Potential additions:

-   Runtime configuration validation
-   Configuration versioning
-   Dynamic reconfiguration
-   Configuration persistence (EEPROM/NVS)
-   Web-based configuration editor
-   Configuration export/import (JSON)
