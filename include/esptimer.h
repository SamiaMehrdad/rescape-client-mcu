/************************* esptimer.h ***************************
 * ESP32 Hardware Timer Helper Library
 * Simplifies setup and management of ESP32 hardware timers
 * Created by MSK, November 2025
 ***************************************************************/

#ifndef ESPTIMER_H
#define ESPTIMER_H

#include "msk.h"
#include <Arduino.h>

// Maximum number of timers available on ESP32 (typically 4)
#define MAX_TIMERS 4

// Timer callback function type
typedef void (*TimerCallback)(void);

class ESPTimer
{
public:
        /**
         * Initialize a hardware timer
         * @param timerNum Timer number (0-3 for ESP32)
         * @param intervalMs Timer interval in milliseconds
         * @param callback Function to call on timer interrupt (must be IRAM_ATTR)
         * @param autoReload If true, timer repeats automatically; if false, runs once
         * @return Pointer to initialized timer, or nullptr on failure
         */
        static hw_timer_t *begin(u8 timerNum, u16 intervalMs, TimerCallback callback, bool autoReload = true);

        /**
         * Initialize a hardware timer with microsecond precision
         * @param timerNum Timer number (0-3 for ESP32)
         * @param intervalUs Timer interval in microseconds
         * @param callback Function to call on timer interrupt (must be IRAM_ATTR)
         * @param autoReload If true, timer repeats automatically; if false, runs once
         * @return Pointer to initialized timer, or nullptr on failure
         */
        static hw_timer_t *beginMicros(u8 timerNum, u32 intervalUs, TimerCallback callback, bool autoReload = true);

        /**
         * Stop and detach a timer
         * @param timer Pointer to timer returned by begin()
         */
        static void end(hw_timer_t *timer);

        /**
         * Change timer interval (timer must be stopped first)
         * @param timer Pointer to timer
         * @param intervalMs New interval in milliseconds
         */
        static void setInterval(hw_timer_t *timer, u16 intervalMs);

        /**
         * Change timer interval in microseconds (timer must be stopped first)
         * @param timer Pointer to timer
         * @param intervalUs New interval in microseconds
         */
        static void setIntervalMicros(hw_timer_t *timer, u32 intervalUs);

        /**
         * Start/resume a timer
         * @param timer Pointer to timer
         */
        static void start(hw_timer_t *timer);

        /**
         * Stop/pause a timer
         * @param timer Pointer to timer
         */
        static void stop(hw_timer_t *timer);

        /**
         * Get the prescaler value for 1 MHz tick rate
         * ESP32 runs at 80 MHz, so prescaler of 80 gives 1 μs per tick
         * @return Prescaler value (80 for ESP32, 40 for ESP32-S2/S3/C3)
         */
        static u8 getPrescaler();

private:
        static hw_timer_t *timers[MAX_TIMERS]; // Track allocated timers
};

#endif // ESPTIMER_H
