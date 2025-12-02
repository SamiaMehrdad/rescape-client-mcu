/************************* mcupins.h ***************************
 * MCU pin assignments and hardware configuration
 * Board-specific GPIO definitions for ESP32 variants
 * Created by MSK, November 2025
 * Tested and finalized on ESP32-C3 11/19/25
 ***************************************************************/

#ifndef MCUPINS_H
#define MCUPINS_H

#include <Arduino.h>
#include "msk.h"

// Pin definitions for the ESP32 microcontroller
#ifdef SEEED_XIAO_ESP32C3

// ============================================================================
// Seeed XIAO ESP32-C3 Pin Mapping
// ============================================================================
// The XIAO ESP32-C3 board has silk-screen labels (D0-D10, A0-A3) that map
// to specific ESP32-C3 GPIO numbers. Use the GPIO numbers in code.
//
// Digital Pins (silk-screen → GPIO):
//   D0  → GPIO 2   (supports ADC1_CH2, PWM)
//   D1  → GPIO 3   (supports ADC1_CH3, PWM)
//   D2  → GPIO 4   (supports ADC1_CH4, PWM)
//   D3  → GPIO 5   (supports ADC2_CH0, PWM)
//   D4  → GPIO 6   (supports PWM, I2C SDA default)
//   D5  → GPIO 7   (supports PWM, I2C SCL default)
//   D6  → GPIO 21  (supports PWM, UART TX default)
//   D7  → GPIO 20  (supports PWM, UART RX default)
//   D8  → GPIO 8   (supports PWM)
//   D9  → GPIO 9   (supports PWM, Boot button)
//   D10 → GPIO 10  (supports PWM)
//
// Analog Pins (silk-screen → GPIO):
//   A0  → GPIO 2   (ADC1_CH2, same as D0)
//   A1  → GPIO 3   (ADC1_CH3, same as D1)
//   A2  → GPIO 4   (ADC1_CH4, same as D2)
//   A3  → GPIO 5   (ADC2_CH0, same as D3)
//
// Special Notes:
//   - GPIO 9 is connected to the on-board boot button
//   - GPIO 20/21 are default UART pins (USB serial uses different pins)
//   - GPIO 6/7 are default I2C pins
//   - All GPIOs support PWM via LEDC peripheral
// ============================================================================

// Input pins
constexpr u8 BTN_1_PIN = 9; // D9 - Boot button (only button)

// Status LED pin
constexpr u8 STATUS_LED_PIN = 3; // D1/A1 - Status indicator LED (GPIO 3)
// Status modes: ON=OK, Fast blink (5Hz)=I2C-ERR, Slow blink (1Hz)=TYPE-ERR

// I2C pins (default)
constexpr u8 I2C_SDA_PIN = 6; // D4 - I2C SDA
constexpr u8 I2C_SCL_PIN = 7; // D5 - I2C SCL

// UART pins (default)
constexpr u8 TX_PIN = 21;      // D6 - UART TX (default)
constexpr u8 RX_PIN = 20;      // D7 - UART RX (default, note: changed from GPIO22)
constexpr u8 RS485_DE_PIN = 8; // D8 - Driver Enable for RS-485 (not used with auto-direction)

constexpr u8 SPKR_PIN = 5; // D3 - PWM audio output (GPIO 5)

constexpr u8 PIXEL_PIN = 4; // D2 - WS2812B data line (GPIO 4)

// Configuration ADC pin for device type selection (5-bit = 32 types)
// Hardware: 10kΩ-25kΩ multi-turn trimmer pot between 3.3V and GND
// Wiper connected to CONFIG_ADC_PIN for voltage adjustment (0V-3.3V)
constexpr u8 CONFIG_ADC_PIN = 2; // D0/A0 - Device type configuration (GPIO 2, ADC1_CH2)

// I/O Expander Configuration (PCF8575-based)
constexpr u8 IO_EXPANDER_I2C_ADDR = 0x20; // I/O Expander base address (A0=A1=A2=0)
constexpr u8 IO_EXPANDER_INT_PIN = 10;    // D10 - I/O Expander interrupt pin (optional, active LOW)

#elif defined(S2_MINI)

// ============================================================================
// ESP32-S2-Mini (Wemos/LOLIN S2 Mini) Pin Mapping
// ============================================================================
// The ESP32-S2-Mini board exposes GPIO pins directly (no D0-style labels).
// GPIO numbering matches the ESP32-S2 chip GPIO numbers.
//
// Available GPIO Pins:
//   GPIO 0  → Boot button (pulled up, active low)
//   GPIO 1-14 → General purpose I/O (most support ADC, PWM)
//   GPIO 15-17 → General purpose I/O
//   GPIO 18 → On-board RGB LED (some boards)
//   GPIO 21 → General purpose I/O
//   GPIO 26 → General purpose I/O
//   GPIO 33-40 → General purpose I/O (GPIO33-34 support ADC)
//
// ADC Channels:
//   ADC1: GPIO 1-10 (ADC1_CH0 to ADC1_CH9)
//   ADC2: GPIO 11-20 (ADC2_CH0 to ADC2_CH9)
//
// Default Peripheral Pins:
//   I2C: SDA=33, SCL=35 (configurable)
//   SPI: MOSI=35, MISO=37, SCK=36, SS=34 (configurable)
//   UART: TX=43, RX=44 (USB CDC, separate from GPIO UART)
//
// Special Notes:
//   - GPIO 0 connected to boot button (use with pull-up)
//   - GPIO 18 may be connected to RGB LED (board variant dependent)
//   - All GPIOs support PWM via LEDC peripheral
//   - USB pins (GPIO 19/20) reserved for USB, don't use for GPIO
//   - Strapping pins: GPIO 0, 45, 46 (avoid if possible)
// ============================================================================

// Input pins
constexpr u8 BTN_1_PIN = 14; // GPIO 14 (only button)

// Status LED pin
constexpr u8 STATUS_LED_PIN = 3; // GPIO 3 - Status indicator LED
// Status modes: ON=OK, Fast blink (5Hz)=I2C-ERR, Slow blink (1Hz)=TYPE-ERR

// I2C pins (default)
constexpr u8 I2C_SDA_PIN = 6; // GPIO 6 - I2C SDA
constexpr u8 I2C_SCL_PIN = 7; // GPIO 7 - I2C SCL

// UART pins (default)
constexpr u8 TX_PIN = 21;       // GPIO 21 - UART TX
constexpr u8 RX_PIN = 22;       // GPIO 22 - UART RX
constexpr u8 RS485_DE_PIN = 18; // GPIO 18 - Driver Enable for RS-485 (not used with auto-direction)

constexpr u8 SPKR_PIN = 5; // GPIO 5 - PWM audio output

constexpr u8 PIXEL_PIN = 4; // GPIO 4 - WS2812B data line

// Configuration ADC pin for device type selection (5-bit = 32 types)
// Hardware: 10kΩ-25kΩ multi-turn trimmer pot between 3.3V and GND
// Wiper connected to CONFIG_ADC_PIN for voltage adjustment (0V-3.3V)
constexpr u8 CONFIG_ADC_PIN = 1; // GPIO 1 - Device type configuration (ADC1_CH0)

// I/O Expander Configuration (PCF8575-based)
constexpr u8 IO_EXPANDER_I2C_ADDR = 0x20; // I/O Expander base address (A0=A1=A2=0)
constexpr u8 IO_EXPANDER_INT_PIN = 12;    // GPIO 12 - I/O Expander interrupt pin (optional, active LOW)

#else
#error No board defined! Define SEEED_XIAO_ESP32C3 or S2_MINI.
#endif // BOARD CHECK

// LEDC channel assignment for audio output
constexpr u8 AUDIO_PWM_CHANNEL = 0;

#endif // MCUPINS_H