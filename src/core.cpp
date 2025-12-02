#include "core.h"
#include <Arduino.h>

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

// Device type names (0-63, currently using 0-31)
// These are placeholder names at the Core firmware level.
// High-level App code should implement its own type name mapping
// for application-specific device types.
// Types 32-63 reserved for future expansion
const char *Core::kDeviceTypeNames[64] = {
    // Currently used types (0-31)
    "TYPE_00", "TYPE_01", "TYPE_02", "TYPE_03",
    "TYPE_04", "TYPE_05", "TYPE_06", "TYPE_07",
    "TYPE_08", "TYPE_09", "TYPE_10", "TYPE_11",
    "TYPE_12", "TYPE_13", "TYPE_14", "TYPE_15",
    "TYPE_16", "TYPE_17", "TYPE_18", "TYPE_19",
    "TYPE_20", "TYPE_21", "TYPE_22", "TYPE_23",
    "TYPE_24", "TYPE_25", "TYPE_26", "TYPE_27",
    "TYPE_28", "TYPE_29", "TYPE_30", "TYPE_31",

    // Reserved for future expansion (32-63)
    "TYPE_32", "TYPE_33", "TYPE_34", "TYPE_35",
    "TYPE_36", "TYPE_37", "TYPE_38", "TYPE_39",
    "TYPE_40", "TYPE_41", "TYPE_42", "TYPE_43",
    "TYPE_44", "TYPE_45", "TYPE_46", "TYPE_47",
    "TYPE_48", "TYPE_49", "TYPE_50", "TYPE_51",
    "TYPE_52", "TYPE_53", "TYPE_54", "TYPE_55",
    "TYPE_56", "TYPE_57", "TYPE_58", "TYPE_59",
    "TYPE_60", "TYPE_61", "TYPE_62", "TYPE_63"};

//============================================================================
// CONSTRUCTOR
//============================================================================

Core::Core(PixelStrip *pixels, Synth *synth, Animation *animation,
           InputManager *inputManager, RoomSerial *roomBus)
    : m_pixels(pixels),
      m_synth(synth),
      m_animation(animation),
      m_inputManager(inputManager),
      m_roomBus(roomBus),
      m_mode(MODE_INTERACTIVE),
      m_colorIndex(0),
      m_deviceType(0),
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
}

//============================================================================
// PUBLIC METHODS
//============================================================================

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

//============================================================================
// CONFIGURATION
//============================================================================

u8 Core::readDeviceType(bool verbose)
{
        // First, check for disconnected pot using pull-down
        pinMode(CONFIG_ADC_PIN, INPUT_PULLDOWN);
        delay(10); // Allow pull-down to stabilize

        int checkReading = analogRead(CONFIG_ADC_PIN);

        // If reading is very low, pot is likely disconnected
        // Threshold set to 30 (well below TYPE_00 range of 0-63)
        if (checkReading < 30)
        {
                if (verbose)
                {
                        Serial.print("Config ADC: ");
                        Serial.print(checkReading);
                        Serial.println(" (range: -) -> DISCONNECTED/INVALID");
                }
                return 0xFF; // Disconnected
        }

        // Pot appears connected - switch to normal INPUT mode for accurate reading
        // This prevents pull-down from affecting the voltage divider
        pinMode(CONFIG_ADC_PIN, INPUT);
        delay(10); // Allow pin to stabilize without pull-down

        // Take multiple readings and average for stability
        const int numSamples = 32; // More samples for 5-bit precision
        int sum = 0;
        int minReading = 4095;
        int maxReading = 0;

        for (int i = 0; i < numSamples; i++)
        {
                int reading = analogRead(CONFIG_ADC_PIN);
                sum += reading;

                if (reading < minReading)
                        minReading = reading;
                if (reading > maxReading)
                        maxReading = reading;

                delayMicroseconds(100);
        }

        int adcValue = sum / numSamples;
        int adcRange = maxReading - minReading;

        // Check for excessive noise (bad connection)
        bool isNoisy = (adcRange > 200);

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
                        return 0xFF; // Invalid reading
                }
        }

        // If noisy in non-verbose mode, still return invalid
        if (isNoisy)
        {
                return 0xFF;
        }

        // Convert ADC value to device type (0-31 for 5-bit, expandable to 0-63 for 6-bit)
        // Full ADC range: 0-4095
        // Using step size of 64 (instead of 128) allows future expansion to 64 types
        // Current implementation: Uses types 0-31 (lower half)
        // Future expansion: Can use types 32-63 (upper half) without firmware changes
        u8 deviceType = adcValue / 64; // 4096 / 64 = 64 possible types

        // Currently clamp to 0-31 range (5-bit addressing)
        // Remove this clamp in future to enable full 0-63 range (6-bit addressing)
        if (deviceType > 31)
                deviceType = 31;

        if (verbose)
        {
                Serial.print(" -> Type ");
                Serial.print(deviceType);
                Serial.print(" (");
                Serial.print(kDeviceTypeNames[deviceType]);
                Serial.println(")");
        }

        return deviceType;
}

void Core::printBootReport()
{
        Serial.println("\n╔════════════════════════════════════════════════════════════╗");
        Serial.println("║           ESCAPE ROOM CLIENT - BOOT REPORT                ║");
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
        }

        // Status LED
        Serial.print("│ Status LED:        ");
        switch (m_statusLedMode)
        {
        case STATUS_OK:
                Serial.println("OK (solid ON)");
                break;
        case STATUS_I2C_ERROR:
                Serial.println("I2C ERROR (fast blink)");
                break;
        case STATUS_TYPE_ERROR:
                Serial.println("TYPE ERROR (slow blink)");
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
        Serial.println("ℹ️  To reconfigure device type: Long-press boot button");
        Serial.println("ℹ️  Device ready for operation");
        Serial.println();
}

void Core::saveDeviceType(u8 type)
{
        // Clamp to valid range
        if (type > 31)
                type = 31;

        m_preferences.putUChar("deviceType", type);
        Serial.print("Device type ");
        Serial.print(type);
        Serial.println(" saved to NVS.");
}

u8 Core::loadDeviceType()
{
        // Returns 0xFF if key doesn't exist (first boot or after factory reset)
        return m_preferences.getUChar("deviceType", 0xFF);
}

void Core::clearStoredDeviceType()
{
        m_preferences.remove("deviceType");
        Serial.println("Stored device type cleared (factory reset).");
        Serial.println("Reboot to read device type from ADC.");
}

const char *Core::getDeviceTypeName() const
{
        // Ensure device type is in valid range
        if (m_deviceType > 31)
                return "INVALID";

        return kDeviceTypeNames[m_deviceType];
}

//============================================================================
// EVENT HANDLING
//============================================================================

void Core::setMode(CoreMode mode)
{
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
                handleKeypadPress(m_inputManager->getKeypadNote(event));
                break;

        case INPUT_SWITCH1_ON:
                Serial.println("Switch 1: ON");
                break;

        case INPUT_SWITCH1_OFF:
                Serial.println("Switch 1: OFF");
                break;

        case INPUT_SWITCH2_ON:
                Serial.println("Switch 2: ON");
                break;

        case INPUT_SWITCH2_OFF:
                Serial.println("Switch 2: OFF");
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
        Serial.println("Color change: cycle colors");

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
        // Toggle type detection mode
        if (m_mode == MODE_TYPE_DETECTION)
        {
                exitTypeDetectionMode();
        }
        else
        {
                enterTypeDetectionMode();
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
        if (frame.cmd_srv == CMD_GLOW_SET_COLOR)
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
        m_statusLedMode = mode;
        m_lastLedToggle = millis();

        // Set initial state based on mode
        if (mode == STATUS_OK)
        {
                m_ledState = true;
                digitalWrite(STATUS_LED_PIN, HIGH); // Solid ON for OK
        }
        else
        {
                m_ledState = false;
                digitalWrite(STATUS_LED_PIN, LOW); // Start with OFF for blink modes
        }
}

void Core::updateStatusLed()
{
        // Don't update status LED in type detection mode
        if (m_mode == MODE_TYPE_DETECTION)
        {
                return;
        }

        u32 now = millis();
        u32 interval = 0;

        switch (m_statusLedMode)
        {
        case STATUS_OK:
                // Solid ON - no blinking needed
                if (!m_ledState)
                {
                        m_ledState = true;
                        digitalWrite(STATUS_LED_PIN, HIGH);
                }
                return;

        case STATUS_I2C_ERROR:
                // Fast blink - 5 Hz (200ms period, 100ms ON/OFF)
                interval = 100;
                break;

        case STATUS_TYPE_ERROR:
                // Slow blink - 1 Hz (1000ms period, 500ms ON/OFF)
                interval = 500;
                break;
        }

        // Toggle LED at specified interval
        if (now - m_lastLedToggle >= interval)
        {
                m_lastLedToggle = now;
                m_ledState = !m_ledState;
                digitalWrite(STATUS_LED_PIN, m_ledState ? HIGH : LOW);
        }
}

//============================================================================
// TYPE DETECTION MODE
//============================================================================

void Core::enterTypeDetectionMode()
{
        Serial.println("\n=== ENTERING TYPE DETECTION MODE ===");
        Serial.println("Adjust trimmer pot to select device type (0-31)");
        Serial.println("Type changes will be logged automatically.");
        Serial.println("LED flashes once per reading (every 1 second).");
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
        m_pixels->show();

        // Initialize type detection state
        m_lastTypeRead = millis() - 500; // Start halfway to first read (read after 500ms instead of immediate)
        m_typeDetectionBlink = false;

        // Keep current device type - don't change it until we get a valid reading
        // Initialize last detected to 0xFF to force first valid reading to be logged
        m_lastDetectedType = 0xFF;

        // Turn off status LED (will be used for reading flash only)
        digitalWrite(STATUS_LED_PIN, LOW);
}

void Core::exitTypeDetectionMode()
{
        Serial.println("\n=== EXITING TYPE DETECTION MODE ===");

        // Check if current reading is valid before saving
        if (m_deviceType == 0xFF)
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
        if (m_statusLedMode == STATUS_OK)
        {
                digitalWrite(STATUS_LED_PIN, HIGH);
        }

        // Play exit sound
        m_synth->setWaveform(WAVE_SINE);
        m_synth->setADSR(5, 50, 80, 100);
        m_synth->playNote(NOTE_C5, 100, 150);
}

void Core::updateTypeDetectionMode()
{
        u32 now = millis();

        // Read device type every 500ms (0.5 seconds)
        if (now - m_lastTypeRead >= 500)
        {
                m_lastTypeRead = now;

                // Re-read device type from ADC (non-verbose - no auto-logging)
                u8 newType = readDeviceType(false);

                // Check if reading is valid
                if (newType == 0xFF)
                {
                        // Invalid/disconnected - warn but don't change stored type
                        if (m_lastDetectedType != 0xFF) // Only warn once
                        {
                                Serial.println("WARNING: Potentiometer disconnected or invalid reading!");
                                Serial.println("Reconnect potentiometer to continue calibration.");
                                m_lastDetectedType = 0xFF; // Mark as invalid state
                        }

                        // Flash LED in error pattern (fast double blink)
                        digitalWrite(STATUS_LED_PIN, HIGH);
                        delay(50);
                        digitalWrite(STATUS_LED_PIN, LOW);
                        delay(50);
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
                        if (m_lastDetectedType == 0xFF)
                        {
                                // First valid reading in this calibration session
                                Serial.print("Current type: ");
                                Serial.print(getDeviceTypeName());
                                Serial.print(" (");
                                Serial.print(m_deviceType);
                                Serial.println(")");
                                m_lastDetectedType = m_deviceType;
                        }
                        else if (m_deviceType != m_lastDetectedType)
                        {
                                // Type changed from previous valid reading
                                Serial.print("Type changed: ");
                                Serial.print(kDeviceTypeNames[m_lastDetectedType > 31 ? 31 : m_lastDetectedType]);
                                Serial.print(" (");
                                Serial.print(m_lastDetectedType);
                                Serial.print(") -> ");
                                Serial.print(getDeviceTypeName());
                                Serial.print(" (");
                                Serial.print(m_deviceType);
                                Serial.println(")");
                                m_lastDetectedType = m_deviceType;
                        }
                        else
                        {
                                // Same as last reading - no log needed
                        }

                        // Flash LED once (100ms pulse)
                        digitalWrite(STATUS_LED_PIN, HIGH);
                        m_typeDetectionBlink = true;

                        // Play a short beep
                        m_synth->setWaveform(WAVE_SQUARE);
                        m_synth->setADSR(1, 10, 20, 20);
                        m_synth->playNote(NOTE_A4, 50, 100);
                }
        }

        // Turn off LED after 100ms flash
        if (m_typeDetectionBlink && (now - m_lastTypeRead >= 100))
        {
                digitalWrite(STATUS_LED_PIN, LOW);
                m_typeDetectionBlink = false;
        }
}
