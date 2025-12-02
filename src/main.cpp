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

//============================================================================
// SYSTEM CONFIGURATION
//============================================================================

// Timer configuration: ISR interval in milliseconds
constexpr u8 ISR_INTERVAL_MS = 5;  // 5 ms = 200 Hz
constexpr u8 ANIM_REFRESH_MS = 40; // 40 ms = 25 Hz pixel buffer update

// Flag to signal pixel update from ISR
volatile bool pixelUpdateFlag = false;

//============================================================================
// HARDWARE OBJECTS
//============================================================================

// Synthesizer instance drives the speaker via PWM
Synth synth(SPKR_PIN, AUDIO_PWM_CHANNEL);

// PixelStrip(pin, logicalCount, groupSize, brightness)
PixelStrip pixels(PIXEL_PIN, 8, 1, 5);

// RS-485 communication for Room Bus
RoomSerial roomBus(RX_PIN, TX_PIN, -1, 9600);

// I/O Expander (keypad, motors, switches)
IOExpander ioExpander(IO_EXPANDER_I2C_ADDR, &Wire);

// Hardware timer
hw_timer_t *timer = nullptr;

//============================================================================
// CORE FIRMWARE MODULES
//============================================================================

Animation animation(&pixels);
InputManager inputManager(&ioExpander);
Core core(&pixels, &synth, &animation, &inputManager, &roomBus);

//============================================================================
// ISR AND SYSTEM FUNCTIONS
//============================================================================

// Timer ISR: refresh button logic and trigger pixel updates
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
void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32 Escape Room Client Starting ===");

  // Disable I2C error logging to reduce Serial spam
  esp_log_level_set("i2c", ESP_LOG_NONE);

  // Initialize I2C for I/O Expander
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println("I2C initialized");

  // Initialize I/O Expander
  bool i2cOk = false;
  if (ioExpander.begin())
  {
    Serial.println("I/O Expander initialized");
    ioExpander.stopAllMotors();
    i2cOk = true;
  }
  else
  {
    Serial.println("WARNING: I/O Expander not found - keypad/switches disabled");
  }

  // Initialize pixel strip
  pixels.begin();
  Serial.println("Pixel strip initialized");

  // Initialize button handling (single button now)
  initButtons(BTN_1_PIN);
  Serial.println("Button initialized");

  // Configure hardware timer for button updates and animation timing
  timer = ESPTimer::begin(0, ISR_INTERVAL_MS, &refreshTimer);
  Serial.println("Timer initialized");

  // Initialize watchdog (1 second timeout)
  Watchdog::begin(1, true);
  Serial.println("Watchdog initialized");

  // Initialize synthesizer
  synth.init(SOUND_DEFAULT);
  Serial.println("Synthesizer initialized");

  // Initialize RS-485 communication for Room Bus
  roomBus.begin();
  Serial.println("Room Bus initialized");

  // Initialize core firmware modules
  animation.init();    // Animation system
  inputManager.init(); // Input management for keypad and switches
  core.init();         // Core firmware logic

  // Set status LED based on I2C health
  if (!i2cOk)
  {
    core.setStatusLed(STATUS_I2C_ERROR);
    Serial.println("Status LED: I2C ERROR mode (fast blink)");
  }
  else
  {
    core.setStatusLed(STATUS_OK);
    Serial.println("Status LED: OK mode (solid ON)");
  }

  Serial.println("=== System Ready ===\n");

  // Print comprehensive boot report
  core.printBootReport();
}

//============================================================================
// ARDUINO MAIN LOOP
//============================================================================
void loop()
{
  // Feed the watchdog
  Watchdog::reset();

  // Update pixel animations when flagged by ISR
  animation.refresh(pixelUpdateFlag);

  // Update core firmware (handles inputs and Room Bus)
  core.update();

  // Small delay to prevent hot loop
  delay(10);
}