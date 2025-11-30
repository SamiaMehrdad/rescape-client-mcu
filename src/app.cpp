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
      m_colorIndex(0)
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

        // Set up input callback
        m_inputManager->setCallback(onInputEvent);

        Serial.println("Application initialized");
}

void Application::update()
{
        // Poll inputs
        m_inputManager->poll();

        // Check for Room Bus commands
        RoomFrame rxFrame;
        if (m_roomBus->receiveFrame(&rxFrame))
        {
                handleRoomBusFrame(rxFrame);
        }
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

        case INPUT_BTN2_PRESS:
                handleButton2Press();
                break;

        case INPUT_BTN1_LONG_PRESS:
        case INPUT_BTN2_LONG_PRESS:
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
        Serial.println("Color change: backward");

        // Stop animation when changing colors manually
        if (m_animation->isActive())
        {
                m_animation->stop();
        }

        // Cycle color backward
        m_colorIndex = (m_colorIndex - 1 + kColorCount) % kColorCount;
        m_pixels->setAll(kColors[m_colorIndex]);
        m_pixels->show(); // Actually update the LEDs!

        // Play sound feedback
        m_synth->setWaveform(WAVE_TRIANGLE);
        m_synth->setADSR(10, 20, 250, 250);
        m_synth->playNote(NOTE_A4, 300, 250);
}

void Application::handleButton2Press()
{
        Serial.println("Color change: forward (rainbow)");

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
        // Toggle animation
        if (m_animation->isActive())
        {
                m_animation->stop();
                Serial.println("Animation stopped");
                m_pixels->clear();
        }
        else
        {
                m_animation->start(ANIM_RED_DOT_CHASE);
                Serial.println("Animation started");
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
