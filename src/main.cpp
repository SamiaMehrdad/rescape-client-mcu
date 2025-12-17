/************************* main.cpp ****************************
 * Escape Room Client - Main Application Entry Point
 * ESP32-based client controller for escape room automation
 * Created by MSK, November 2025
 * Core + App architecture with device type configuration system
 ***************************************************************/

// Board selection is now configured in platformio.ini via build_flags
// #define SEEED_XIAO_ESP32C3
// #define S2_MINI

#include "msk.h"
#include <Arduino.h>
#include <Wire.h>
#include "esp_log.h"
#include "roombus.h"
#include "mcupins.h"
#include "colors.h"
#include "synth.h"
#include "buttons.h"
#include "watchdog.h"
#include "pixel.h"
#include "roomserial.h"
#include "ioexpander.h"
#include "esptimer.h"
#include "animation.h"
#include "inputmanager.h"
#include "core.h"
#include "test_animation.h"

//============================================================================
// SYSTEM CONFIGURATION
//============================================================================

// Timer configuration: ISR interval in milliseconds
// Note: Using extern const instead of constexpr to allow cross-compilation unit visibility
extern const u8 ISR_INTERVAL_MS = 5;  // 5 ms = 200 Hz
extern const u8 ANIM_REFRESH_MS = 40; // 40 ms = 25 Hz pixel buffer update

// Flag to signal pixel update from ISR
volatile bool pixelUpdateFlag = false;

//============================================================================
// HARDWARE OBJECTS
//============================================================================

// Synthesizer instance drives the speaker via PWM
Synth synth(SPKR_PIN, AUDIO_PWM_CHANNEL);

// PixelStrip(pin, logicalCount, groupSize, brightness)
PixelStrip pixels(PIXEL_PIN, 16, 1, PIXEL_BRIGHTNESS);

// RS-485 communication for Room Bus
RoomSerial roomBus(RX_PIN, TX_PIN, -1, 9600);

// I/O Expander (keypad, motors, switches)
IOExpander ioExpander(IO_EXPANDER_I2C_ADDR, &Wire);

//============================================================================
// CORE FIRMWARE MODULES
//============================================================================

Animation animation(&pixels);
InputManager inputManager(&ioExpander);
Core core(&pixels, &synth, &animation, &inputManager, &roomBus, &ioExpander);

//============================================================================
// ISR AND SYSTEM FUNCTIONS
//============================================================================

// Timer ISR: refresh button logic and trigger pixel updates
/************************* refreshTimer ***********************************
 * ISR: refresh button logic and trigger pixel updates on schedule.
 ***************************************************************/
void IRAM_ATTR refreshTimer()
{
  static u8 animDelay = 0;

  updateButtons();

  // Trigger pixel update based on ANIM_REFRESH_MS / ISR_INTERVAL_MS
  if (!(++animDelay % (ANIM_REFRESH_MS / ISR_INTERVAL_MS)))
    pixelUpdateFlag = true;
}

//============================================================================
// ARDUINO SETUP
//============================================================================
/************************* setup ******************************************
 * Arduino setup: initialize core and start test animation.
 ***************************************************************/
void setup()
{
  // Initialize system via Core
  core.begin();

  // Start test animation
  animation.startBitmap(&kColorTestAnimation, true);
}

//============================================================================
// ARDUINO MAIN LOOP
//============================================================================
/************************* loop *******************************************
 * Arduino main loop: watchdog feed, animation refresh, core update.
 ***************************************************************/
void loop()
{
  // Feed the watchdog
  Watchdog::reset();

  // Refresh pixel animations (timing-critical, driven by ISR flag)
  core.refreshAnimations(pixelUpdateFlag);

  // Update core firmware (handles inputs, status LED, and Room Bus)
  core.update();

  // Small delay to prevent hot loop
  delay(10);
}