/************************* core.cpp ****************************
 * Core Firmware Implementation
 * Handles device type configuration, mode management, and hardware control
 * Created by MSK, November 2025
 * Architecture: Core (firmware) + App (high-level) separation
 ***************************************************************/

#include "core.h"
#include "deviceconfig.h"
#include "app_base.h"
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

// Instruction: add new device types in ALL_DEVICES (deviceconfig.cpp) and DeviceType enum (deviceconfig.h);
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

/************************* Core ***********************************
 * Constructor for the Core system.
 * @param pixels Pointer to PixelStrip driver.
 * @param synth Pointer to Synth driver.
 * @param animation Pointer to Animation driver.
 * @param inputManager Pointer to InputManager driver.
 * @param roomBus Pointer to RoomSerial driver.
 * @param ioExpander Pointer to IOExpander driver.
 ***************************************************************/
Core::Core(PixelStrip *pixels, Synth *synth, Animation *animation,
           InputManager *inputManager, RoomSerial *roomBus, IOExpander *ioExpander)
    : m_pixels(pixels),
      m_synth(synth),
      m_animation(animation),
      m_inputManager(inputManager),
      m_roomBus(roomBus),
      m_ioExpander(ioExpander),
      m_matrixPanel(new MatrixPanel(pixels)), // Initialize matrix panel
      m_app(nullptr),
      m_mode(MODE_INTERACTIVE),
      m_colorIndex(0),
      m_address(0),
      m_type(TERMINAL),
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

/************************* begin ***********************************
 * System-wide initialization - sets up all hardware and firmware modules.
 * Initializes Serial, I2C, Pixels, Buttons, Timer, Watchdog, Synth, RoomBus.
 ***************************************************************/
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

/************************* init ***********************************
 * Initializes core firmware logic and state.
 * Sets initial mode, status LED, and loads configuration.
 ***************************************************************/
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

        // 1. Load Device Type (Factory Config)
        u8 typeVal = loadDeviceType();
        if (typeVal == 0xFF)
        {
                // First boot - read from ADC
                typeVal = readDeviceType();
                if (typeVal == 0xFF)
                {
                        Serial.println("⚠️  WARNING: Cannot read device type from ADC!");
                        typeVal = 0; // Default
                        m_statusLedMode = STATUS_TYPE_ERROR;
                }
                else
                {
                        saveDeviceType(typeVal);
                }
        }
        m_type = (DeviceType)typeVal;

        // 2. Load Device Address (Room Setup)
        m_address = loadAddress();
        if (m_address == 0xFF)
        {
                m_address = ADDR_UNASSIGNED; // 0x00
        }

        // Initialize Application
        m_app = AppBase::create(m_type);
        if (m_app)
        {
                AppContext context = {
                    m_pixels,
                    m_synth,
                    m_animation,
                    m_inputManager,
                    m_roomBus,
                    m_ioExpander,
                    m_matrixPanel,
                    &m_address,
                    &m_type};
                m_app->setup(context);
        }

        // Send HELLO to server
        sendHello();

        // Set up input callback
        m_inputManager->setCallback(onInputEvent);
}

/************************* update ***********************************
 * Main loop update function.
 * Polls inputs, updates status LED, handles RoomBus commands, and updates the active App.
 ***************************************************************/
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

        // Update Application
        if (m_app && m_mode == MODE_INTERACTIVE)
        {
                m_app->loop();
        }
}

/************************* refreshAnimations ***********************************
 * Updates the animation system.
 * @param flag Reference to the pixel update flag.
 ***************************************************************/
void Core::refreshAnimations(volatile bool &flag)
{
        m_animation->refresh(flag);
}

//============================================================================
// CONFIGURATION
//============================================================================

/************************* readDeviceType ***********************************
 * Reads the device type from the configuration potentiometer.
 * Uses averaging and noise detection for reliability.
 * @param verbose Whether to print debug info to Serial.
 * @return Detected DeviceType or INVALID_TYPE.
 ***************************************************************/
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
                Serial.print(DeviceConfigurations::getName((DeviceType)deviceType));
                Serial.println(")");
        }

        return deviceType;
}

/************************* printBootReport ***********************************
 * Prints a detailed boot report to Serial.
 * Includes device config, hardware status, and operating mode.
 ***************************************************************/
void Core::printBootReport()
{
        Serial.println("\n╔════════════════════════════════════════════════════════════╗");
        Serial.println("║            ESCAPE ROOM CLIENT - BOOT REPORT                ║");
        Serial.println("╚════════════════════════════════════════════════════════════╝");
        Serial.println();

        // Device identification
        Serial.println("┌─ DEVICE CONFIGURATION ─────────────────────────────────────┐");
        Serial.print("│ Device Address:    ");
        Serial.print(m_address);
        Serial.print(" (Type: ");
        Serial.print(getDeviceTypeName());
        Serial.println(")");

        // Storage status
        Serial.print("│ Configuration:     ");
        u8 storedAddr = m_preferences.getUChar("address", 0xFF);
        if (storedAddr == 0xFF)
        {
                Serial.println("Not saved (temporary)");
        }
        else
        {
                Serial.print("Stored in NVS");
                if (storedAddr != m_address)
                {
                        Serial.print(" (warning: mismatch!)");
                }
                Serial.println();
        }

        // Device hardware configuration
        Serial.println("│");
        Serial.println("│ Hardware Config:");
        DeviceConfigurations::printConfig(m_type);

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

/************************* saveAddress ***********************************
 * Saves the device address to NVS.
 * @param address The address to save.
 ***************************************************************/
void Core::saveAddress(u8 address)
{
        using namespace TypeLimits;

        // Validate and clamp to valid range
        if (address > MAX_CURRENT_TYPE && address != INVALID_TYPE)
        {
                Serial.print("WARNING: Device address ");
                Serial.print(address);
                Serial.print(" out of range. Clamping to ");
                Serial.println(MAX_CURRENT_TYPE);
                address = MAX_CURRENT_TYPE;
        }

        // Don't save invalid address
        if (address == INVALID_TYPE)
        {
                Serial.println("ERROR: Cannot save invalid device address to NVS.");
                return;
        }

        // Save to NVS
        size_t bytesWritten = m_preferences.putUChar("address", address);
        if (bytesWritten == 0)
        {
                Serial.println("ERROR: Failed to save device address to NVS!");
        }
        else
        {
                Serial.print("Device address ");
                Serial.print(address);
                Serial.println(" saved to NVS.");
        }
}

/************************* loadAddress ***********************************
 * Loads the device address from NVS.
 * @return The loaded address or INVALID_TYPE if not found.
 ***************************************************************/
u8 Core::loadAddress()
{
        using namespace TypeLimits;

        // Returns INVALID_TYPE (0xFF) if key doesn't exist (first boot or after factory reset)
        u8 address = m_preferences.getUChar("address", INVALID_TYPE);

        // Validate loaded address
        if (address != INVALID_TYPE && address > MAX_CURRENT_TYPE)
        {
                Serial.print("WARNING: Loaded invalid device address ");
                Serial.print(address);
                Serial.println(" from NVS. Treating as unconfigured.");
                return INVALID_TYPE;
        }

        return address;
}

/************************* saveDeviceType ***********************************
 * Saves the device type to NVS.
 * @param type The device type to save.
 ***************************************************************/
void Core::saveDeviceType(u8 type)
{
        using namespace TypeLimits;
        if (type > MAX_CURRENT_TYPE && type != INVALID_TYPE)
                type = MAX_CURRENT_TYPE;
        if (type == INVALID_TYPE)
                return;

        size_t bytesWritten = m_preferences.putUChar("deviceType", type);
        if (bytesWritten > 0)
        {
                Serial.print("Device type ");
                Serial.print(type);
                Serial.println(" saved to NVS.");
        }
}

/************************* loadDeviceType ***********************************
 * Loads the device type from NVS.
 * @return The loaded device type or INVALID_TYPE if not found.
 ***************************************************************/
u8 Core::loadDeviceType()
{
        using namespace TypeLimits;
        u8 type = m_preferences.getUChar("deviceType", INVALID_TYPE);
        if (type != INVALID_TYPE && type > MAX_CURRENT_TYPE)
                return INVALID_TYPE;
        return type;
}

/************************* clearStoredConfig ***********************************
 * Clears all stored configuration from NVS (Factory Reset).
 ***************************************************************/
void Core::clearStoredConfig()
{
        m_preferences.remove("address");
        m_preferences.remove("deviceType");
        /************************* getDeviceTypeName ***********************************
         * Gets the string name of the current device type.
         * @return C-string name of the device type.
         ***************************************************************/
        Serial.println("Stored config cleared (factory reset).");
}

const char *Core::getDeviceTypeName() const
{
        return DeviceConfigurations::getName(m_type);
}

/************************* setMode ***********************************
 * Sets the operating mode of the Core.
 * Handles mode transitions and logging.
 * @param mode The new CoreMode to set.
 ***************************************************************/
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

/************************* onInputEvent ***********************************
 * Static callback wrapper for input events.
 * Forwards the event to the singleton instance.
 * @param event The input event ID.
 ***************************************************************/
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

/************************* handleInputEvent ***********************************
 * Dispatches input events to the appropriate handler based on mode.
 * Handles system overrides (long press), test modes, and app input.
 * @param event The input event ID.
 ***************************************************************/
void Core::handleInputEvent(InputEvent event)
{
        // System-wide overrides
        if (event == INPUT_BTN1_LONG_PRESS)
        {
                handleButtonLongPress();
                return;
        }

        // Mode-specific handling
        if (m_mode == MODE_KEYPAD_TEST)
        {
                if (event >= INPUT_KEYPAD_0 && event <= INPUT_KEYPAD_15)
                {
                        handleKeypadTestPress(m_inputManager->getKeypadNote(event));
                }
                return;
        }

        // Application handling (Normal Operation)
        if (m_app && m_mode == MODE_INTERACTIVE)
        {
                if (m_app->handleInput(event))
                {
                        return; // App consumed the event
                }
                // Fall through to legacy handling if app didn't consume it
        }

        // Legacy/Fallback handling (if no app or not interactive)
        switch (event)
        {
        case INPUT_BTN1_PRESS:
                handleButton1Press();
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
                handleKeypadPress(m_inputManager->getKeypadNote(event));
                break;

        default:
                break;
        }
}
/************************* handleButton1Press ***********************************
 * Handles short press of Button 1.
 * Triggers pixel check on first press, then cycles colors/modes.
 ***************************************************************/

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

/************************* handleButtonLongPress ***********************************
 * Handles long press of Button 1.
 * Cycles through special modes: Normal -> Keypad Test -> Type Detection.
 ***************************************************************/
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

/************************* handleKeypadPress ***********************************
 * Handles keypad button press events.
 * Plays a note corresponding to the key index.
 * @param keyIndex The index of the pressed key (0-15).
 ***************************************************************/
void Core::handleKeypadPress(u8 keyIndex)
{
        int note = kNoteMap[keyIndex];

        /************************* handleRoomBusFrame ***********************************
         * Handles incoming RoomBus frames.
         * Filters by address and dispatches commands to Core or App.
         * @param frame The received RoomFrame.
         ***************************************************************/
        m_synth->setWaveform(WAVE_SINE);
        m_synth->setADSR(5, 50, 100, 100);
        m_synth->playNote(note, 150, 100);
}

void Core::handleRoomBusFrame(const RoomFrame &frame)
{
        // Filter by address
        // Accept if:
        // 1. Broadcast
        // 2. Addressed to me
        // 3. I am unassigned (0x00) and message is for 0x00 (e.g. SET_ADDRESS)
        bool isForMe = (frame.addr == m_address) || (frame.addr == ADDR_BROADCAST);

        // Special case: If I am unassigned, I might accept assignment commands
        if (m_address == ADDR_UNASSIGNED && frame.addr == ADDR_UNASSIGNED)
        {
                isForMe = true;
        }

        if (!isForMe)
                return;

        Serial.print("Room Bus frame received! Addr: 0x");
        Serial.print(frame.addr, HEX);
        Serial.print(" Cmd_srv: 0x");
        Serial.print(frame.cmd_srv, HEX);
        Serial.print(" Cmd_dev: 0x");
        Serial.println(frame.cmd_dev, HEX);

        // 1. Handle Common Commands (Core Level)
        switch (frame.cmd_srv)
        {
        case CORE_PING:
                // Respond with PONG (or just ACK)
                // TODO: Send ACK
                Serial.println("-> PING received");
                return; // Handled

        case CORE_RESET:
                Serial.println("-> RESET received. Rebooting...");
                delay(100);
                ESP.restart();
                return; // Handled

        case CORE_HELLO:
                // Server saying hello? Usually device says hello.
                return;

        case CORE_SET_ADDRESS:
                // Payload[0] = New Address
                if (frame.p[0] != 0 && frame.p[0] != 0xFF)
                {
                        Serial.print("-> SET_ADDRESS received: ");
                        Serial.println(frame.p[0]);
                        m_address = frame.p[0];
                        saveAddress(m_address);
                        sendHello(); // Announce new address
                }
                return;
        }

        // 2. Pass to Application (Device Specific)
        if (m_app)
        {
                m_app->handleCommand(frame);
        }
}

/************************* sendHello ***********************************
 * Sends a HELLO message to the server.
 * Identifies the device by address and type.
 * If unassigned, includes MAC address for identification.
 ***************************************************************/
void Core::sendHello()
{
        if (m_roomBus)
        {
                RoomFrame frame;
                // If unassigned, send from 0x00. If assigned, send from m_address.
                // But room_frame_init_device sets addr to ADDR_SERVER (destination).
                // The source address is not in the frame structure?
                // Wait, RoomFrame struct has `addr` which is DESTINATION.
                // RS-485 usually doesn't have Source Address in the frame unless payload.
                // But the Server needs to know who sent it.
                // Ah, `roombus.h` says:
                // typedef struct { u8 addr; ... } RoomFrame;
                // "addr: destination address"

                // If the protocol doesn't have Source Address in the header,
                // the device must put it in the payload or the server infers it?
                // Usually RS-485 is Master-Slave polling.
                // But here we have "Device -> Server events".
                // If multiple devices talk, how does Server know who?
                // The `RoomFrame` struct in `roombus.h` only has one `addr` field.
                // If it's a message TO Server, `addr` is ADDR_SERVER (0x01).
                // So the Server doesn't know the source unless it's in the payload.

                // Let's put the Device Address in p[0] and Type in p[1] for HELLO?
                // Or maybe the protocol assumes the Server polls devices?
                // "EV_GLOW_PRESSED = 0x80" -> Device sends this.

                // For HELLO, we definitely need to identify ourselves.

                room_frame_init_device(&frame, CORE_HELLO);

                // Payload:
                // p[0] = My Address (so server knows who I am)
                // p[1] = My Device Type

                frame.p[0] = m_address;
                frame.p[1] = (u8)m_type;

                // If unassigned, maybe add a random ID or MAC to p[2]..p[7]?
                if (m_address == ADDR_UNASSIGNED)
                {
                        uint8_t mac[6];
                        esp_read_mac(mac, ESP_MAC_WIFI_STA);
                        for (int i = 0; i < 6; i++)
                                frame.p[2 + i] = mac[i];
                }

                m_roomBus->sendFrame(&frame);
                Serial.println("Sent HELLO to server.");
        }
}

//============================================================================
// STATUS LED CONTROL
//============================================================================

/************************* setStatusLed ***********************************
 * Sets the status LED blinking pattern.
 * @param mode The StatusLedMode to set.
 ***************************************************************/
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

/************************* updateStatusLed ***********************************
 * Updates the status LED state based on the current pattern.
 * Called from the main loop.
 ***************************************************************/
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

/************************* enterTypeDetectionMode ***********************************
 * Enters the device type detection mode.
 * Allows the user to select the device type using the potentiometer.
 ***************************************************************/
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
        m_typeBeforeCalibration = (u8)m_type;

        // Stop any running animation
        if (m_animation->isActive())
        {
                m_animation->stop();
        }

        // Clear pixels
        m_pixels->clear();
        // m_pixels->show();

        // Initialize type detection state
        m_lastTypeRead = millis() - READ_INTERVAL_MS + INITIAL_DELAY_MS; // Read after 500ms instead of immediate

        // Keep current device type - don't change it until we get a valid reading
        // Initialize last detected to INVALID to force first valid reading to be logged
        m_lastDetectedType = INVALID_TYPE;

        // Set status LED to detection mode
        m_previousStatusLedMode = m_statusLedMode;
        setStatusLed(STATUS_DEVICE_DETECTION);
}

/************************* exitTypeDetectionMode ***********************************
 * Exits the device type detection mode.
 * Saves the selected device type to NVS if valid.
 * Restores the previous operating mode.
 ***************************************************************/
void Core::exitTypeDetectionMode()
{
        using namespace TypeLimits;

        Serial.println("\n=== EXITING TYPE DETECTION MODE ===");

        // Check if current reading is valid before saving
        if (m_type == (DeviceType)INVALID_TYPE)
        {
                Serial.println("ERROR: Invalid device type detected!");
                Serial.println("Potentiometer may be disconnected.");
                Serial.println("Device type NOT CHANGED in NVS.");

                // Restore the type from before calibration started
                m_type = (DeviceType)m_typeBeforeCalibration;
                Serial.print("Restored previous type: ");
                Serial.print(getDeviceTypeName());
                Serial.print(" (");
                Serial.print(m_type);
                Serial.println(")");
        }
        else
        {
                // Valid reading - save it
                Serial.print("Final Device Type: ");
                Serial.print(getDeviceTypeName());
                Serial.print(" (");
                Serial.print(m_type);
                Serial.println(")");

                saveDeviceType((u8)m_type);
        }

        Serial.println();

        // Re-initialize the application with the new type
        if (m_app)
        {
                delete m_app;
                m_app = nullptr;
        }

        Serial.print("Initializing App for type: ");
        Serial.println(getDeviceTypeName());

        m_app = AppBase::create(m_type);
        if (m_app)
        {
                AppContext context = {
                    m_pixels,
                    m_synth,
                    m_animation,
                    m_inputManager,
                    m_roomBus,
                    m_ioExpander,
                    m_matrixPanel,
                    &m_address,
                    &m_type};
                m_app->setup(context);
        }

        // Restore previous mode
        m_mode = m_previousMode;

        // Restore status LED to previous state
        setStatusLed(m_previousStatusLedMode);

        // Play exit sound
        m_synth->setWaveform(WAVE_SINE);
        m_synth->setADSR(5, 50, 80, 100);
        m_synth->playNote(NOTE_C5, 100, 150);
}

/************************* updateTypeDetectionMode ***********************************
 * Updates the device type detection logic.
 * Reads the potentiometer and updates the current device type.
 ***************************************************************/
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
                        m_type = (DeviceType)newType;

                        // Log on first valid reading or when type changes
                        if (m_lastDetectedType == INVALID_TYPE)
                        {
                                // First valid reading in this calibration session
                                Serial.print("Current type: ");
                                Serial.print(getDeviceTypeName());
                                Serial.print(" (");
                                Serial.print(m_type);
                                Serial.println(")");
                                Serial.println();
                                DeviceConfigurations::printConfig(m_type);
                                Serial.println();
                                m_lastDetectedType = (u8)m_type;
                        }
                        else if ((u8)m_type != m_lastDetectedType)
                        {
                                // Type changed from previous valid reading
                                Serial.print("Type changed: ");
                                Serial.print(DeviceConfigurations::getName((DeviceType)(m_lastDetectedType > MAX_CURRENT_TYPE ? MAX_CURRENT_TYPE : m_lastDetectedType)));
                                Serial.print(" (");
                                Serial.print(m_lastDetectedType);
                                Serial.print(") -> ");
                                Serial.print(getDeviceTypeName());
                                Serial.print(" (");
                                Serial.print(m_type);
                                Serial.println(")");
                                Serial.println();
                                DeviceConfigurations::printConfig(m_type);
                                Serial.println();
                                m_lastDetectedType = (u8)m_type;
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

/************************* enterKeypadTestMode ***********************************
 * Enters the keypad test mode.
 * Allows testing of keypad buttons and LEDs.
 ***************************************************************/
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

        // Reset all keypad LED states
        for (int i = 0; i < 16; i++)
        {
                m_keypadLedStates[i] = false;
        }
}

/************************* exitKeypadTestMode ***********************************
 * Exits the keypad test mode.
 * Restores the previous operating mode.
 ***************************************************************/
void Core::exitKeypadTestMode()
{
        Serial.println("\n╔════════════════════════════════════════════════════════════╗");
        Serial.println("║              EXITING KEYPAD TEST MODE                      ║");
        Serial.println("╚════════════════════════════════════════════════════════════╝\n");

        // Clear all LEDs
        m_pixels->clear();
        // m_pixels->show();

        /************************* handleKeypadTestPress ***********************************
         * Handles keypad presses in test mode.
         * Toggles the LED corresponding to the pressed key.
         * @param keyIndex The index of the pressed key (0-15).
         ***************************************************************/
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
