#pragma once

#include "msk.h"
#include "pixel.h"
#include "synth.h"
#include "animation.h"
#include "inputmanager.h"
#include "roomserial.h"
#include "colors.h"
#include "mcupins.h"
#include <Preferences.h>

// Core firmware modes
enum CoreMode
{
        MODE_INTERACTIVE,   // User button/keypad control
        MODE_ANIMATION,     // Automated animation
        MODE_REMOTE,        // Remote control via Room Bus
        MODE_TYPE_DETECTION // Type detection/calibration mode
};

// Status LED modes
enum StatusLedMode
{
        STATUS_OK,        // Solid ON - everything OK
        STATUS_I2C_ERROR, // Fast blink (5 Hz) - I2C communication error
        STATUS_TYPE_ERROR // Slow blink (1 Hz) - Invalid device type
};

class Core
{
public:
        // Constructor
        Core(PixelStrip *pixels, Synth *synth, Animation *animation,
             InputManager *inputManager, RoomSerial *roomBus);

        // Initialize core firmware
        void init();

        // Main core update loop
        void update();

        // Set core mode
        void setMode(CoreMode mode);
        CoreMode getMode() const { return m_mode; }

        // Get device type (0-31 from trimmer pot ADC)
        u8 getDeviceType() const { return m_deviceType; }

        // Get device type name string
        // Note: These are placeholder names at firmware level.
        // High-level App code can override these by implementing its own
        // type name lookup based on getDeviceType() for application-specific naming.
        const char *getDeviceTypeName() const;

        // Print boot report showing device configuration
        void printBootReport();

        // Status LED control
        void setStatusLed(StatusLedMode mode);
        void updateStatusLed(); // Call frequently from main loop

private:
        // Hardware references
        PixelStrip *m_pixels;
        Synth *m_synth;
        Animation *m_animation;
        InputManager *m_inputManager;
        RoomSerial *m_roomBus;

        // Core state
        CoreMode m_mode;
        int m_colorIndex;
        u8 m_deviceType; // Device type (0-31) stored in NVS, calibrated via ADC

        // Non-volatile storage for device type
        Preferences m_preferences;

        // Status LED state
        StatusLedMode m_statusLedMode;
        u32 m_lastLedToggle;
        bool m_ledState;

        // Type detection mode state
        CoreMode m_previousMode;    // Mode to return to after type detection
        u32 m_lastTypeRead;         // Last time device type was read and logged
        bool m_typeDetectionBlink;  // Blink state for type detection LED
        u8 m_lastDetectedType;      // Last detected type (to track changes)
        u8 m_typeBeforeCalibration; // Device type before entering calibration (for restore)

        // Color palette - defined in core.cpp
        static const u32 kColors[];
        static const size_t kColorCount;

        // Device type names (0-63) - defined in core.cpp
        // Placeholder names for firmware level. High-level App can override.
        // Currently using 0-31, expandable to 64 types in future
        static const char *kDeviceTypeNames[64];

        // MIDI note mapping for keypad - defined in core.cpp
        static const int kNoteMap[16];

        // Event handlers
        static void onInputEvent(InputEvent event);
        void handleInputEvent(InputEvent event);
        void handleButton1Press();
        void handleButtonLongPress();
        void handleKeypadPress(u8 keyIndex);
        void handleRoomBusFrame(const RoomFrame &frame);

        // Configuration
        u8 readDeviceType(bool verbose = true);
        void saveDeviceType(u8 type);
        u8 loadDeviceType();
        void clearStoredDeviceType(); // Factory reset

        // Type detection mode
        void enterTypeDetectionMode();
        void exitTypeDetectionMode();
        void updateTypeDetectionMode();

        // Singleton instance for static callback
        static Core *s_instance;
};
