/************************* core.cpp ****************************
 * Core Firmware Implementation
 * Handles device type configuration, mode management, and hardware control
 * Created by MSK, November 2025
 * Architecture: Core (firmware) + App (high-level) separation
 ***************************************************************/

#include "core.h"
#include "deviceconfig.h"
#include <Arduino.h>
#include "mcupins.h"
#include "buttons.h"
#include "watchdog.h"
#include "esptimer.h"
#include "ioexpander.h"
#include <Wire.h>
#include "esp_log.h"

// Timer ISR interval configuration (defined in main.cpp)
extern const u8 ISR_INTERVAL_MS;
extern const u8 ANIM_REFRESH_MS;
extern volatile bool pixelUpdateFlag;

// Timer ISR: refresh button logic and trigger pixel updates (defined in main.cpp)
extern void IRAM_ATTR refreshTimer();

//============================================================================
// CONFIGURATION CONSTANTS
//============================================================================

// ADC configuration for device type detection
namespace ADCConfig
{
        constexpr int DISCONNECT_THRESHOLD = 30; // ADC value below this = disconnected pot
        constexpr int NOISE_THRESHOLD = 200;     // ADC range above this = noisy/bad connection
        constexpr int NUM_SAMPLES = 32;          // Number of ADC samples to average
        constexpr int STEP_SIZE = 64;            // ADC units per device type (4096/64 = 64 types)
        constexpr int PIN_STABILIZE_MS = 10;     // Delay after pinMode change (ms)
        constexpr int SAMPLE_DELAY_US = 100;     // Delay between ADC samples (microseconds)
}

// Instruction: add new device types in DEVICE_TYPE_LIST and kConfigs (deviceconfig.*);
// bump MAX_CURRENT_TYPE if you want the potentiometer map to expose IDs above 31.
// Device type limits
namespace TypeLimits
{
        constexpr u8 MAX_CURRENT_TYPE = 31; // Current maximum type (5-bit, 0-31)
        constexpr u8 MAX_FUTURE_TYPE = 63;  // Future maximum type (6-bit, 0-63)
        constexpr u8 INVALID_TYPE = 0xFF;   // Marker for invalid/disconnected
}

// Type detection mode timing
namespace DetectionTiming
{
        constexpr u32 READ_INTERVAL_MS = 500; // How often to read ADC in detection mode (ms)
        constexpr u32 LED_FLASH_MS = 100;     // LED flash duration (ms)
        constexpr u32 ERROR_BLINK_MS = 50;    // Error pattern blink duration (ms)
        constexpr u32 INITIAL_DELAY_MS = 500; // Delay before first reading in detection mode
}

//============================================================================
// STATIC DATA
//============================================================================

// Static instance pointer for callback
Core *Core::s_instance = nullptr;

// Color palette for cycling
const u32 Core::kColors[] = {CLR_WT, CLR_PN, CLR_PR, CLR_MG, CLR_OR,
                             CLR_YL, CLR_RD, CLR_BL, CLR_CY, CLR_GR};
const size_t Core::kColorCount = sizeof(kColors) / sizeof(kColors[0]);

// Note map for keypad (16 keys -> 16 notes)
const int Core::kNoteMap[16] = {
    NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4,
    NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5,
    NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5,
    NOTE_A5, NOTE_B5, NOTE_C6, NOTE_D6};

// LED patterns for status indication
const LedPattern Core::kLedPatterns[] = {
    {100, 3000}, // STATUS_OK: Long ON, short OFF (mostly off)
    {100, 100},  // STATUS_I2C_ERROR: Fast blink (5 Hz)
    {500, 500},  // STATUS_TYPE_ERROR: Slow blink (1 Hz)
    {100, 400}   // STATUS_DEVICE_DETECTION: Detection blink (2 Hz)
};

//============================================================================
// CONSTRUCTOR
//============================================================================

Core::Core(PixelStrip *pixels, Synth *synth, Animation *animation,
           InputManager *inputManager, RoomSerial *roomBus, IOExpander *ioExpander)
    : m_pixels(pixels),
      m_synth(synth),
      m_animation(animation),
      m_inputManager(inputManager),
      m_roomBus(roomBus),
      m_ioExpander(ioExpander),
      m_matrixPanel(new MatrixPanel(pixels)), // Initialize matrix panel
      m_mode(MODE_INTERACTIVE),
      m_colorIndex(0),
      m_deviceType(0),
      m_pixelCheckDone(false),
      m_statusLedMode(STATUS_OK),
      m_lastLedToggle(0),
      m_ledState(false),
      m_previousMode(MODE_INTERACTIVE),
      m_lastTypeRead(0),
      m_typeDetectionBlink(false),
      m_lastDetectedType(0xFF),  // Invalid initial value to force first log
      m_typeBeforeCalibration(0) // Will be set when entering calibration
{
        s_instance = this;

        // Initialize keypad LED states to off
        for (int i = 0; i < 16; i++)
        {
                m_keypadLedStates[i] = false;
        }
}

//============================================================================
// PUBLIC METHODS
//============================================================================

// System-wide initialization - sets up all hardware and firmware modules
void Core::begin()
{
        delay(500);
        Serial.begin(115200);
        delay(1000);

        // Disable I2C error logging to reduce Serial spam
        esp_log_level_set("i2c", ESP_LOG_NONE);

        // Initialize I2C for I/O Expander
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

        // Initialize I/O Expander
        bool i2cOk = false;
        if (m_ioExpander->begin())
        {
                m_ioExpander->stopAllMotors();
                i2cOk = true;
        }

        // Initialize pixel strip
        m_pixels->begin();

        // Initialize button handling (single button now)
        initButtons(BTN_1_PIN);

        // Configure hardware timer for button updates and animation timing
        // Note: We don't store the timer handle as we don't need to stop it
        ESPTimer::begin(0, ISR_INTERVAL_MS, &refreshTimer);

        // Initialize watchdog (1 second timeout)
        Watchdog::begin(1, true);

        // Initialize synthesizer
        m_synth->init(SOUND_DEFAULT);

        // Initialize RS-485 communication for Room Bus
        m_roomBus->begin();

        // Initialize core firmware modules
        m_animation->init();    // Animation system
        m_inputManager->init(); // Input management for keypad and switches
        init();                 // Core firmware logic

        // Set status LED based on I2C health
        if (!i2cOk)
        {
                setStatusLed(STATUS_I2C_ERROR);
        }
        else
        {
                setStatusLed(STATUS_OK);
        }

        // Print comprehensive boot report
        printBootReport();
}

void Core::init()
{
        m_mode = MODE_INTERACTIVE;
        m_colorIndex = 0;

        // Initialize status LED pin
        pinMode(STATUS_LED_PIN, OUTPUT);
        digitalWrite(STATUS_LED_PIN, LOW);
        m_statusLedMode = STATUS_OK;
        m_lastLedToggle = 0;
        m_ledState = false;

        // Initialize NVS (Non-Volatile Storage)
        m_preferences.begin("core", false); // namespace: "core", readonly: false

        // Try to load device type from NVS first
        m_deviceType = loadDeviceType();

        if (m_deviceType == 0xFF) // 0xFF means not found in NVS
        {
                // First boot or factory reset - read from ADC and save
                m_deviceType = readDeviceType();

                if (m_deviceType == 0xFF)
                {
                        // Potentiometer disconnected or invalid reading
                        Serial.println("⚠️  WARNING: Cannot read device type from ADC!");
                        Serial.println("   Please check potentiometer connection.");
                        m_deviceType = 0;                    // Temporary default
                        m_statusLedMode = STATUS_TYPE_ERROR; // Show error on LED
                }
                else
                {
                        // Valid reading - save it
                        saveDeviceType(m_deviceType);
                }
        }

        // Set up input callback
        m_inputManager->setCallback(onInputEvent);
}

void Core::update()
{
        // Poll inputs
        m_inputManager->poll();

        // Update type detection mode if active
        if (m_mode == MODE_TYPE_DETECTION)
        {
                updateTypeDetectionMode();
        }

        // Update status LED
        updateStatusLed();

        // Check for Room Bus commands
        RoomFrame rxFrame;
        if (m_roomBus->receiveFrame(&rxFrame))
        {
                handleRoomBusFrame(rxFrame);
        }
}

void Core::refreshAnimations(volatile bool &flag)
{
        m_animation->refresh(flag);
}

//============================================================================
// CONFIGURATION
//============================================================================

u8 Core::readDeviceType(bool verbose)
{
        using namespace ADCConfig;
        using namespace TypeLimits;

        // First, check for disconnected pot using pull-down
        pinMode(CONFIG_ADC_PIN, INPUT_PULLDOWN);
        delay(PIN_STABILIZE_MS); // Allow pull-down to stabilize

        int checkReading = analogRead(CONFIG_ADC_PIN);

        // If reading is very low, pot is likely disconnected
        // Threshold set well below TYPE_00 range (0-63)
        if (checkReading < DISCONNECT_THRESHOLD)
        {
                if (verbose)
                {
                        Serial.print("Config ADC: ");
                        Serial.print(checkReading);
                        Serial.println(" (range: -) -> DISCONNECTED/INVALID");
                }
                return INVALID_TYPE; // Disconnected
        }

        // Pot appears connected - switch to normal INPUT mode for accurate reading
        // This prevents pull-down from affecting the voltage divider
        pinMode(CONFIG_ADC_PIN, INPUT);
        delay(PIN_STABILIZE_MS); // Allow pin to stabilize without pull-down

        // Take multiple readings and average for stability
        int sum = 0;
        int minReading = 4095;
        int maxReading = 0;

        for (int i = 0; i < NUM_SAMPLES; i++)
        {
                int reading = analogRead(CONFIG_ADC_PIN);
                sum += reading;

                if (reading < minReading)
                        minReading = reading;
                if (reading > maxReading)
                        maxReading = reading;

                delayMicroseconds(SAMPLE_DELAY_US);
        }

        int adcValue = sum / NUM_SAMPLES;
        int adcRange = maxReading - minReading;

        // Check for excessive noise (bad connection)
        bool isNoisy = (adcRange > NOISE_THRESHOLD);

        // Only log if verbose mode is enabled
        if (verbose)
        {
                Serial.print("Config ADC: ");
                Serial.print(adcValue);
                Serial.print(" (range: ");
                Serial.print(adcRange);
                Serial.print(")");

                if (isNoisy)
                {
                        Serial.println(" -> NOISY/INVALID");
                        return INVALID_TYPE; // Invalid reading
                }
        }

        // If noisy in non-verbose mode, still return invalid
        if (isNoisy)
        {
                return INVALID_TYPE;
        }

        // Convert ADC value to device type (0-31 for 5-bit, expandable to 0-63 for 6-bit)
        // Full ADC range: 0-4095
        // Using step size of 64 (instead of 128) allows future expansion to 64 types
        // Current implementation: Uses types 0-31 (lower half)
        // Future expansion: Can use types 32-63 (upper half) without firmware changes
        u8 deviceType = adcValue / STEP_SIZE; // 4096 / 64 = 64 possible types

        // Currently clamp to 0-31 range (5-bit addressing)
        // Remove this clamp in future to enable full 0-63 range (6-bit addressing)
        if (deviceType > MAX_CURRENT_TYPE)
                deviceType = MAX_CURRENT_TYPE;

        if (verbose)
        {
                Serial.print(" -> Type ");
                Serial.print(deviceType);
                Serial.print(" (");
                Serial.print(DeviceConfigurations::getDeviceName(deviceType));
                Serial.println(")");
        }

        return deviceType;
}

void Core::printBootReport()
{
        Serial.println("\n╔════════════════════════════════════════════════════════════╗");
        Serial.println("║            ESCAPE ROOM CLIENT - BOOT REPORT                ║");
        Serial.println("╚════════════════════════════════════════════════════════════╝");
        Serial.println();

        // Device identification
        Serial.println("┌─ DEVICE CONFIGURATION ─────────────────────────────────────┐");
        Serial.print("│ Device Type:       ");
        Serial.print(getDeviceTypeName());
        Serial.print(" (Index: ");
        Serial.print(m_deviceType);
        Serial.println(")");

        // Storage status
        Serial.print("│ Configuration:     ");
        u8 storedType = m_preferences.getUChar("deviceType", 0xFF);
        if (storedType == 0xFF)
        {
                Serial.println("Not saved (temporary)");
        }
        else
        {
                Serial.print("Stored in NVS");
                if (storedType != m_deviceType)
                {
                        Serial.print(" (warning: mismatch!)");
                }
                Serial.println();
        }

        // Device hardware configuration
        Serial.println("│");
        Serial.println("│ Hardware Config:");
        DeviceConfigurations::printHardwareConfig(m_deviceType, "│   ");

        // Mode status
        Serial.print("│ Operating Mode:    ");
        switch (m_mode)
        {
        case MODE_INTERACTIVE:
                Serial.println("INTERACTIVE (manual control)");
                break;
        case MODE_ANIMATION:
                Serial.println("ANIMATION (auto sequences)");
                break;
        case MODE_REMOTE:
                Serial.println("REMOTE (Room Bus control)");
                break;
        case MODE_TYPE_DETECTION:
                Serial.println("TYPE_DETECTION (calibration)");
                break;
        case MODE_KEYPAD_TEST:
                Serial.println("KEYPAD_TEST (LED toggle test)");
                break;
        }

        // Status LED
        Serial.print("│ Status LED:        ");
        switch (m_statusLedMode)
        {
        case STATUS_OK:
                Serial.println("OK");
                break;
        case STATUS_I2C_ERROR:
                Serial.println("I2C ERROR");
                break;
        case STATUS_TYPE_ERROR:
                Serial.println("TYPE MISMATCH");
                break;
        }

        Serial.println("└────────────────────────────────────────────────────────────┘");

        // Hardware info
        Serial.println();
        Serial.println("┌─ HARDWARE INFO ────────────────────────────────────────────┐");

        // Chip info
        Serial.print("│ Chip:              ESP32-C3 @ ");
        Serial.print(ESP.getCpuFreqMHz());
        Serial.println(" MHz");

        Serial.print("│ Flash:             ");
        Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
        Serial.println(" MB");

        Serial.print("│ Free Heap:         ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes");

        // MAC address
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        Serial.print("│ MAC Address:       ");
        for (int i = 0; i < 6; i++)
        {
                if (mac[i] < 16)
                        Serial.print("0");
                Serial.print(mac[i], HEX);
                if (i < 5)
                        Serial.print(":");
        }
        Serial.println();

        Serial.println("└────────────────────────────────────────────────────────────┘");

        // Firmware info
        Serial.println();
        Serial.println("┌─ FIRMWARE INFO ────────────────────────────────────────────┐");
        Serial.println("│ Name:              Escape Room Client Core Firmware");
        Serial.println("│ Version:           1.0.0");
        Serial.println("│ Build Date:        " __DATE__ " " __TIME__);
        Serial.println("│ Architecture:      Core + App separation");
        Serial.println("└────────────────────────────────────────────────────────────┘");

        Serial.println();
        // if status is not ok, show a warning message
        if (m_statusLedMode != STATUS_OK)
        {
                Serial.println("⚠️  WARNING: Device status indicates an issue!");
        }
        Serial.println("ℹ️  To reconfigure device type: Long-press boot button");
        Serial.println("ℹ️  Device ready for operation");
        Serial.println();
}

void Core::saveDeviceType(u8 type)
{
        using namespace TypeLimits;

        // Validate and clamp to valid range
        if (type > MAX_CURRENT_TYPE && type != INVALID_TYPE)
        {
                Serial.print("WARNING: Device type ");
                Serial.print(type);
                Serial.print(" out of range. Clamping to ");
                Serial.println(MAX_CURRENT_TYPE);
                type = MAX_CURRENT_TYPE;
        }

        // Don't save invalid type
        if (type == INVALID_TYPE)
        {
                Serial.println("ERROR: Cannot save invalid device type to NVS.");
                return;
        }

        // Save to NVS
        size_t bytesWritten = m_preferences.putUChar("deviceType", type);
        if (bytesWritten == 0)
        {
                Serial.println("ERROR: Failed to save device type to NVS!");
        }
        else
        {
                Serial.print("Device type ");
                Serial.print(type);
                Serial.println(" saved to NVS.");
        }
}

u8 Core::loadDeviceType()
{
        using namespace TypeLimits;

        // Returns INVALID_TYPE (0xFF) if key doesn't exist (first boot or after factory reset)
        u8 type = m_preferences.getUChar("deviceType", INVALID_TYPE);

        // Validate loaded type
        if (type != INVALID_TYPE && type > MAX_CURRENT_TYPE)
        {
                Serial.print("WARNING: Loaded invalid device type ");
                Serial.print(type);
                Serial.println(" from NVS. Treating as unconfigured.");
                return INVALID_TYPE;
        }

        return type;
}

void Core::clearStoredDeviceType()
{
        bool success = m_preferences.remove("deviceType");
        if (success)
        {
                Serial.println("Stored device type cleared (factory reset).");
                Serial.println("Reboot to read device type from ADC.");
        }
        else
        {
                Serial.println("WARNING: Failed to clear device type from NVS.");
        }
}

const char *Core::getDeviceTypeName() const
{
        using namespace TypeLimits;

        // Ensure device type is in valid range
        if (m_deviceType > MAX_FUTURE_TYPE)
                return "INVALID";

        return DeviceConfigurations::getDeviceName(m_deviceType);
}

//============================================================================
// EVENT HANDLING
//============================================================================

void Core::setMode(CoreMode mode)
{
        // Validate mode
        if (mode != MODE_INTERACTIVE && mode != MODE_ANIMATION &&
            mode != MODE_REMOTE && mode != MODE_TYPE_DETECTION &&
            mode != MODE_KEYPAD_TEST)
        {
                Serial.print("ERROR: Invalid mode ");
                Serial.println((int)mode);
                return;
        }

        m_mode = mode;

        switch (mode)
        {
        case MODE_INTERACTIVE:
                m_animation->stop();
                Serial.println("Mode: INTERACTIVE");
                break;

        case MODE_ANIMATION:
                Serial.println("Mode: ANIMATION");
                break;

        case MODE_REMOTE:
                m_animation->stop();
                Serial.println("Mode: REMOTE");
                break;

        case MODE_TYPE_DETECTION:
                // Don't log here - enterTypeDetectionMode() handles it
                break;

        case MODE_KEYPAD_TEST:
                // Don't log here - enterKeypadTestMode() handles it
                break;
        }
}

//============================================================================
// CALLBACK WRAPPER
//============================================================================

// Static callback wrapper
void Core::onInputEvent(InputEvent event)
{
        if (s_instance)
        {
                s_instance->handleInputEvent(event);
        }
}

//============================================================================
// INPUT EVENT DISPATCHER
//============================================================================

void Core::handleInputEvent(InputEvent event)
{
        switch (event)
        {
        case INPUT_BTN1_PRESS:
                handleButton1Press();
                break;

        case INPUT_BTN1_LONG_PRESS:
                handleButtonLongPress();
                break;

        case INPUT_KEYPAD_0:
        case INPUT_KEYPAD_1:
        case INPUT_KEYPAD_2:
        case INPUT_KEYPAD_3:
        case INPUT_KEYPAD_4:
        case INPUT_KEYPAD_5:
        case INPUT_KEYPAD_6:
        case INPUT_KEYPAD_7:
        case INPUT_KEYPAD_8:
        case INPUT_KEYPAD_9:
        case INPUT_KEYPAD_10:
        case INPUT_KEYPAD_11:
        case INPUT_KEYPAD_12:
        case INPUT_KEYPAD_13:
        case INPUT_KEYPAD_14:
        case INPUT_KEYPAD_15:
                // In keypad test mode, toggle LED instead of playing note
                if (m_mode == MODE_KEYPAD_TEST)
                {
                        handleKeypadTestPress(m_inputManager->getKeypadNote(event));
                }
                else
                {
                        handleKeypadPress(m_inputManager->getKeypadNote(event));
                }
                break;

        default:
                break;
        }
}

//============================================================================
// BUTTON EVENT HANDLERS
//============================================================================

void Core::handleButton1Press()
{
        // First button press after boot: run pixel check
        if (!m_pixelCheckDone)
        {
                Serial.println("\n*** First button press - running pixel check... ***");
                m_pixelCheckDone = true;
                m_pixels->pixelCheck(200); // 200ms delay between each LED
                Serial.println("*** Pixel check complete. Next button press will cycle colors. ***\n");
                return;
        }

        Serial.println("Button press: cycle colors");

        // Stop animation when changing colors manually
        if (m_animation->isActive())
        {
                m_animation->stop();
        }

        // Cycle color forward
        m_colorIndex = (m_colorIndex + 1) % kColorCount;

        // Create a rainbow pattern across the LEDs
        for (int i = 0; i < m_pixels->getCount(); i++)
        {
                int idx = (m_colorIndex + i) % kColorCount;
                m_pixels->setColor(i, kColors[idx]);
        }
        m_pixels->show(); // Actually update the LEDs!

        // Play sound feedback
        m_synth->setWaveform(WAVE_TRIANGLE);
        m_synth->setADSR(5, 120, 100, 180);
        m_synth->playNote(NOTE_GS4, 300, 250);
}

void Core::handleButtonLongPress()
{
        // Cycle through special modes: Normal -> Keypad Test -> Type Detection -> Normal
        if (m_mode == MODE_KEYPAD_TEST)
        {
                // Exit keypad test, enter type detection
                exitKeypadTestMode();
                enterTypeDetectionMode();
        }
        else if (m_mode == MODE_TYPE_DETECTION)
        {
                // Exit type detection, return to normal
                exitTypeDetectionMode();
        }
        else
        {
                // Enter keypad test mode
                enterKeypadTestMode();
        }
}

void Core::handleKeypadPress(u8 keyIndex)
{
        int note = kNoteMap[keyIndex];

        m_synth->setWaveform(WAVE_SINE);
        m_synth->setADSR(5, 50, 100, 100);
        m_synth->playNote(note, 150, 100);
}

void Core::handleRoomBusFrame(const RoomFrame &frame)
{
        Serial.print("Room Bus frame received! Addr: 0x");
        Serial.print(frame.addr, HEX);
        Serial.print(" Cmd_srv: 0x");
        Serial.print(frame.cmd_srv, HEX);
        Serial.print(" Cmd_dev: 0x");
        Serial.println(frame.cmd_dev, HEX);

        // Handle color set command
        if (frame.cmd_srv == GLOW_SET_COLOR)
        {
                // Extract RGB from parameters
                u8 r = frame.p[0];
                u8 g = frame.p[1];
                u8 b = frame.p[2];
                u32 color = (r << 16) | (g << 8) | b;

                // Stop animation and set color
                m_animation->stop();
                m_pixels->setAll(color);
                m_pixels->show();

                Serial.println("Color set via Room Bus command");
        }
}

//============================================================================
// STATUS LED CONTROL
//============================================================================

void Core::setStatusLed(StatusLedMode mode)
{
        // Validate mode
        if (mode != STATUS_OK && mode != STATUS_I2C_ERROR &&
            mode != STATUS_TYPE_ERROR && mode != STATUS_DEVICE_DETECTION)
        {
                Serial.print("ERROR: Invalid status LED mode ");
                Serial.println((int)mode);
                return;
        }

        m_statusLedMode = mode;
        m_lastLedToggle = millis();
        m_ledState = true; // Start with LED ON
        digitalWrite(STATUS_LED_PIN, HIGH);
}

void Core::updateStatusLed()
{
        u32 now = millis();
        const LedPattern &pattern = kLedPatterns[m_statusLedMode];

        // Determine current interval (ON time or OFF time)
        u32 interval = m_ledState ? pattern.timeOn : pattern.timeOff;

        // Toggle LED when interval expires
        if (now - m_lastLedToggle >= interval)
        {
                m_lastLedToggle = now;
                m_ledState = !m_ledState;
                digitalWrite(STATUS_LED_PIN, m_ledState ? HIGH : LOW);
        }
}

//============================================================================
// LOGICAL KEYPAD/LED ABSTRACTION LAYER (Delegates to MatrixPanel)
//============================================================================

/**
 * Convert column (x) and row (y) coordinates to logical cell index
 * This provides a consistent logical view of the keypad/LED grid
 * @param x Column index (0-3)
 * @param y Row index (0-3)
 * @return Logical cell index (0-15), or 0xFF if out of bounds
 */
u8 Core::cellIndex(u8 x, u8 y) const
{
        return m_matrixPanel->cellIndex(x, y);
}

/**
 * Set LED color by logical index (hides physical wiring)
 * This function translates logical index to physical LED index
 * @param logicalIndex Logical cell index (0-15)
 * @param color 32-bit RGB color value (0x00RRGGBB)
 */
void Core::ledControl(u8 logicalIndex, u32 color)
{
        m_matrixPanel->ledControl(logicalIndex, color);
}

/**
 * Set LED color by logical index with separate RGB values
 * @param logicalIndex Logical cell index (0-15)
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 */
void Core::ledControl(u8 logicalIndex, u8 r, u8 g, u8 b)
{
        m_matrixPanel->ledControl(logicalIndex, r, g, b);
}

//============================================================================
// TYPE DETECTION MODE
//============================================================================

void Core::enterTypeDetectionMode()
{
        using namespace DetectionTiming;
        using namespace TypeLimits;

        Serial.println("\n=== ENTERING TYPE DETECTION MODE ===");
        Serial.println("Adjust trimmer pot to select device type (0-31)");
        Serial.println("Type changes will be logged automatically.");
        Serial.println("LED flashes once per reading (every 0.5 seconds).");
        Serial.println("Long press button again to exit.\n");

        // Save current mode to restore later
        m_previousMode = m_mode;
        m_mode = MODE_TYPE_DETECTION;

        // Save current device type - we'll restore this if calibration fails
        m_typeBeforeCalibration = m_deviceType;

        // Stop any running animation
        if (m_animation->isActive())
        {
                m_animation->stop();
        }

        // Clear pixels
        m_pixels->clear();
        //m_pixels->show();

        // Initialize type detection state
        m_lastTypeRead = millis() - READ_INTERVAL_MS + INITIAL_DELAY_MS; // Read after 500ms instead of immediate

        // Keep current device type - don't change it until we get a valid reading
        // Initialize last detected to INVALID to force first valid reading to be logged
        m_lastDetectedType = INVALID_TYPE;

        // Set status LED to detection mode
        m_previousStatusLedMode = m_statusLedMode;
        setStatusLed(STATUS_DEVICE_DETECTION);
}

void Core::exitTypeDetectionMode()
{
        using namespace TypeLimits;

        Serial.println("\n=== EXITING TYPE DETECTION MODE ===");

        // Check if current reading is valid before saving
        if (m_deviceType == INVALID_TYPE)
        {
                Serial.println("ERROR: Invalid device type detected!");
                Serial.println("Potentiometer may be disconnected.");
                Serial.println("Device type NOT CHANGED in NVS.");

                // Restore the type from before calibration started
                m_deviceType = m_typeBeforeCalibration;
                Serial.print("Restored previous type: ");
                Serial.print(getDeviceTypeName());
                Serial.print(" (");
                Serial.print(m_deviceType);
                Serial.println(")");
        }
        else
        {
                // Valid reading - save it
                Serial.print("Final Device Type: ");
                Serial.print(getDeviceTypeName());
                Serial.print(" (");
                Serial.print(m_deviceType);
                Serial.println(")");

                saveDeviceType(m_deviceType);
        }

        Serial.println();

        // Restore previous mode
        m_mode = m_previousMode;

        // Restore status LED to previous state
        setStatusLed(m_previousStatusLedMode);

        // Play exit sound
        m_synth->setWaveform(WAVE_SINE);
        m_synth->setADSR(5, 50, 80, 100);
        m_synth->playNote(NOTE_C5, 100, 150);
}

void Core::updateTypeDetectionMode()
{
        using namespace DetectionTiming;
        using namespace TypeLimits;

        u32 now = millis();

        // Read device type at specified interval
        if (now - m_lastTypeRead >= READ_INTERVAL_MS)
        {
                m_lastTypeRead = now;

                // Re-read device type from ADC (non-verbose - no auto-logging)
                u8 newType = readDeviceType(false);

                // Check if reading is valid
                if (newType == INVALID_TYPE)
                {
                        // Invalid/disconnected - warn but don't change stored type
                        if (m_lastDetectedType != INVALID_TYPE) // Only warn once
                        {
                                Serial.println("WARNING: Potentiometer disconnected or invalid reading!");
                                Serial.println("Reconnect potentiometer to continue calibration.");
                                m_lastDetectedType = INVALID_TYPE; // Mark as invalid state
                        }

                        // Flash LED in error pattern - set initial state, will be handled by non-blocking code
                        digitalWrite(STATUS_LED_PIN, HIGH);
                        m_typeDetectionBlink = true;

                        // Play error tone
                        m_synth->setWaveform(WAVE_SQUARE);
                        m_synth->setADSR(1, 10, 20, 20);
                        m_synth->playNote(NOTE_C4, 100, 100); // Lower pitch for error
                }
                else
                {
                        // Valid reading - update device type
                        m_deviceType = newType;

                        // Log on first valid reading or when type changes
                        if (m_lastDetectedType == INVALID_TYPE)
                        {
                                // First valid reading in this calibration session
                                Serial.print("Current type: ");
                                Serial.print(getDeviceTypeName());
                                Serial.print(" (");
                                Serial.print(m_deviceType);
                                Serial.println(")");
                                Serial.println();
                                DeviceConfigurations::printHardwareConfig(m_deviceType, "  ");
                                Serial.println();
                                m_lastDetectedType = m_deviceType;
                        }
                        else if (m_deviceType != m_lastDetectedType)
                        {
                                // Type changed from previous valid reading
                                Serial.print("Type changed: ");
                                Serial.print(DeviceConfigurations::getDeviceName(m_lastDetectedType > MAX_CURRENT_TYPE ? MAX_CURRENT_TYPE : m_lastDetectedType));
                                Serial.print(" (");
                                Serial.print(m_lastDetectedType);
                                Serial.print(") -> ");
                                Serial.print(getDeviceTypeName());
                                Serial.print(" (");
                                Serial.print(m_deviceType);
                                Serial.println(")");
                                Serial.println();
                                DeviceConfigurations::printHardwareConfig(m_deviceType, "  ");
                                Serial.println();
                                m_lastDetectedType = m_deviceType;
                        }
                        else
                        {
                                // Same as last reading - no log needed
                        }

                        // Play a short beep
                        m_synth->setWaveform(WAVE_SQUARE);
                        m_synth->setADSR(1, 10, 20, 20);
                        m_synth->playNote(NOTE_A4, 50, 100);
                }
        }
}

//============================================================================
// KEYPAD TEST MODE
//============================================================================

void Core::enterKeypadTestMode()
{
        Serial.println("\n╔════════════════════════════════════════════════════════════╗");
        Serial.println("║              ENTERING KEYPAD TEST MODE                     ║");
        Serial.println("╚════════════════════════════════════════════════════════════╝");
        Serial.println();
        Serial.println("Press any keypad button to toggle its corresponding LED.");
        Serial.println("Each key (0-15) controls one LED.");
        Serial.println("Long press Button 1 to exit test mode.\n");

        // Save current mode
        m_previousMode = m_mode;
        m_mode = MODE_KEYPAD_TEST;

        // Stop any running animation and clear LEDs
        m_animation->stop(true);

        // Turn off all LEDs
        // m_pixels->clear();
        //m_pixels->show();

        // Reset all keypad LED states
        for (int i = 0; i < 16; i++)
        {
                m_keypadLedStates[i] = false;
        }
}

void Core::exitKeypadTestMode()
{
        Serial.println("\n╔════════════════════════════════════════════════════════════╗");
        Serial.println("║              EXITING KEYPAD TEST MODE                      ║");
        Serial.println("╚════════════════════════════════════════════════════════════╝\n");

        // Clear all LEDs
        m_pixels->clear();
        //m_pixels->show();

        // Restore previous mode
        m_mode = m_previousMode;
}

void Core::handleKeypadTestPress(u8 keyIndex)
{
        // Ensure keyIndex is within bounds (0-15)
        if (keyIndex >= 16)
                return;

        // Toggle LED state for this logical index
        m_keypadLedStates[keyIndex] = !m_keypadLedStates[keyIndex];

        Serial.print("Key ");
        Serial.print(keyIndex);
        Serial.print(" -> LED ");
        Serial.print(keyIndex); // Now using logical index
        Serial.print(" ");
        Serial.println(m_keypadLedStates[keyIndex] ? "ON" : "OFF");

        // Update the LED using logical abstraction layer
        if (m_keypadLedStates[keyIndex])
        {
                // Turn on LED with red color
                ledControl(keyIndex, 255, 0, 0);
        }
        else
        {
                // Turn off LED
                ledControl(keyIndex, 0, 0, 0);
        }
        m_pixels->show();
}
