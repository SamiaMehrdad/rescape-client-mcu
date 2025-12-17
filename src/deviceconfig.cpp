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


/************************* kCommonCommands ********************************
 * Commands shared by all device types (hello/ack/ping/reset)
 ***************************************************************/
static const CommandSet kCommonCommands = makeCommandSet({CORE_HELLO, CORE_ACK, CORE_PING, CORE_RESET});

const DeviceConfig DeviceConfigurations::kConfigs[MAX_DEVICE_TYPES] = {
    // TYPE 0: Terminal - Full 4x4 keypad with LEDs (uses default key names)
    {.cellCount = 16, // Full 4x4 matrix
     .keys = {},      // Use default names: "1","2","3","A", "4","5","6","B", etc.
     .motors = {},    // No motors
     .commands = makeCommandSet({TERM_RESET})},

    // TYPE 1: GlowButton - Single button with LED
    {.cellCount = 1, // Single button
     .keys = {
         // Custom name for the single key
         {"Activate"} // Key 0: "Activate" instead of "1"
     },
     .motors = {},
     .commands = makeCommandSet({GLOW_SET_COLOR})},

    // TYPE 2: NumBox - Numeric keypad (10 keys: 0-9)
    {.cellCount = 4 * 6 * 7, // 4*6 7segments
     .keys = {},
     .motors = {},
     .commands = makeCommandSet({NUM_SET_DIGIT_COLOR, NUM_SET_DIGIT_VAL, NUM_SET_ROW_NUM})},

    // TYPE 3: Timer - Display with 4 buttons
    {.cellCount = 4, // Control buttons
     .keys = {},
     .motors = {},
     .commands = makeCommandSet({TMR_SET_COLOR, TMR_SET_VALUE, TMR_START, TMR_PAUSE})},

    // TYPE 4: GlowDots - LED display grid
    {.cellCount = 16, // Full LED grid
     .keys = {},
     .motors = {},
     .commands = makeCommandSet({DOTS_SET_COLORS, DOTS_SET_MOVE, DOTS_SET_DELAY, DOTS_SET_LED})},

    // TYPE 5: QB - Puzzle with buttons and motor
    {.cellCount = 16, // 4x4 button array
     .keys = {},
     .motors = {},
     .commands = makeCommandSet({QB_SET_COLORS, QB_SET_MODES})},

    // TYPE 6: RGBMixer - Color mixing interface
    {.cellCount = 8, // R, G, B up/down and check buttons
     .keys = {},
     .motors = {},
     .commands = makeCommandSet()},

    // TYPE 7: Bomb - Defusal interface
    {.cellCount = 16, // Full keypad
     .keys = {},
     .motors = {},
     .commands = makeCommandSet({BOMB_SET_STATE})},

    // TYPE 8: FinalOrder - Sequence puzzle
    {.cellCount = 12, // 3x4 sequence buttons
     .keys = {},
     .motors = {},
     .commands = makeCommandSet({FINAL_RESET})},

    // TYPE 9: BallGate - Ball release mechanism
    {.cellCount = 1, // LED only, gate indicator
     .keys = {},
     .motors = {
         {"Gate motor"},  // Gate open
         {"Reject motor"} // Reject ball
     },
     .commands = makeCommandSet()},

    // TYPE 10: Actuator - Motor control device
    {.cellCount = 0, // No matrix
     .keys = {},
     .motors = {
         {"Actuator 1"}, // Motor 1
         {"Actuator 2"}  // Motor 2
     },
     .commands = makeCommandSet({ACT_OPEN, ACT_CLOSE})},

    // TYPE 11: TheWall - Grid puzzle
    {.cellCount = 8, // Slots control
     .keys = {},
     .motors = {{"Fall"}, {"Remove"}},
     .commands = makeCommandSet()},

    // TYPE 12: Scores - Score display with buttons
    {.cellCount = 2, // Player buttons
     .keys = {},
     .motors = {},
     .commands = makeCommandSet()},

    // TYPE 13: BallBase - Ball detection base
    {.cellCount = 4, // Position indicators
     .keys = {},
     .motors = {},
     .commands = makeCommandSet({BALL_ACTIVATE})},

    // TYPES 14-63: Unconfigured (default zero-initialized)
};

/************************* getConfig **************************************
 * Lookup config pointer for a device type.
 ***************************************************************/
const DeviceConfig *DeviceConfigurations::getConfig(u8 deviceType)
{
        if (deviceType >= MAX_DEVICE_TYPES)
        {
                return nullptr;
        }
        return &kConfigs[deviceType];
}

/************************* getDeviceName **********************************
 * Get device type name string.
 ***************************************************************/
const char *DeviceConfigurations::getDeviceName(u8 deviceType)
{
        if (deviceType >= MAX_DEVICE_TYPES)
        {
                return "INVALID";
        }
        return DEVICE_TYPE_NAMES[deviceType];
}

/************************* getCellCount ***********************************
 * Get LED cell count for type.
 ***************************************************************/
/* DeviceConfigurations::getCellCount removed (unused). */

/************************* getMotorCount **********************************
 * Count motors with names for type.
 ***************************************************************/
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

/************************* getCommandSet **********************************
 * Get device-specific command set pointer.
 ***************************************************************/
const CommandSet *DeviceConfigurations::getCommandSet(u8 deviceType)
{
        const DeviceConfig *config = getConfig(deviceType);
        return config ? &config->commands : nullptr;
}

/************************* getMergedCommandSet *****************************
 * Merge common and device-specific commands.
 ***************************************************************/
CommandSet DeviceConfigurations::getMergedCommandSet(u8 deviceType)
{
        CommandSet merged{};

        // Start with common commands
        u8 idx = 0;
        for (u8 i = 0; i < kCommonCommands.count && idx < MAX_COMMANDS; ++i)
        {
                merged.cmds[idx++] = kCommonCommands.cmds[i];
        }

        // Append device-specific commands
        const CommandSet *deviceCmds = getCommandSet(deviceType);
        if (deviceCmds)
        {
                for (u8 i = 0; i < deviceCmds->count && idx < MAX_COMMANDS; ++i)
                {
                        merged.cmds[idx++] = deviceCmds->cmds[i];
                }
        }

        merged.count = idx;
        return merged;
}

/************************* getKeyName *************************************
 * Get custom key name for a type/index (nullable).
 ***************************************************************/
const char *DeviceConfigurations::getKeyName(u8 deviceType, u8 keyIndex)
{
        if (keyIndex >= MAX_KEYS)
        {
                return nullptr;
        }

        const DeviceConfig *config = getConfig(deviceType);
        if (!config)
        {
                return nullptr;
        }

        return config->keys[keyIndex].name;
}

/************************* getMotorName ***********************************
 * Get motor name for a type/index (nullable).
 ***************************************************************/
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

/************************* printConfig ************************************
 * Print detailed config for a device type.
 ***************************************************************/
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
        Serial.println("========================================");

        // Cell count (LEDs)
        Serial.print("LED Cells: ");
        Serial.println(config->cellCount);

        // Keys (show custom names if any)
        Serial.print("Custom Key Names: ");
        u8 customKeyCount = 0;
        for (u8 i = 0; i < MAX_KEYS; i++)
        {
                if (config->keys[i].name != nullptr)
                {
                        customKeyCount++;
                }
        }
        Serial.println(customKeyCount);
        for (u8 i = 0; i < MAX_KEYS; i++)
        {
                if (config->keys[i].name != nullptr)
                {
                        Serial.print("  Key ");
                        Serial.print(i);
                        Serial.print(": ");
                        Serial.println(config->keys[i].name);
                }
        }

        // Motors
        u8 motorCount = getMotorCount(deviceType);
        Serial.print("Motors: ");
        Serial.println(motorCount);
        for (u8 i = 0; i < MAX_MOTORS; i++)
        {
                if (config->motors[i].name != nullptr)
                {
                        Serial.print("  Motor ");
                        Serial.print(i);
                        Serial.print(": ");
                        Serial.println(config->motors[i].name);
                }
        }

        Serial.println("========================================\n");
}

/************************* printHardwareConfig *****************************
 * Print concise hardware config with optional indent.
 ***************************************************************/
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

        // Cell count
        Serial.print(indent);
        Serial.print("LED Cells:     ");
        Serial.println(config->cellCount);

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

        // Custom key names (if any)
        u8 customKeyCount = 0;
        for (u8 i = 0; i < MAX_KEYS; i++)
        {
                const char *keyName = getKeyName(deviceType, i);
                if (keyName != nullptr)
                {
                        customKeyCount++;
                }
        }

        if (customKeyCount > 0)
        {
                Serial.print(indent);
                Serial.print("Custom Keys:   ");
                Serial.println(customKeyCount);

                for (u8 i = 0; i < MAX_KEYS; i++)
                {
                        const char *keyName = getKeyName(deviceType, i);
                        if (keyName != nullptr)
                        {
                                Serial.print(indent);
                                Serial.print("  Key ");
                                Serial.print(i);
                                Serial.print(":      ");
                                Serial.println(keyName);
                        }
                }
        }
}
