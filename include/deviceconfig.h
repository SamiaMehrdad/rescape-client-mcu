/**
 * @file deviceconfig.h
 * @brief Device-specific hardware configuration system
 *
 * Defines hardware configurations for each device type, including:
 * - Number of matrix cells (keypad + LED) used
 * - Number and names of motors
 * - Number and names of switches/sensors
 *
 * Simple, maintainable configuration with just counts and optional names.
 */

#ifndef DEVICECONFIG_H
#define DEVICECONFIG_H

#include "msk.h"
#include "ioexpander.h"
#include "roombus.h" // Room Bus server command IDs
#include <initializer_list>

// Maximum device types supported (0-63)
constexpr u8 MAX_DEVICE_TYPES = 64;

// Maximum hardware components
constexpr u8 MAX_MOTORS = 4;
constexpr u8 MAX_KEYS = 16;
constexpr u8 MAX_COMMANDS = 8;

// --- Device Types ---
// Manually defined for code readability.
// These IDs must match the 'id' field in ALL_DEVICES in deviceconfig.cpp
enum DeviceType : u8
{
        TERMINAL = 0,
        GLOW_BUTTON = 1,
        NUM_BOX = 2,
        TIMER = 3,
        GLOW_DOTS = 4,
        QB = 5,
        RGB_MIXER = 6,
        PROTO = 7,
        FINAL_ORDER = 8,
        BALL_GATE = 9,
        ACTUATOR = 10,
        THE_WALL = 11,
        SCORES = 12,
        BALL_BASE = 13,
        PURGER = 14,
        // ... types 15-63 are generic/reserved
};

// --- Data Structures ---

// 1. Command Set Wrapper
struct CommandSet
{
        RoomServerCommand cmds[MAX_COMMANDS];
        u8 count;
};

// Helper to create sets easily: makeCommandSet({CMD_A, CMD_B})
inline CommandSet makeCommandSet(std::initializer_list<RoomServerCommand> list)
{
        CommandSet cs{};
        size_t n = list.size();
        if (n > MAX_COMMANDS)
                n = MAX_COMMANDS;
        for (size_t i = 0; i < n; i++)
                cs.cmds[i] = *(list.begin() + i);
        cs.count = (u8)n;
        return cs;
}

// 2. Hardware Configuration
struct DeviceConfig
{
        u16 cellCount;                      // Number of LEDs/Keys
        const char *keyNames[MAX_KEYS];     // Optional key names (nullptr = default)
        const char *motorNames[MAX_MOTORS]; // Optional motor names (nullptr = unused)
        CommandSet commands;                // Supported commands
};

// 3. The Master Device Definition
struct DeviceDefinition
{
        DeviceType type;     // The Device Type (Class)
        const char *name;    // Display name (e.g. "GlowButton")
        DeviceConfig config; // Hardware setup
};

// --- Public API ---

class DeviceConfigurations
{
public:
        // Get definition for a specific Type
        static const DeviceDefinition *getDefinition(DeviceType type);

        // Helpers (wrappers around getDefinition())
        static const char *getName(DeviceType type);
        static CommandSet getMergedCommandSet(DeviceType type); // Adds core commands
        static void printConfig(DeviceType type);

        // Legacy/Helper accessors
        static u8 getMotorCount(DeviceType type);
        static const char *getKeyName(DeviceType type, u8 keyIndex);
        static const char *getMotorName(DeviceType type, u8 motorIndex);
};

#endif // DEVICECONFIG_H
