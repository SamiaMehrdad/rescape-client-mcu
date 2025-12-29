/************************* app_purger.cpp **********************
 * Purger Application Implementation
 * Logic for the Purger device type
 * Created by MSK, November 2025
 ***************************************************************/

#include "app_purger.h"
#include <Arduino.h>
#include "pixel.h"
#include "synth.h" // Include synth.h to access Synth class and note definitions

/************************* setup ***********************************
 * Initializes the Purger application.
 * Sets initial LED state.
 * @param context The application context.
 ***************************************************************/
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

/************************* loop ***********************************
 * Main loop for the Purger application.
 ***************************************************************/
void AppPurger::loop()
{
        // static u32 loopCount = 0;
        // static u32 lastTime = millis();

        // loopCount++;
        // if (loopCount >= 1000)
        // {
        //         u32 now = millis();
        //         Serial.printf("Purger: Ech loop took %u ms\n", (now - lastTime) / 1000);
        //         lastTime = now;
        //         loopCount = 0;
        // }

        //  Purger logic would go here
}

/************************* handleInput ***********************************
 * Handles input events for the Purger application.
 * @param event The input event ID.
 ***************************************************************/
bool AppPurger::handleInput(InputEvent event)

{
        int keyIndex = getKeypadIndex(event);
        if (keyIndex != -1)
        {
                Serial.print("Purger Key: ");
                Serial.println(keyIndex);
        }
        else
        {
                Serial.print("Purger Input: ");
                Serial.println(event);
        }
        return false; // Let Core handle default actions
}

/************************* handleCommand ***********************************
 * Handles RoomBus commands for the Purger application.
 * @param frame The received command frame.
 ***************************************************************/
void AppPurger::handleCommand(const RoomFrame &frame)
{
        // Handle purger-specific commands here
}
