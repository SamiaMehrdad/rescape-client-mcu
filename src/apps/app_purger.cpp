#include "app_purger.h"
#include <Arduino.h>
#include "pixel.h"

void AppPurger::setup(const AppContext &context)
{
        AppBase::setup(context);
        Serial.println("--- PURGER APP STARTED ---");

        // Example: Set LEDs to Red to indicate danger
        if (m_context.pixels)
        {
                m_context.pixels->setAll(0xFF0000); // Red
                m_context.pixels->show();
        }
}

void AppPurger::loop()
{
        // Purger logic would go here
}

void AppPurger::handleInput(InputEvent event)
{
        Serial.print("Purger Input: ");
        Serial.println(event);
}

void AppPurger::handleCommand(const RoomFrame &frame)
{
        // Handle purger-specific commands here
}
