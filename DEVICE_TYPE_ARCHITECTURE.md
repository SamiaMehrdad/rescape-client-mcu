# Device Type Architecture

## Overview

The Core firmware supports 32 different device types (0-31) configured via a trimmer potentiometer on GPIO 2. This allows a single firmware image to support multiple hardware variants or application-specific behaviors.

## Architecture Design

The system is designed with separation of concerns:

-   **Core Firmware Layer**: Provides basic type detection and placeholder names
-   **Application Layer**: Can implement device-specific behavior and custom type names

## Core Firmware Level (Current Implementation)

### Hardware Configuration

-   **GPIO 2**: ADC input from 10kΩ-25kΩ trimmer potentiometer
-   **Resolution**: 5-bit (32 distinct types)
-   **Detection**: One-time read at startup + runtime calibration mode

### Placeholder Type Names

The Core firmware defines generic placeholder names:

```cpp
const char* Core::kDeviceTypeNames[32] = {
    "TYPE_00", "TYPE_01", "TYPE_02", "TYPE_03",
    // ... through TYPE_31
};
```

### Core API

```cpp
// Get numeric device type (0-31)
u8 getDeviceType() const;

// Get placeholder type name
const char* getDeviceTypeName() const;
```

## Application Layer Integration

### Option 1: Direct Type Query

High-level App code can query the device type and implement its own logic:

```cpp
// In your App layer
void App::init() {
    u8 deviceType = core.getDeviceType();

    switch(deviceType) {
        case 0:
            // Initialize as "Door Controller"
            initDoorController();
            break;
        case 1:
            // Initialize as "Window Sensor"
            initWindowSensor();
            break;
        // ... etc
    }
}
```

### Option 2: Custom Type Name Mapping

App layer can provide meaningful names:

```cpp
// In your App layer
const char* App::getDeviceTypeName(u8 type) {
    static const char* appTypeNames[32] = {
        "Door Controller",
        "Window Sensor",
        "RGB Light Strip",
        "Motion Detector",
        "Temperature Sensor",
        "Keypad Entry",
        "Audio Player",
        "Servo Controller",
        // ... custom names for your application
        "TYPE_08",  // Not yet assigned
        "TYPE_09",  // Not yet assigned
        // ... etc
    };

    return (type < 32) ? appTypeNames[type] : "INVALID";
}
```

### Option 3: Polymorphic Device Classes

For complex applications, use device type to instantiate different classes:

```cpp
// Abstract device interface
class Device {
public:
    virtual void init() = 0;
    virtual void update() = 0;
    virtual const char* getName() = 0;
};

// Concrete device types
class DoorController : public Device { ... };
class WindowSensor : public Device { ... };
class RGBLightStrip : public Device { ... };

// In App layer
Device* App::createDevice(u8 type) {
    switch(type) {
        case 0: return new DoorController();
        case 1: return new WindowSensor();
        case 2: return new RGBLightStrip();
        // ... etc
        default: return nullptr;
    }
}
```

## Type Detection Mode

Users can calibrate the device type at runtime:

1. **Enter**: Long-press boot button (>1 second)
2. **Behavior**:
    - Reads ADC every 1 second
    - Logs type changes to serial
    - Flashes LED + beep on each reading
3. **Exit**: Long-press boot button again

Example output:

```
=== ENTERING TYPE DETECTION MODE ===
Type changed: TYPE_00 (0) -> TYPE_05 (5)
Type changed: TYPE_05 (5) -> TYPE_12 (12)

=== EXITING TYPE DETECTION MODE ===
Final Device Type: TYPE_12 (12)
```

## Best Practices

### 1. Keep Core Firmware Generic

-   Core firmware should remain application-agnostic
-   No application-specific logic in Core layer
-   Use placeholder names at Core level

### 2. Application-Specific Logic in App Layer

-   All device-specific behavior in separate App code
-   App layer can be in separate files/directories
-   Maintains clean separation of concerns

### 3. Independent Source Code Structure

Recommended structure for independent type applications:

```
client-mcu/
├── include/
│   └── core.h          # Core firmware (generic)
├── src/
│   └── core.cpp        # Core firmware (generic)
│   └── main.cpp        # Hardware setup
└── app/                # Application layer (type-specific)
    ├── common/
    │   └── app_base.h  # Common app interface
    ├── type_00_door/
    │   ├── door_app.h
    │   └── door_app.cpp
    ├── type_01_window/
    │   ├── window_app.h
    │   └── window_app.cpp
    └── type_02_light/
        ├── light_app.h
        └── light_app.cpp
```

### 4. Build System Integration

Use PlatformIO build flags to select which App to compile:

```ini
; platformio.ini
[env:door_controller]
build_flags =
    -D SEEED_XIAO_ESP32C3
    -D APP_TYPE=0
    -D APP_DOOR_CONTROLLER

[env:window_sensor]
build_flags =
    -D SEEED_XIAO_ESP32C3
    -D APP_TYPE=1
    -D APP_WINDOW_SENSOR
```

Then in code:

```cpp
#ifdef APP_DOOR_CONTROLLER
    #include "app/type_00_door/door_app.h"
#elif defined(APP_WINDOW_SENSOR)
    #include "app/type_01_window/window_app.h"
#endif
```

## Hardware Assignment Table

Document your device type assignments:

| Type | Name    | Description | Hardware Notes     |
| ---- | ------- | ----------- | ------------------ |
| 0    | TYPE_00 | _Available_ | Trimmer at minimum |
| 1    | TYPE_01 | _Available_ |                    |
| ...  | ...     | ...         |                    |
| 31   | TYPE_31 | _Available_ | Trimmer at maximum |

Replace with your actual assignments as you develop applications.

## Summary

This architecture allows you to:

-   ✅ Use a single Core firmware for all devices
-   ✅ Keep Core code application-independent
-   ✅ Develop separate App code for each device type
-   ✅ Maintain clean separation of concerns
-   ✅ Easily add new device types without modifying Core
-   ✅ Configure device type in field with just a trimmer adjustment

The Core firmware provides the foundation, and your App layer builds application-specific functionality on top.
