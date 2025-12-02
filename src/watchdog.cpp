/************************* watchdog.cpp ************************
 * Watchdog Timer Implementation
 * Multi-platform watchdog support for system reliability
 * Created by MSK, November 2025
 * Supports ESP32, AVR, SAM, SAMD, and Teensy platforms
 ***************************************************************/

#include "watchdog.h"

// ============================================================================
// Platform detection and implementation
// ============================================================================

#if defined(SEEED_XIAO_ESP32C3) || defined(S2_MINI) || defined(ESP32)
// ---------------------------- ESP32 Implementation --------------------------
#include <esp_task_wdt.h>

static bool wdtInitialized = false;

void Watchdog::begin(uint32_t timeoutSeconds, bool enablePanic)
{
        if (wdtInitialized)
        {
                return; // Already initialized
        }

        esp_task_wdt_init(timeoutSeconds, enablePanic);
        esp_task_wdt_add(NULL); // Add current task
        wdtInitialized = true;
}

void Watchdog::reset()
{
        if (wdtInitialized)
        {
                esp_task_wdt_reset();
        }
}

void Watchdog::disable()
{
        if (wdtInitialized)
        {
                esp_task_wdt_delete(NULL);
                esp_task_wdt_deinit();
                wdtInitialized = false;
        }
}

// ============================================================================
#elif defined(__AVR__)
// ---------------------------- AVR (ATmega) Implementation -------------------
#include <avr/wdt.h>

void Watchdog::begin(uint32_t timeoutSeconds, bool enablePanic)
{
        // AVR watchdog timeout values are fixed: 15ms, 30ms, 60ms, 120ms, 250ms, 500ms, 1s, 2s, 4s, 8s
        uint8_t wdtTimeout;

        if (timeoutSeconds >= 8)
                wdtTimeout = WDTO_8S;
        else if (timeoutSeconds >= 4)
                wdtTimeout = WDTO_4S;
        else if (timeoutSeconds >= 2)
                wdtTimeout = WDTO_2S;
        else if (timeoutSeconds >= 1)
                wdtTimeout = WDTO_1S;
        else if (timeoutSeconds * 1000 >= 500)
                wdtTimeout = WDTO_500MS;
        else if (timeoutSeconds * 1000 >= 250)
                wdtTimeout = WDTO_250MS;
        else if (timeoutSeconds * 1000 >= 120)
                wdtTimeout = WDTO_120MS;
        else
                wdtTimeout = WDTO_60MS;

        wdt_enable(wdtTimeout);
}

void Watchdog::reset()
{
        wdt_reset();
}

void Watchdog::disable()
{
        wdt_disable();
}

// ============================================================================
#elif defined(__SAM3X8E__) || defined(__SAM3A8C__) || defined(ARDUINO_ARCH_SAM)
// ---------------------------- SAM (Arduino Due) Implementation --------------
#include <Watchdog.h> // Arduino Due Watchdog library

static WatchdogSAM wdt;
static bool wdtInitialized = false;

void Watchdog::begin(uint32_t timeoutSeconds, bool enablePanic)
{
        if (wdtInitialized)
        {
                return;
        }

        // Convert seconds to milliseconds (Due WDT uses ms)
        uint32_t timeoutMs = timeoutSeconds * 1000;

        // Arduino Due WDT maximum is typically 16 seconds (16000 ms)
        if (timeoutMs > 16000)
                timeoutMs = 16000;

        wdt.enable(timeoutMs);
        wdtInitialized = true;
}

void Watchdog::reset()
{
        if (wdtInitialized)
        {
                wdt.restart();
        }
}

void Watchdog::disable()
{
        if (wdtInitialized)
        {
                wdt.disable();
                wdtInitialized = false;
        }
}

// ============================================================================
#elif defined(__arm__) && defined(TEENSYDUINO)
// ---------------------------- Teensy (ARM Cortex-M) Implementation ----------
// Teensy 3.x/4.x use Kinetis or i.MX RT watchdog

void Watchdog::begin(uint32_t timeoutSeconds, bool enablePanic)
{
        // Teensy doesn't have a standard watchdog library in Arduino
        // For now, this is a placeholder - user should implement based on specific Teensy model
        // Example for Teensy 4.x would use RTWDOG peripheral
        (void)timeoutSeconds;
        (void)enablePanic;
        // TODO: Implement Teensy-specific watchdog
}

void Watchdog::reset()
{
        // Placeholder
}

void Watchdog::disable()
{
        // Placeholder
}

// ============================================================================
#else
// ---------------------------- Fallback (No Watchdog) ------------------------
#warning "Watchdog not implemented for this platform"

void Watchdog::begin(uint32_t timeoutSeconds, bool enablePanic)
{
        (void)timeoutSeconds;
        (void)enablePanic;
        // No-op on unsupported platforms
}

void Watchdog::reset()
{
        // No-op
}

void Watchdog::disable()
{
        // No-op
}

#endif
