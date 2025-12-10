#pragma once

#include "msk.h"
#include "pixel.h"
#include "synth.h"
#include "animation.h"
#include "inputmanager.h"
#include "roomserial.h"
#include "colors.h"
#include "mcupins.h"
#include "ioexpander.h" // For KEYPAD_SIZE
#include "matrixpanel.h"
#include <Preferences.h>

// Core firmware modes
enum CoreMode
{
        MODE_INTERACTIVE,    // User button/keypad control
        MODE_ANIMATION,      // Automated animation
        MODE_REMOTE,         // Remote control via Room Bus
        MODE_TYPE_DETECTION, // Type detection/calibration mode
        MODE_KEYPAD_TEST     // Keypad LED test mode (toggle LED on/off per key)
};

// Status LED pattern structure
struct LedPattern
{
        u32 timeOn;  // LED ON duration in milliseconds
        u32 timeOff; // LED OFF duration in milliseconds
};

// Status LED modes
enum StatusLedMode
{
        STATUS_OK,        // Long ON (3000ms ON, 100ms OFF) - Normal operation
        STATUS_I2C_ERROR, // Fast blink (100ms ON, 100ms OFF) - I2C communication error
        STATUS_TYPE_ERROR // Slow blink (500ms ON, 500ms OFF) - Invalid device type
};

class Core
{
public:
        // Constructor
        Core(PixelStrip *pixels, Synth *synth, Animation *animation,
             InputManager *inputManager, RoomSerial *roomBus);

        // System-wide initialization (call once from setup())
        static void systemInit(PixelStrip *pixels, Synth *synth, Animation *animation,
                               InputManager *inputManager, RoomSerial *roomBus,
                               Core *core, IOExpander *ioExpander, hw_timer_t **timer);

        // Initialize core firmware
        void init();

        // Main core update loop
        void update();

        // Refresh animations (call from main loop with ISR flag)
        void refreshAnimations(volatile bool &flag);

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

        // Logical keypad/LED abstraction layer (delegates to MatrixPanel)
        // These functions provide convenient access to the matrix panel
        u8 cellIndex(u8 x, u8 y) const;                     // Convert column(x), row(y) to logical cell index (0-15)
        void ledControl(u8 logicalIndex, u32 color);        // Set LED color by logical index (hides physical wiring)
        void ledControl(u8 logicalIndex, u8 r, u8 g, u8 b); // Set LED color by logical index with RGB values

        // Direct access to matrix panel for advanced operations
        MatrixPanel *getMatrixPanel() { return m_matrixPanel; }

private:
        // Hardware references
        PixelStrip *m_pixels;
        Synth *m_synth;
        Animation *m_animation;
        InputManager *m_inputManager;
        RoomSerial *m_roomBus;
        MatrixPanel *m_matrixPanel; // Keypad+LED matrix abstraction

        // Core state
        CoreMode m_mode;
        int m_colorIndex;
        u8 m_deviceType;       // Device type (0-31) stored in NVS, calibrated via ADC
        bool m_pixelCheckDone; // Track if pixel check has been performed

        // Keypad test mode state
        bool m_keypadLedStates[KEYPAD_SIZE]; // Track LED state for each keypad button (on/off)

        // Non-volatile storage for device type
        Preferences m_preferences;

        // Status LED state
        StatusLedMode m_statusLedMode;
        u32 m_lastLedToggle;
        bool m_ledState;

        // LED patterns for different modes
        static const LedPattern LED_PATTERN_OK;
        static const LedPattern LED_PATTERN_I2C_ERROR;
        static const LedPattern LED_PATTERN_TYPE_ERROR;

        // Get pattern for current mode
        const LedPattern &getCurrentLedPattern() const;

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

        // Keypad test mode
        void enterKeypadTestMode();
        void exitKeypadTestMode();
        void handleKeypadTestPress(u8 keyIndex);

        // Singleton instance for static callback
        static Core *s_instance;
};
