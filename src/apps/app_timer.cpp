#include "app_timer.h"
#include <Arduino.h>
#include "pixel.h"
#include "roombus.h"

void AppTimer::setup(const AppContext &context)
{
        AppBase::setup(context);
        Serial.println("--- TIMER APP STARTED ---");

        // Example: Set LEDs to Blue to indicate timer
        if (m_context.pixels)
        {
                m_context.pixels->setAll(0x0000FF); // Blue
                m_context.pixels->show();
        }
}

void AppTimer::loop()
{
        // Timer logic would go here
}

void AppTimer::handleInput(InputEvent event)
{
        Serial.print("Timer Input: ");
        Serial.println(event);

        // Mock: Send event on button press
        if (event == INPUT_BTN1_PRESS)
        {
                sendEvent(EV_TMR_DONE);
                Serial.println("Sent EV_TMR_DONE");
        }
}

// Handle RoomBus commands specific to Timer
// Commands are defined in roombus.h

void AppTimer::handleCommand(const RoomFrame &frame)
{
        Serial.print("Timer received command: 0x");
        Serial.println(frame.cmd_srv, HEX);

        switch (frame.cmd_srv)
        {
        case TMR_START:
                Serial.println("-> START TIMER");
                // Mock: Change color to Green
                if (m_context.pixels)
                {
                        m_context.pixels->setAll(0x00FF00);
                        m_context.pixels->show();
                }
                break;

        case TMR_PAUSE:
                Serial.println("-> PAUSE TIMER");
                // Mock: Change color to Yellow
                if (m_context.pixels)
                {
                        m_context.pixels->setAll(0xFFFF00);
                        m_context.pixels->show();
                }
                break;

        default:
                break;
        }
}
