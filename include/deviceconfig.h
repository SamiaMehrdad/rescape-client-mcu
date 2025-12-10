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

// Maximum device types supported (0-63)
constexpr u8 MAX_DEVICE_TYPES = 64;

// Maximum hardware components
constexpr u8 MAX_MOTORS = 2;
constexpr u8 MAX_SWITCHES = 4;

/**
 * @brief Device Type Definition List (X-Macro Pattern)
 *
 * SINGLE SOURCE OF TRUTH for all device types.
 * Format: DEVICE_TYPE(EnumName, "DisplayName")
 *
 * This macro generates BOTH the enum and the names array automatically.
 * To add a new device type:
 * 1. Replace one TYPE_XX line with your new device
 * 2. Use SCREAMING_SNAKE_CASE for enum, PascalCase for display name
 *
 * All 64 types are compiled into firmware.
 * Runtime device type is selected via potentiometer reading saved to NVS.
 */
#define DEVICE_TYPE_LIST                       \
        DEVICE_TYPE(TERMINAL, "Terminal")      \
        DEVICE_TYPE(GLOW_BUTTON, "GlowButton") \
        DEVICE_TYPE(NUM_BOX, "NumBox")         \
        DEVICE_TYPE(TIMER, "Timer")            \
        DEVICE_TYPE(GLOW_DOTS, "GlowDots")     \
        DEVICE_TYPE(QB, "QB")                  \
        DEVICE_TYPE(RGB_MIXER, "RGBMixer")     \
        DEVICE_TYPE(BOMB, "Bomb")              \
        DEVICE_TYPE(FINAL_ORDER, "FinalOrder") \
        DEVICE_TYPE(BALL_GATE, "BallGate")     \
        DEVICE_TYPE(ACTUATOR, "Actuator")      \
        DEVICE_TYPE(THE_WALL, "TheWall")       \
        DEVICE_TYPE(SCORES, "Scores")          \
        DEVICE_TYPE(BALL_BASE, "BallBase")     \
        DEVICE_TYPE(TYPE_14, "TYPE_14")        \
        DEVICE_TYPE(TYPE_15, "TYPE_15")        \
        DEVICE_TYPE(TYPE_16, "TYPE_16")        \
        DEVICE_TYPE(TYPE_17, "TYPE_17")        \
        DEVICE_TYPE(TYPE_18, "TYPE_18")        \
        DEVICE_TYPE(TYPE_19, "TYPE_19")        \
        DEVICE_TYPE(TYPE_20, "TYPE_20")        \
        DEVICE_TYPE(TYPE_21, "TYPE_21")        \
        DEVICE_TYPE(TYPE_22, "TYPE_22")        \
        DEVICE_TYPE(TYPE_23, "TYPE_23")        \
        DEVICE_TYPE(TYPE_24, "TYPE_24")        \
        DEVICE_TYPE(TYPE_25, "TYPE_25")        \
        DEVICE_TYPE(TYPE_26, "TYPE_26")        \
        DEVICE_TYPE(TYPE_27, "TYPE_27")        \
        DEVICE_TYPE(TYPE_28, "TYPE_28")        \
        DEVICE_TYPE(TYPE_29, "TYPE_29")        \
        DEVICE_TYPE(TYPE_30, "TYPE_30")        \
        DEVICE_TYPE(TYPE_31, "TYPE_31")        \
        DEVICE_TYPE(TYPE_32, "TYPE_32")        \
        DEVICE_TYPE(TYPE_33, "TYPE_33")        \
        DEVICE_TYPE(TYPE_34, "TYPE_34")        \
        DEVICE_TYPE(TYPE_35, "TYPE_35")        \
        DEVICE_TYPE(TYPE_36, "TYPE_36")        \
        DEVICE_TYPE(TYPE_37, "TYPE_37")        \
        DEVICE_TYPE(TYPE_38, "TYPE_38")        \
        DEVICE_TYPE(TYPE_39, "TYPE_39")        \
        DEVICE_TYPE(TYPE_40, "TYPE_40")        \
        DEVICE_TYPE(TYPE_41, "TYPE_41")        \
        DEVICE_TYPE(TYPE_42, "TYPE_42")        \
        DEVICE_TYPE(TYPE_43, "TYPE_43")        \
        DEVICE_TYPE(TYPE_44, "TYPE_44")        \
        DEVICE_TYPE(TYPE_45, "TYPE_45")        \
        DEVICE_TYPE(TYPE_46, "TYPE_46")        \
        DEVICE_TYPE(TYPE_47, "TYPE_47")        \
        DEVICE_TYPE(TYPE_48, "TYPE_48")        \
        DEVICE_TYPE(TYPE_49, "TYPE_49")        \
        DEVICE_TYPE(TYPE_50, "TYPE_50")        \
        DEVICE_TYPE(TYPE_51, "TYPE_51")        \
        DEVICE_TYPE(TYPE_52, "TYPE_52")        \
        DEVICE_TYPE(TYPE_53, "TYPE_53")        \
        DEVICE_TYPE(TYPE_54, "TYPE_54")        \
        DEVICE_TYPE(TYPE_55, "TYPE_55")        \
        DEVICE_TYPE(TYPE_56, "TYPE_56")        \
        DEVICE_TYPE(TYPE_57, "TYPE_57")        \
        DEVICE_TYPE(TYPE_58, "TYPE_58")        \
        DEVICE_TYPE(TYPE_59, "TYPE_59")        \
        DEVICE_TYPE(TYPE_60, "TYPE_60")        \
        DEVICE_TYPE(TYPE_61, "TYPE_61")        \
        DEVICE_TYPE(TYPE_62, "TYPE_62")        \
        DEVICE_TYPE(TYPE_63, "TYPE_63")

// Generate enum from DEVICE_TYPE_LIST
#define DEVICE_TYPE(name, str) name,
enum DeviceType : u8
{
        DEVICE_TYPE_LIST
};
#undef DEVICE_TYPE

// Generate names array from DEVICE_TYPE_LIST
#define DEVICE_TYPE(name, str) str,
constexpr const char *DEVICE_TYPE_NAMES[MAX_DEVICE_TYPES] = {
    DEVICE_TYPE_LIST};
#undef DEVICE_TYPE

/**
 * @brief Named hardware component
 * Simple structure for motors and switches with optional names
 */
struct NamedComponent
{
        const char *name; // Component name/purpose (nullptr if unused)
};

/**
 * @brief Complete device hardware configuration
 * Simple configuration with component names (nullptr = not used)
 * 
 * Note: Device type is implicit from array index in kConfigs[].
 * kConfigs[0] = TERMINAL, kConfigs[1] = GLOW_BUTTON, etc.
 */
struct DeviceConfig
{
        // Matrix cells (keypad + LED pairs)
        u8 matrixCellCount; // Number of matrix cells used (0-16)

        // Motors (0-2 motors available) - nullptr name = not used
        NamedComponent motors[MAX_MOTORS]; // Motor names/purposes

        // Switches/Sensors (0-4 available) - nullptr name = not used
        NamedComponent switches[MAX_SWITCHES]; // Switch names/purposes

        bool isConfigured; // Is this type fully configured?
};

/**
 * @brief Device configuration database and utility functions
 */
class DeviceConfigurations
{
public:
        /**
         * @brief Get configuration for a specific device type
         * @param deviceType Device type ID (0-63)
         * @return Pointer to DeviceConfig, or nullptr if invalid
         */
        static const DeviceConfig *getConfig(u8 deviceType);

        /**
         * @brief Get device type name
         * @param deviceType Device type ID (0-63)
         * @return Device name string, or "INVALID" if out of range
         */
        static const char *getDeviceName(u8 deviceType);

        /**
         * @brief Get number of matrix cells used by device
         * @param deviceType Device type ID (0-63)
         * @return Number of cells (0-16)
         */
        static u8 getMatrixCellCount(u8 deviceType);

        /**
         * @brief Get number of motors used by device
         * @param deviceType Device type ID (0-63)
         * @return Number of motors (0-2)
         */
        static u8 getMotorCount(u8 deviceType);

        /**
         * @brief Get number of switches used by device
         * @param deviceType Device type ID (0-63)
         * @return Number of switches (0-4)
         */
        static u8 getSwitchCount(u8 deviceType);

        /**
         * @brief Get name/purpose of a specific motor
         * @param deviceType Device type ID (0-63)
         * @param motorIndex Motor index (0-1)
         * @return Motor name, or nullptr if not configured
         */
        static const char *getMotorName(u8 deviceType, u8 motorIndex);

        /**
         * @brief Get name/purpose of a specific switch
         * @param deviceType Device type ID (0-63)
         * @param switchIndex Switch index (0-3)
         * @return Switch name, or nullptr if not configured
         */
        static const char *getSwitchName(u8 deviceType, u8 switchIndex);

        /**
         * @brief Print device configuration summary to Serial
         * @param deviceType Device type ID (0-63)
         */
        static void printConfig(u8 deviceType);

        /**
         * @brief Print hardware configuration details to Serial
         * Displays matrix cells, motors, and switches for a device type.
         * Can be used in boot reports and device type selection.
         * @param deviceType Device type ID (0-63)
         * @param indent String to prepend to each line (e.g., "│   " for box formatting)
         */
        static void printHardwareConfig(u8 deviceType, const char *indent = "");

private:
        // Configuration database (defined in deviceconfig.cpp)
        static const DeviceConfig kConfigs[MAX_DEVICE_TYPES];
};

#endif // DEVICECONFIG_H
