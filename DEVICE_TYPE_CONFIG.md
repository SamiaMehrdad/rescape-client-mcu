# Device Type Configuration

This document describes the hardware configuration method for selecting device type (personality) at startup.

## Overview

The firmware uses a **5-bit ADC reading** to determine device type (0-31), allowing a single firmware image to support up to **32 different device personalities**.

## Hardware Setup

### Components Required

-   **1× Trimmer Potentiometer:** 10kΩ or 25kΩ (multi-turn recommended)
-   **1× Capacitor (optional):** 0.1µF ceramic for noise filtering

### Circuit Connection

```
     3.3V ────┬──────────────────┐
              │                  │
              ├─── Wiper         │
              │      │           │
    10kΩ-25kΩ │      └──────> GPIO2 (CONFIG_ADC_PIN)
    Trimmer   │                  │
    Pot       │                 ─┴─ 0.1µF (optional filter cap)
              │                  │
              └──────────────────┴──── GND
```

### Pin Assignment

-   **GPIO2** (D0/A0 on XIAO ESP32-C3) - ADC input
-   Configured in `mcupins.h` as `CONFIG_ADC_PIN`

## Device Type Selection

### Setting Device Type

1. Power on the device with Serial Monitor (115200 baud)
2. Observe the startup message: `Device Type: X`
3. Adjust the trimmer potentiometer to achieve desired type (0-31)
4. The device reads the configuration **once at startup**

### Voltage to Device Type Mapping

| Type | Voltage Range | Type | Voltage Range | Type | Voltage Range | Type | Voltage Range |
| ---- | ------------- | ---- | ------------- | ---- | ------------- | ---- | ------------- |
| 0    | 0.00V - 0.10V | 8    | 0.83V - 0.93V | 16   | 1.65V - 1.75V | 24   | 2.48V - 2.58V |
| 1    | 0.10V - 0.21V | 9    | 0.93V - 1.03V | 17   | 1.75V - 1.86V | 25   | 2.58V - 2.68V |
| 2    | 0.21V - 0.31V | 10   | 1.03V - 1.14V | 18   | 1.86V - 1.96V | 26   | 2.68V - 2.79V |
| 3    | 0.31V - 0.41V | 11   | 1.14V - 1.24V | 19   | 1.96V - 2.06V | 27   | 2.79V - 2.89V |
| 4    | 0.41V - 0.52V | 12   | 1.24V - 1.34V | 20   | 2.06V - 2.17V | 28   | 2.89V - 2.99V |
| 5    | 0.52V - 0.62V | 13   | 1.34V - 1.45V | 21   | 2.17V - 2.27V | 29   | 2.99V - 3.10V |
| 6    | 0.62V - 0.72V | 14   | 1.45V - 1.55V | 22   | 2.27V - 2.37V | 30   | 3.10V - 3.20V |
| 7    | 0.72V - 0.83V | 15   | 1.55V - 1.65V | 23   | 2.37V - 2.48V | 31   | 3.20V - 3.30V |

### Example Device Types (Planned)

```cpp
0  = GLOW_STRIP        - RGB LED strip controller
1  = BUTTON_PAD        - 4-button input pad
2  = KEYPAD            - 4x4 matrix keypad with sound
3  = SWITCH_PANEL      - Switch input panel
4  = MOTOR_CTRL        - Motor/servo controller
5  = AUDIO_PLAYER      - Audio feedback device
6  = SENSOR_NODE       - Sensor input node
7  = RELAY_BOARD       - Relay output board
8-31 = Custom types    - Reserved for future devices
```

## Technical Details

### ADC Configuration

-   **Resolution:** 12-bit (0-4095)
-   **Reference voltage:** 3.3V
-   **Sampling:** 32 samples averaged for stability
-   **Sample interval:** 100µs between readings

### Conversion Algorithm

```cpp
deviceType = (adcValue / 128) clamped to 0-31
```

### Reliability

-   Each device type spans ~128 ADC units (~103mV)
-   With averaging, noise is < ±5 ADC units (~4mV)
-   Provides ~50mV margin between adjacent types
-   **Very reliable** with proper trimmer adjustment

## Recommendations

### Trimmer Selection

-   **Best:** 10-turn or 25-turn multi-turn trimmer
    -   Provides fine adjustment control
    -   Stable positioning
    -   Less drift
-   **Acceptable:** Single-turn trimmer pot
    -   Cheaper but harder to set precisely
    -   May require multiple attempts

### Optional Improvements

-   Add 0.1µF capacitor from ADC pin to GND for noise filtering
-   Use trimpot with built-in locknut to prevent accidental adjustment
-   Label trimmer position on PCB for reference

## Usage in Firmware

The device type is read once during `Application::init()` and stored in `m_deviceType`:

```cpp
// In app.cpp
void Application::init()
{
    // Read device type from trimmer pot (one-time at startup)
    m_deviceType = readDeviceType();
    Serial.print("Device Type: ");
    Serial.println(m_deviceType);

    // Device type is now available via getDeviceType()
    // Use it to configure device-specific behavior
}
```

Access the device type anywhere in the application:

```cpp
u8 type = app.getDeviceType(); // Returns 0-31
```

## Troubleshooting

### Device type unstable/changing

-   Add 0.1µF filter capacitor
-   Use shielded cable if trimmer is remote
-   Increase averaging samples (currently 32)

### Cannot achieve specific type

-   Verify trimmer is connected correctly (wiper to GPIO2)
-   Check voltage range with multimeter
-   Ensure trimmer pot is 10kΩ-100kΩ range (not too high resistance)

### Reading shows "Type 31" always

-   Check for open circuit (disconnected wiper)
-   Verify trimmer is working (measure resistance)

### Reading shows "Type 0" always

-   Check for short circuit to GND
-   Verify 3.3V is connected to top of trimmer
