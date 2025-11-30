#define SEEED_XIAO_ESP32C3
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
#include "app.h"

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
// APPLICATION MODULES
//============================================================================

Animation animation(&pixels);
InputManager inputManager(&ioExpander);
Application app(&pixels, &synth, &animation, &inputManager, &roomBus);

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
  if (ioExpander.begin())
  {
    Serial.println("I/O Expander initialized");
    ioExpander.stopAllMotors();
  }
  else
  {
    Serial.println("WARNING: I/O Expander not found - keypad/switches disabled");
  }

  // Initialize pixel strip
  pixels.begin();
  Serial.println("Pixel strip initialized");

  // Initialize button handling
  initButtons(BTN_1_PIN, BTN_2_PIN);
  Serial.println("Buttons initialized");

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

  // Initialize application modules
  animation.init();    // Animation system
  inputManager.init(); // Input management for keypad and switches
  app.init();          // Application logic

  Serial.println("=== System Ready ===\n");
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

  // Update application (handles inputs and Room Bus)
  app.update();

  // Small delay to prevent hot loop
  delay(10);
}