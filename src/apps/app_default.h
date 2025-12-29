/************************* app_default.h ***********************
 * Default Application Implementation
 * Fallback app for undefined device types
 * Created by MSK, November 2025
 ***************************************************************/

#pragma once
#include "app_base.h"
#include "pixel.h"

class AppDefault : public AppBase
{
public:
        /************************* setup ***********************************
         * Initializes the Default application.
         * Clears pixels.
         ***************************************************************/
        void setup(const AppContext &context) override
        {
                AppBase::setup(context);
                // Default behavior: Clear pixels
                if (m_context.pixels)
                {
                        m_context.pixels->clear();
                        m_context.pixels->show();
                }
        }

        /************************* loop ***********************************
         * Main loop for the Default application.
         * WARNING: Do not use long delays (delay()) or blocking loops here.
         * If this loop takes longer than the watchdog timeout,
         * the system will reset. Use millis() for timing instead.
         *
         * Performance Note: Typical loop cycle time is ~13ms (~75Hz)
         * due to Core overhead (Input polling, I2C, etc).
         ***************************************************************/
        void loop() override
        {
                // Do nothing by default
        }
};
