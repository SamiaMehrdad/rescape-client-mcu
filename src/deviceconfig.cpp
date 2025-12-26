/**
 * @file deviceconfig.cpp
 * @brief Implementation of device-specific hardware configurations
 *
 * Simple configuration database defining how many matrix cells, motors,
 * and switches each device type uses, with optional component names.
 *
 * This is the single source of truth for device definitions.
 */

#include "deviceconfig.h"
#include <Arduino.h>

// =================================================================================
// MASTER DEVICE LIST
// =================================================================================
// This is the only place you need to edit to add/change devices.
// IDs must be unique. Order doesn't strictly matter, but keeping them sorted helps.
// =================================================================================

static const DeviceDefinition DEVICE_CATALOG[] = {

    // ID 0: Terminal
    {
        TERMINAL, "Terminal", {.cellCount = 16, .keyNames = {}, // Default names
                               .motorNames = {},
                               .commands = makeCommandSet({TERM_RESET})}},

    // ID 1: Glow Button
    {
        GLOW_BUTTON, "GlowButton", {.cellCount = 1, .keyNames = {"Activate"}, .motorNames = {}, .commands = makeCommandSet({GLOW_SET_COLOR})}},

    // ID 2: Num Box
    {
        NUM_BOX, "NumBox",
        {.cellCount = 28, // 4*6 7segments
         .keyNames = {},
         .motorNames = {},
         .commands = makeCommandSet({NUM_SET_DIGIT_COLOR, NUM_SET_DIGIT_VAL, NUM_SET_ROW_NUM})}},

    // ID 3: Timer
    {
        TIMER, "Timer", {.cellCount = 4, .keyNames = {}, .motorNames = {}, .commands = makeCommandSet({TMR_SET_COLOR, TMR_SET_VALUE, TMR_START, TMR_PAUSE})}},

    // ID 4: Glow Dots
    {
        GLOW_DOTS, "GlowDots", {.cellCount = 16, .keyNames = {}, .motorNames = {}, .commands = makeCommandSet({DOTS_SET_COLORS, DOTS_SET_MOVE, DOTS_SET_DELAY, DOTS_SET_LED})}},

    // ID 5: QB
    {
        QB, "QB", {.cellCount = 16, .keyNames = {}, .motorNames = {}, .commands = makeCommandSet({QB_SET_COLORS, QB_SET_MODES})}},

    // ID 6: RGB Mixer
    {
        RGB_MIXER, "RGBMixer", {.cellCount = 8, .keyNames = {}, .motorNames = {}, .commands = makeCommandSet({})}},

    // ID 7: Purger
    {
        PURGER, "Purger", {.cellCount = 16, .keyNames = {}, .motorNames = {}, .commands = makeCommandSet({PURGER_SET_STATE})}},

    // ID 8: Final Order
    {
        FINAL_ORDER, "FinalOrder", {.cellCount = 12, .keyNames = {}, .motorNames = {}, .commands = makeCommandSet({FINAL_RESET})}},

    // ID 9: Ball Gate
    {
        BALL_GATE, "BallGate", {.cellCount = 1, .keyNames = {}, .motorNames = {"Gate motor", "Reject motor"}, .commands = makeCommandSet({})}},

    // ID 10: Actuator
    {
        ACTUATOR, "Actuator", {.cellCount = 0, .keyNames = {}, .motorNames = {"Actuator 1", "Actuator 2"}, .commands = makeCommandSet({ACT_OPEN, ACT_CLOSE})}},

    // ID 11: The Wall
    {
        THE_WALL, "TheWall", {.cellCount = 0, .keyNames = {}, .motorNames = {}, .commands = makeCommandSet({})}},

    // ID 12: Scores
    {
        SCORES, "Scores", {.cellCount = 0, .keyNames = {}, .motorNames = {}, .commands = makeCommandSet({})}},

    // ID 13: Ball Base
    {
        BALL_BASE, "BallBase", {.cellCount = 0, .keyNames = {}, .motorNames = {}, .commands = makeCommandSet({})}}};

static const size_t DEVICE_COUNT = sizeof(DEVICE_CATALOG) / sizeof(DEVICE_CATALOG[0]);

// =================================================================================
// Implementation
// =================================================================================

const DeviceDefinition *DeviceConfigurations::getDefinition(DeviceType type)
{
        for (size_t i = 0; i < DEVICE_COUNT; i++)
        {
                if (DEVICE_CATALOG[i].type == type)
                {
                        return &DEVICE_CATALOG[i];
                }
        }
        return nullptr;
}

const char *DeviceConfigurations::getName(DeviceType type)
{
        const DeviceDefinition *def = getDefinition(type);
        return def ? def->name : "UNKNOWN";
}

CommandSet DeviceConfigurations::getMergedCommandSet(DeviceType type)
{
        CommandSet merged = makeCommandSet({CORE_HELLO, CORE_ACK, CORE_PING, CORE_RESET});

        const DeviceDefinition *def = getDefinition(type);
        if (def)
        {
                for (u8 i = 0; i < def->config.commands.count; i++)
                {
                        if (merged.count < MAX_COMMANDS)
                        {
                                merged.cmds[merged.count++] = def->config.commands.cmds[i];
                        }
                }
        }
        return merged;
}

u8 DeviceConfigurations::getMotorCount(DeviceType type)
{
        const DeviceDefinition *def = getDefinition(type);
        if (!def)
                return 0;

        u8 count = 0;
        for (int i = 0; i < MAX_MOTORS; i++)
        {
                if (def->config.motorNames[i] != nullptr)
                        count++;
        }
        return count;
}

const char *DeviceConfigurations::getKeyName(DeviceType type, u8 keyIndex)
{
        const DeviceDefinition *def = getDefinition(type);
        if (def && keyIndex < MAX_KEYS && def->config.keyNames[keyIndex] != nullptr)
        {
                return def->config.keyNames[keyIndex];
        }

        // Default names if not specified
        static char defaultName[4];
        if (keyIndex < 10)
                snprintf(defaultName, sizeof(defaultName), "%d", keyIndex);
        else
                snprintf(defaultName, sizeof(defaultName), "%c", 'A' + (keyIndex - 10));
        return defaultName;
}

const char *DeviceConfigurations::getMotorName(DeviceType type, u8 motorIndex)
{
        const DeviceDefinition *def = getDefinition(type);
        if (def && motorIndex < MAX_MOTORS && def->config.motorNames[motorIndex] != nullptr)
        {
                return def->config.motorNames[motorIndex];
        }
        return "Motor";
}

void DeviceConfigurations::printConfig(DeviceType type)
{
        const DeviceDefinition *def = getDefinition(type);
        if (!def)
        {
                Serial.printf("Device Type %d: [UNDEFINED]\n", type);
                return;
        }

        Serial.printf("Device Type %d: %s\n", def->type, def->name);
        Serial.printf("  - LEDs/Cells: %d\n", def->config.cellCount);

        // Print Commands
        Serial.print("  - Cmds: ");
        for (u8 i = 0; i < def->config.commands.count; i++)
        {
                Serial.printf("0x%02X ", def->config.commands.cmds[i]);
        }
        Serial.println();
}
