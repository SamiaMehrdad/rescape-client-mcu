#define SEEED_XIAO_ESP32C3
// #define S2_MINI
#define TYPE_A_RGB_2_BTN
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

// Palette of colors cycled by the buttons
constexpr u32 kColors[] = {CLR_WT, CLR_PN, CLR_PR, CLR_MG, CLR_OR, CLR_YL, CLR_RD, CLR_BL, CLR_CY, CLR_GR};
constexpr size_t kColorCount = sizeof(kColors) / sizeof(kColors[0]);

// Timer configuration: ISR interval in milliseconds
constexpr u8 ISR_INTERVAL_MS = 5; // 5 ms = 200 Hz

// Animation timing: Desired pixel refresh interval in milliseconds
constexpr u8 ANIM_REFRESH_MS = 40; // 40 ms = 25 Hz pixel buffer update

// Animation step timing: How fast the red dot moves (in milliseconds per step)
constexpr u16 ANIM_STEP_MS = 50; // 50 ms per step = 20 Hz animation movement

// Synthesizer instance drives the speaker via PWM
Synth synth(SPKR_PIN, AUDIO_PWM_CHANNEL);

// PixelStrip(pin, logicalCount, groupSize, brightness)
// Example: 10 logical pixels, group size 4 = 40 physical LEDs
PixelStrip pixels(PIXEL_PIN, 8, 1, 5);

// RS-485 communication for Room Bus (no DE pin - auto-direction transceiver)
RoomSerial roomBus(RX_PIN, TX_PIN, -1, 9600); // -1 = no DE pin

// I/O Expander (keypad, motors, switches)
IOExpander ioExpander(IO_EXPANDER_I2C_ADDR, &Wire);

// Hardware timer updates the button state machine every 10 ms
hw_timer_t *timer = nullptr;

// Flag to signal pixel update from ISR
volatile bool pixelUpdateFlag = false;

// Forward declarations
static void playMelody();
void IRAM_ATTR refreshTimer();

//----------------------------------------------------------------------
// Timer ISR: refresh button logic and trigger pixel updates
void IRAM_ATTR refreshTimer()
{
  static u8 animDelay = 0;

  updateButtons();

  // Trigger pixel update based on ANIM_REFRESH_MS / ISR_INTERVAL_MS
  if (!(++animDelay % (ANIM_REFRESH_MS / ISR_INTERVAL_MS)))
    pixelUpdateFlag = true;
}

// Optional startup flourish; left here for future use
static void playMelody()
{
  synth.setWaveform(WAVE_SINE);
  synth.setADSR(20, 100, 180, 150);

  synth.playNote(NOTE_C4, 300, 200);
  while (synth.isPlaying())
    delay(100);

  synth.playNote(NOTE_E4, 300, 200);
  while (synth.isPlaying())
    delay(100);

  synth.playNote(NOTE_G4, 300, 200);
  while (synth.isPlaying())
    delay(100);

  synth.playNote(NOTE_C5, 600, 200);
  while (synth.isPlaying())
    delay(100);
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ESP32 Button Test Starting ===");

  // Disable I2C error logging to reduce Serial spam
  esp_log_level_set("i2c", ESP_LOG_NONE);

  // Initialize I2C for I/O Expander
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println("I2C initialized");

  // Initialize I/O Expander
  if (ioExpander.begin())
  {
    Serial.println("I/O Expander initialized successfully");

    // Stop all motors on startup
    ioExpander.stopAllMotors();
  }
  else
  {
    Serial.println("WARNING: I/O Expander not found!");
    Serial.println("Check I2C connections and address (default 0x20)");
  }

  // Initialize pixel strip
  pixels.begin();

  // Prepare button handling and audio synth
  initButtons(BTN_1_PIN, BTN_2_PIN);

  // Configure hardware timer for button updates and animation timing
  timer = ESPTimer::begin(0, ISR_INTERVAL_MS, &refreshTimer);

  // Initialize platform-independent watchdog (1 second timeout)
  Watchdog::begin(1, true);

  synth.begin(20000);
  synth.setWaveform(WAVE_TRIANGLE);
  synth.setADSR(
      5,    // attack
      120,  // decay
      100,  // sustain
      180); // release

  // Initialize RS-485 communication for Room Bus
  roomBus.begin();
  Serial.println("RS-485 Room Bus initialized");

  // playMelody(); // Uncomment if a startup riff is desired
}

void loop()
{
  static int colorIndex = 0;
  static bool animationActive = false;
  static u8 animationPosition = 0;
  static u8 animationFrameCounter = 0;

  // Feed the watchdog every loop iteration
  Watchdog::reset();

  // Update pixel display when ISR signals
  if (pixelUpdateFlag)
  {
    pixelUpdateFlag = false;

    // If animation is active, update the buffer with shifting pattern
    if (animationActive)
    {
      // Update animation position based on ANIM_STEP_MS
      animationFrameCounter++;
      if (animationFrameCounter >= (ANIM_STEP_MS / ANIM_REFRESH_MS))
      {
        animationFrameCounter = 0;
        animationPosition++;
        if (animationPosition >= pixels.getCount())
        {
          animationPosition = 0;
        }

        // Debug: print timing info
        static u32 lastStepTime = 0;
        u32 now = millis();
        if (lastStepTime > 0)
        {
          Serial.print("Step time: ");
          Serial.print(now - lastStepTime);
          Serial.print(" ms (should be ");
          Serial.print(ANIM_STEP_MS);
          Serial.println(" ms)");
        }
        lastStepTime = now;
      }

      // Set all pixels to blue, except the current position which is red
      u32 *buffer = pixels.getBuffer();
      for (u8 i = 0; i < pixels.getCount(); i++)
      {
        if (i == animationPosition)
        {
          buffer[i] = CLR_RD; // Red dot
        }
        else
        {
          buffer[i] = CLR_BL; // Blue background
        }
      }
    }

    pixels.applyBuffer();
  }

  // Scan I/O Expander keypad for key presses
  u8 keyIndex = ioExpander.scanKeypad();
  if (keyIndex != 255)
  {
    const int noteMap[16] = {
        NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4,
        NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5,
        NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5,
        NOTE_A5, NOTE_B5, NOTE_C6, NOTE_D6};
    int note = noteMap[keyIndex];
    synth.setWaveform(WAVE_SINE);
    synth.setADSR(5, 50, 100, 100);
    synth.playNote(note, 150, 100);
  }

  // Read switches from I/O Expander
  static bool lastSwitch1 = false;
  static bool lastSwitch2 = false;
  bool switch1 = ioExpander.readSwitch1();
  bool switch2 = ioExpander.readSwitch2();
  if (switch1 != lastSwitch1)
  {
    lastSwitch1 = switch1;
  }
  if (switch2 != lastSwitch2)
  {
    lastSwitch2 = switch2;
  }

  if (keyPressed(BTN1))
  {
    // Stop animation when changing colors manually
    animationActive = false;

    colorIndex = (colorIndex - 1 + kColorCount) % kColorCount;

    // Set all pixels to the selected color
    pixels.setAll(kColors[colorIndex]);

    synth.setWaveform(WAVE_TRIANGLE);
    synth.setADSR(
        10,   // attack
        20,   // decay
        250,  // sustain
        250); // release
    synth.playNote(NOTE_A4, 300, 250);
  }

  if (keyPressed(BTN2))
  {
    // Stop animation when changing colors manually
    animationActive = false;

    colorIndex = (colorIndex + 1) % kColorCount;

    // Create a rainbow pattern across the 10 LEDs
    for (int i = 0; i < pixels.getCount(); i++)
    {
      int idx = (colorIndex + i) % kColorCount;
      pixels.setColor(i, kColors[idx]);
    }

    synth.setWaveform(WAVE_TRIANGLE);
    synth.setADSR(
        5,    // attack
        120,  // decay
        100,  // sustain
        180); // release
    synth.playNote(NOTE_GS4, 300, 250);
  }

  // Long press on either button toggles color shifting animation
  if (keyLongPressed(BTN1) || keyLongPressed(BTN2))
  {
    animationActive = !animationActive;

    if (animationActive)
    {
      Serial.println("Red dot animation started!");
      animationPosition = 0; // Reset animation to start
      animationFrameCounter = 0;
    }
    else
    {
      Serial.println("Animation stopped.");
      pixels.clear();
    }
  }

  // Check for incoming Room Bus frames
  RoomFrame rxFrame;
  if (roomBus.receiveFrame(&rxFrame))
  {
    Serial.print("Room Bus frame received! Addr: 0x");
    Serial.print(rxFrame.addr, HEX);
    Serial.print(" Cmd_srv: 0x");
    Serial.print(rxFrame.cmd_srv, HEX);
    Serial.print(" Cmd_dev: 0x");
    Serial.println(rxFrame.cmd_dev, HEX);

    // Example: Respond to a command
    if (rxFrame.cmd_srv == CMD_GLOW_SET_COLOR)
    {
      // Extract RGB from parameters
      u8 r = rxFrame.p[0];
      u8 g = rxFrame.p[1];
      u8 b = rxFrame.p[2];
      u32 color = (r << 16) | (g << 8) | b;

      pixels.setAll(color);
      pixels.show();

      Serial.println("Color set via Room Bus command");
    }
  }

  delay(10); // Prevent a hot loop; audio + button updates drive responsiveness
}