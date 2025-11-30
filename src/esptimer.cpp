/************************* esptimer.cpp ************************
 * ESP32 Hardware Timer Helper Library Implementation
 * Created by MSK, November 2025
 ***************************************************************/

#include "esptimer.h"

// Static member initialization
hw_timer_t *ESPTimer::timers[MAX_TIMERS] = {nullptr};

/**
 * Initialize a hardware timer with millisecond interval
 */
hw_timer_t *ESPTimer::begin(u8 timerNum, u16 intervalMs, TimerCallback callback, bool autoReload)
{
        return beginMicros(timerNum, (u32)intervalMs * 1000, callback, autoReload);
}

/**
 * Initialize a hardware timer with microsecond interval
 */
hw_timer_t *ESPTimer::beginMicros(u8 timerNum, u32 intervalUs, TimerCallback callback, bool autoReload)
{
        if (timerNum >= MAX_TIMERS || callback == nullptr)
        {
                return nullptr;
        }

        // Clean up existing timer if already allocated
        if (timers[timerNum] != nullptr)
        {
                end(timers[timerNum]);
        }

        // Initialize timer
        // Parameters: timer number, prescaler, count up (true)
        // Prescaler 80 divides 80 MHz clock to 1 MHz (1 μs per tick)
        timers[timerNum] = timerBegin(timerNum, getPrescaler(), true);

        if (timers[timerNum] == nullptr)
        {
                return nullptr;
        }

        // Attach interrupt callback
        timerAttachInterrupt(timers[timerNum], callback, true);

        // Set alarm value (in microseconds) and auto-reload
        timerAlarmWrite(timers[timerNum], intervalUs, autoReload);

        // Enable the alarm
        timerAlarmEnable(timers[timerNum]);

        return timers[timerNum];
}

/**
 * Stop and detach a timer
 */
void ESPTimer::end(hw_timer_t *timer)
{
        if (timer == nullptr)
        {
                return;
        }

        // Disable alarm
        timerAlarmDisable(timer);

        // Detach interrupt
        timerDetachInterrupt(timer);

        // End timer
        timerEnd(timer);

        // Clear from tracking array
        for (u8 i = 0; i < MAX_TIMERS; i++)
        {
                if (timers[i] == timer)
                {
                        timers[i] = nullptr;
                        break;
                }
        }
}

/**
 * Change timer interval in milliseconds
 */
void ESPTimer::setInterval(hw_timer_t *timer, u16 intervalMs)
{
        setIntervalMicros(timer, (u32)intervalMs * 1000);
}

/**
 * Change timer interval in microseconds
 */
void ESPTimer::setIntervalMicros(hw_timer_t *timer, u32 intervalUs)
{
        if (timer == nullptr)
        {
                return;
        }

        // Disable alarm
        timerAlarmDisable(timer);

        // Write new alarm value (auto-reload preserved)
        timerWrite(timer, 0); // Reset counter
        timerAlarmWrite(timer, intervalUs, true);

        // Re-enable alarm
        timerAlarmEnable(timer);
}

/**
 * Start/resume a timer
 */
void ESPTimer::start(hw_timer_t *timer)
{
        if (timer == nullptr)
        {
                return;
        }

        timerAlarmEnable(timer);
}

/**
 * Stop/pause a timer
 */
void ESPTimer::stop(hw_timer_t *timer)
{
        if (timer == nullptr)
        {
                return;
        }

        timerAlarmDisable(timer);
}

/**
 * Get prescaler for 1 MHz tick rate
 */
u8 ESPTimer::getPrescaler()
{
        // ESP32 classic runs at 80 MHz, needs prescaler of 80 for 1 μs ticks
        // ESP32-S2/S3/C3 may run at 40 MHz, needs prescaler of 40
        // For simplicity, use 80 (works on all variants)
        return 80;
}
