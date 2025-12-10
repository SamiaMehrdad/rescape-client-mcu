/**
 * @file deviceconfig.cpp
 * @brief Implementation of device-specific hardware configurations
 *
 * Simple configuration database defining how many matrix cells, motors,
 * and switches each device type uses, with optional component names.
 *
 * Device type names are defined in deviceconfig.h as DEVICE_TYPE_NAMES array.
 * This is the single source of truth for device names.
 */

#include "deviceconfig.h"
#include <Arduino.h>

//============================================================================
// DEVICE CONFIGURATION DATABASE
//============================================================================

const DeviceConfig DeviceConfigurations::kConfigs[MAX_DEVICE_TYPES] = {
    // TYPE 0: Terminal - Full 4x4 keypad with LEDs
    {
        .matrixCellCount = 16, // Full 4x4 matrix
        .motors = {},          // No motors (all nullptr)
        .switches = {},        // No switches (all nullptr)
        .isConfigured = true},

    // TYPE 1: GlowButton - Single button with LED
    {
        .matrixCellCount = 1, // Single button
        .motors = {},
        .switches = {},
        .isConfigured = true},

    // TYPE 2: NumBox - Numeric keypad (10 keys: 0-9)
    {
        .matrixCellCount = 10, // Numeric pad
        .motors = {},
        .switches = {},
        .isConfigured = true},

    // TYPE 3: Timer - Display with 4 buttons
    {
        .matrixCellCount = 4, // Control buttons
        .motors = {},
        .switches = {},
        .isConfigured = true},

    // TYPE 4: GlowDots - LED display grid
    {
        .matrixCellCount = 16, // Full LED grid
        .motors = {},
        .switches = {},
        .isConfigured = true},

    // TYPE 5: QB - Puzzle with buttons and motor
    {
        .matrixCellCount = 8, // 2x4 button array
        .motors = {
            {"Lock mechanism"} // Motor 1
        },
        .switches = {
            {"Lock position sensor"} // Switch 1
        },
        .isConfigured = true},

    // TYPE 6: RGBMixer - Color mixing interface
    {
        .matrixCellCount = 3, // R, G, B controls
        .motors = {},
        .switches = {},
        .isConfigured = true},

    // TYPE 7: Bomb - Defusal interface
    {
        .matrixCellCount = 16, // Full keypad
        .motors = {},
        .switches = {
            {"Wire 1 status"}, // Switch 1
            {"Wire 2 status"}  // Switch 2
        },
        .isConfigured = true},

    // TYPE 8: FinalOrder - Sequence puzzle
    {
        .matrixCellCount = 12, // 3x4 sequence buttons
        .motors = {},
        .switches = {},
        .isConfigured = true},

    // TYPE 9: BallGate - Ball release mechanism
    {
        .matrixCellCount = 0, // No matrix
        .motors = {
            {"Gate servo"} // Motor 1
        },
        .switches = {
            {"Gate open sensor"},  // Switch 1
            {"Gate closed sensor"} // Switch 2
        },
        .isConfigured = true},

    // TYPE 10: Actuator - Motor control device
    {
        .matrixCellCount = 0, // No matrix
        .motors = {
            {"Actuator 1"}, // Motor 1
            {"Actuator 2"}  // Motor 2
        },
        .switches = {
            {"Limit switch 1"}, // Switch 1
            {"Limit switch 2"}, // Switch 2
            {"Limit switch 3"}, // Switch 3
            {"Limit switch 4"}  // Switch 4
        },
        .isConfigured = true},

    // TYPE 11: TheWall - Grid puzzle
    {
        .matrixCellCount = 16, // Full grid
        .motors = {},
        .switches = {},
        .isConfigured = true},

    // TYPE 12: Scores - Score display with buttons
    {
        .matrixCellCount = 2, // Player buttons
        .motors = {},
        .switches = {},
        .isConfigured = true},

    // TYPE 13: BallBase - Ball detection base
    {
        .matrixCellCount = 4, // Position indicators
        .motors = {},
        .switches = {
            {"Ball sensor 1"}, // Switch 1
            {"Ball sensor 2"}, // Switch 2
            {"Ball sensor 3"}, // Switch 3
            {"Ball sensor 4"}  // Switch 4
        },
        .isConfigured = true},

    // TYPES 14-63: Unconfigured (placeholders)
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false},
    {0, {}, {}, false}};

//============================================================================
// PUBLIC ACCESSOR FUNCTIONS
//============================================================================

const DeviceConfig *DeviceConfigurations::getConfig(u8 deviceType)
{
        if (deviceType >= MAX_DEVICE_TYPES)
        {
                return nullptr;
        }
        return &kConfigs[deviceType];
}

const char *DeviceConfigurations::getDeviceName(u8 deviceType)
{
        if (deviceType >= MAX_DEVICE_TYPES)
        {
                return "INVALID";
        }
        return DEVICE_TYPE_NAMES[deviceType];
}

u8 DeviceConfigurations::getMatrixCellCount(u8 deviceType)
{
        const DeviceConfig *config = getConfig(deviceType);
        return config ? config->matrixCellCount : 0;
}

u8 DeviceConfigurations::getMotorCount(u8 deviceType)
{
        const DeviceConfig *config = getConfig(deviceType);
        if (!config)
        {
                return 0;
        }

        // Count motors with non-null names
        u8 count = 0;
        for (u8 i = 0; i < MAX_MOTORS; i++)
        {
                if (config->motors[i].name != nullptr)
                {
                        count++;
                }
        }
        return count;
}

u8 DeviceConfigurations::getSwitchCount(u8 deviceType)
{
        const DeviceConfig *config = getConfig(deviceType);
        if (!config)
        {
                return 0;
        }

        // Count switches with non-null names
        u8 count = 0;
        for (u8 i = 0; i < MAX_SWITCHES; i++)
        {
                if (config->switches[i].name != nullptr)
                {
                        count++;
                }
        }
        return count;
}

const char *DeviceConfigurations::getMotorName(u8 deviceType, u8 motorIndex)
{
        if (motorIndex >= MAX_MOTORS)
        {
                return nullptr;
        }

        const DeviceConfig *config = getConfig(deviceType);
        if (!config)
        {
                return nullptr;
        }

        return config->motors[motorIndex].name;
}

const char *DeviceConfigurations::getSwitchName(u8 deviceType, u8 switchIndex)
{
        if (switchIndex >= MAX_SWITCHES)
        {
                return nullptr;
        }

        const DeviceConfig *config = getConfig(deviceType);
        if (!config)
        {
                return nullptr;
        }

        return config->switches[switchIndex].name;
}

void DeviceConfigurations::printConfig(u8 deviceType)
{
        const DeviceConfig *config = getConfig(deviceType);
        if (!config)
        {
                Serial.println("ERROR: Invalid device type");
                return;
        }

        Serial.println("========================================");
        Serial.print("Device Type: ");
        Serial.print(deviceType);
        Serial.print(" (");
        Serial.print(getDeviceName(deviceType));
        Serial.println(")");
        Serial.print("Configured: ");
        Serial.println(config->isConfigured ? "YES" : "NO");
        Serial.println("========================================");

        // Matrix cells
        Serial.print("Matrix Cells: ");
        Serial.print(config->matrixCellCount);
        Serial.println("/16");

        // Motors
        u8 motorCount = getMotorCount(deviceType);
        Serial.print("Motors: ");
        Serial.println(motorCount);
        for (u8 i = 0; i < MAX_MOTORS; i++)
        {
                if (config->motors[i].name != nullptr)
                {
                        Serial.print("  Motor ");
                        Serial.print(i + 1);
                        Serial.print(": ");
                        Serial.println(config->motors[i].name);
                }
        }

        // Switches
        u8 switchCount = getSwitchCount(deviceType);
        Serial.print("Switches: ");
        Serial.println(switchCount);
        for (u8 i = 0; i < MAX_SWITCHES; i++)
        {
                if (config->switches[i].name != nullptr)
                {
                        Serial.print("  Switch ");
                        Serial.print(i + 1);
                        Serial.print(": ");
                        Serial.println(config->switches[i].name);
                }
        }

        Serial.println("========================================\n");
}

void DeviceConfigurations::printHardwareConfig(u8 deviceType, const char *indent)
{
        if (deviceType >= MAX_DEVICE_TYPES)
        {
                Serial.print(indent);
                Serial.println("INVALID DEVICE TYPE");
                return;
        }

        const DeviceConfig *config = getConfig(deviceType);
        if (!config)
        {
                Serial.print(indent);
                Serial.println("Configuration not found");
                return;
        }

        // Matrix cells
        Serial.print(indent);
        Serial.print("Matrix Cells:  ");
        Serial.println(config->matrixCellCount);

        // Motors
        u8 motorCount = getMotorCount(deviceType);
        Serial.print(indent);
        Serial.print("Motors:        ");
        Serial.println(motorCount);

        for (u8 i = 0; i < MAX_MOTORS; i++)
        {
                const char *motorName = getMotorName(deviceType, i);
                if (motorName != nullptr)
                {
                        Serial.print(indent);
                        Serial.print("  Motor ");
                        Serial.print(i);
                        Serial.print(":     ");
                        Serial.println(motorName);
                }
        }

        // Switches
        u8 switchCount = getSwitchCount(deviceType);
        Serial.print(indent);
        Serial.print("Switches:      ");
        Serial.println(switchCount);

        for (u8 i = 0; i < MAX_SWITCHES; i++)
        {
                const char *switchName = getSwitchName(deviceType, i);
                if (switchName != nullptr)
                {
                        Serial.print(indent);
                        Serial.print("  Switch ");
                        Serial.print(i);
                        Serial.print(":    ");
                        Serial.println(switchName);
                }
        }
}
