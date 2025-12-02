#include "app.h"
#include <Arduino.h>

//============================================================================
// STATIC DATA
//============================================================================

// Static instance pointer for callback
Application *Application::s_instance = nullptr;

// Color palette for cycling
const u32 Application::kColors[] = {CLR_WT, CLR_PN, CLR_PR, CLR_MG, CLR_OR,
                                    CLR_YL, CLR_RD, CLR_BL, CLR_CY, CLR_GR};
const size_t Application::kColorCount = sizeof(kColors) / sizeof(kColors[0]);

// Note map for keypad (16 keys -> 16 notes)
const int Application::kNoteMap[16] = {
    NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4,
    NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5,
    NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5,
    NOTE_A5, NOTE_B5, NOTE_C6, NOTE_D6};

//============================================================================
// CONSTRUCTOR
//============================================================================

Application::Application(PixelStrip *pixels, Synth *synth, Animation *animation,
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
      m_lastDetectedType(0xFF) // Invalid initial value to force first log
{
        s_instance = this;
}

//============================================================================
// PUBLIC METHODS
//============================================================================

void Application::init()
{
        m_mode = MODE_INTERACTIVE;
        m_colorIndex = 0;

        // Initialize status LED pin
        pinMode(STATUS_LED_PIN, OUTPUT);
        digitalWrite(STATUS_LED_PIN, LOW);
        m_statusLedMode = STATUS_OK;
        m_lastLedToggle = 0;
        m_ledState = false;

        // Read device type from trimmer pot (one-time read at startup)
        m_deviceType = readDeviceType();
        Serial.print("Device Type: ");
        Serial.println(m_deviceType);

        // Set up input callback
        m_inputManager->setCallback(onInputEvent);

        Serial.println("Application initialized");
}

void Application::update()
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

u8 Application::readDeviceType(bool verbose)
{
        // Configure ADC pin
        pinMode(CONFIG_ADC_PIN, INPUT);

        // Take multiple readings and average for stability
        const int numSamples = 32; // More samples for 5-bit precision
        int sum = 0;

        for (int i = 0; i < numSamples; i++)
        {
                sum += analogRead(CONFIG_ADC_PIN);
                delayMicroseconds(100);
        }

        int adcValue = sum / numSamples;

        // Convert ADC value (0-4095) to device type (0-31 for 5-bit)
        // Each step is ~128 ADC units (4096 / 32)
        u8 deviceType = adcValue / 128;

        // Clamp to 0-31 range
        if (deviceType > 31)
                deviceType = 31;

        // Only log if verbose mode is enabled
        if (verbose)
        {
                Serial.print("Config ADC: ");
                Serial.print(adcValue);
                Serial.print(" -> Type ");
                Serial.println(deviceType);
        }

        return deviceType;
}

//============================================================================
// EVENT HANDLING
//============================================================================

void Application::setMode(AppMode mode)
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
void Application::onInputEvent(InputEvent event)
{
        if (s_instance)
        {
                s_instance->handleInputEvent(event);
        }
}

//============================================================================
// INPUT EVENT DISPATCHER
//============================================================================

void Application::handleInputEvent(InputEvent event)
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

void Application::handleButton1Press()
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

void Application::handleButtonLongPress()
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

void Application::handleKeypadPress(u8 keyIndex)
{
        int note = kNoteMap[keyIndex];

        m_synth->setWaveform(WAVE_SINE);
        m_synth->setADSR(5, 50, 100, 100);
        m_synth->playNote(note, 150, 100);
}

void Application::handleRoomBusFrame(const RoomFrame &frame)
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

void Application::setStatusLed(StatusLedMode mode)
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

void Application::updateStatusLed()
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

void Application::enterTypeDetectionMode()
{
        Serial.println("\n=== ENTERING TYPE DETECTION MODE ===");
        Serial.println("Adjust trimmer pot to select device type (0-31)");
        Serial.println("Type changes will be logged automatically.");
        Serial.println("LED flashes once per reading (every 1 second).");
        Serial.println("Long press button again to exit.\n");

        // Save current mode to restore later
        m_previousMode = m_mode;
        m_mode = MODE_TYPE_DETECTION;

        // Stop any running animation
        if (m_animation->isActive())
        {
                m_animation->stop();
        }

        // Clear pixels
        m_pixels->clear();
        m_pixels->show();

        // Initialize type detection state
        m_lastTypeRead = 0;
        m_typeDetectionBlink = false;
        m_lastDetectedType = 0xFF; // Force first reading to be logged

        // Turn off status LED (will be used for reading flash only)
        digitalWrite(STATUS_LED_PIN, LOW);
}

void Application::exitTypeDetectionMode()
{
        Serial.println("\n=== EXITING TYPE DETECTION MODE ===");
        Serial.print("Final Device Type: ");
        Serial.println(m_deviceType);
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

void Application::updateTypeDetectionMode()
{
        u32 now = millis();

        // Read device type every 1000ms (1 second)
        if (now - m_lastTypeRead >= 1000)
        {
                m_lastTypeRead = now;

                // Re-read device type from ADC (non-verbose - no auto-logging)
                m_deviceType = readDeviceType(false);

                // Only log if type has changed
                if (m_deviceType != m_lastDetectedType)
                {
                        Serial.print("Type changed: ");
                        Serial.print(m_lastDetectedType);
                        Serial.print(" -> ");
                        Serial.println(m_deviceType);
                        m_lastDetectedType = m_deviceType;
                }

                // Flash LED once (100ms pulse)
                digitalWrite(STATUS_LED_PIN, HIGH);
                m_typeDetectionBlink = true;

                // Play a short beep
                m_synth->setWaveform(WAVE_SQUARE);
                m_synth->setADSR(1, 10, 20, 20);
                m_synth->playNote(NOTE_A4, 50, 100);
        }

        // Turn off LED after 100ms flash
        if (m_typeDetectionBlink && (now - m_lastTypeRead >= 100))
        {
                digitalWrite(STATUS_LED_PIN, LOW);
                m_typeDetectionBlink = false;
        }
}
