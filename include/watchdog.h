/************************* watchdog.h **************************
 * Platform-independent watchdog timer abstraction
 * Supports ESP32, AVR, SAM, and ARM Cortex-M MCUs
 * Created by MSK, November 2025
 * Tested and finalized on ESP32-C3 11/19/25
 ***************************************************************/

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>

/**
 * @brief Platform-independent watchdog timer abstraction.
 *
 * Supports ESP32, AVR (ATmega), SAM (Arduino Due), and ARM Cortex-M.
 * Provides a simple init/reset API for keeping the system alive.
 */
class Watchdog
{
public:
        /**
         * @brief Initialize the hardware watchdog with a timeout in seconds.
         * @param timeoutSeconds Watchdog timeout period (platform may round to nearest supported value)
         * @param enablePanic If true, system will reset on timeout; if false, may trigger interrupt only (platform-dependent)
         */
        static void begin(uint32_t timeoutSeconds = 1, bool enablePanic = true);

        /**
         * @brief Reset (feed) the watchdog to prevent timeout.
         * Must be called regularly before the timeout expires.
         */
        static void reset();

        /**
         * @brief Disable the watchdog timer (if platform supports it).
         * Note: Some platforms do not allow disabling once enabled.
         */
        static void disable();
};

#endif // WATCHDOG_H
