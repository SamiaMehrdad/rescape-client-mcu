/************************* pixel.cpp ***************************
 * WS2812B LED Strip Control Implementation
 * Pixel strip helper with grouping and buffer support
 * Created by MSK, November 2025
 * Supports logical grouping and animation buffering
 ***************************************************************/

#include "pixel.h"
#include "watchdog.h"
#include <Arduino.h>

PixelStrip::PixelStrip(u8 pin, u8 count, u8 groupSize, u8 brightness)
    : pixels(count * (groupSize > 0 ? groupSize : 1), pin, NEO_GRB + NEO_KHZ800),
      physicalCount(count * (groupSize > 0 ? groupSize : 1)),
      groupSize(groupSize > 0 ? groupSize : 1),
      logicalCount(count),
      colorBuffer(nullptr)
{
        pixels.setBrightness(brightness);

        // Allocate color buffer for animations
        colorBuffer = new u32[logicalCount];

        // Initialize buffer to all black (off)
        for (u8 i = 0; i < logicalCount; i++)
        {
                colorBuffer[i] = 0;
        }
}

void PixelStrip::begin()
{
        pixels.begin();
        pixels.show(); // Initialize all pixels to 'off'
}

void PixelStrip::setColor(u8 index, u8 r, u8 g, u8 b)
{
        if (index < logicalCount)
        {
                // Update buffer
                colorBuffer[index] = ((u32)r << 16) | ((u32)g << 8) | b;

                // Set all LEDs in this group to the same color
                u8 startLed = index * groupSize;
                for (u8 i = 0; i < groupSize; i++)
                {
                        pixels.setPixelColor(startLed + i, pixels.Color(r, g, b));
                }
        }
}

void PixelStrip::setColor(u8 index, u32 color)
{
        if (index < logicalCount)
        {
                // Update buffer
                colorBuffer[index] = color;

                u8 r = (color >> 16) & 0xFF;
                u8 g = (color >> 8) & 0xFF;
                u8 b = color & 0xFF;

                // Set all LEDs in this group to the same color
                u8 startLed = index * groupSize;
                for (u8 i = 0; i < groupSize; i++)
                {
                        pixels.setPixelColor(startLed + i, pixels.Color(r, g, b));
                }
        }
}

void PixelStrip::setAll(u8 r, u8 g, u8 b)
{
        u32 color = ((u32)r << 16) | ((u32)g << 8) | b;

        // Update buffer
        for (u8 i = 0; i < logicalCount; i++)
        {
                colorBuffer[i] = color;
        }

        // Update physical LEDs
        for (u8 i = 0; i < physicalCount; i++)
        {
                pixels.setPixelColor(i, pixels.Color(r, g, b));
        }
}

void PixelStrip::setAll(u32 color)
{
        // Update buffer
        for (u8 i = 0; i < logicalCount; i++)
        {
                colorBuffer[i] = color;
        }

        u8 r = (color >> 16) & 0xFF;
        u8 g = (color >> 8) & 0xFF;
        u8 b = color & 0xFF;

        // Update physical LEDs
        for (u8 i = 0; i < physicalCount; i++)
        {
                pixels.setPixelColor(i, pixels.Color(r, g, b));
        }
}

void PixelStrip::clear()
{
        // Clear buffer
        for (u8 i = 0; i < logicalCount; i++)
        {
                colorBuffer[i] = 0;
        }

        // Clear physical LEDs
        pixels.clear();
}

void PixelStrip::show()
{
        pixels.show();
}

void PixelStrip::setBrightness(u8 brightness)
{
        pixels.setBrightness(brightness);
}

/**
 * Apply the color buffer to the actual NeoPixels
 * This should be called from the ISR to refresh the display
 */
void IRAM_ATTR PixelStrip::applyBuffer()
{
        for (u8 i = 0; i < logicalCount; i++)
        {
                u32 color = colorBuffer[i];
                u8 r = (color >> 16) & 0xFF;
                u8 g = (color >> 8) & 0xFF;
                u8 b = color & 0xFF;

                // Set all LEDs in this group to the same color
                u8 startLed = i * groupSize;
                for (u8 j = 0; j < groupSize; j++)
                {
                        pixels.setPixelColor(startLed + j, pixels.Color(r, g, b));
                }
        }
        pixels.show();
}

/**
 * Pixel check routine - turns on each LED to white one by one
 * Useful for testing that all LEDs are working
 */
void PixelStrip::pixelCheck(u16 delayMs)
{
        Serial.println("\n=== Pixel Check Starting ===");
        Serial.print("Testing ");
        Serial.print(logicalCount);
        Serial.print(" logical pixels (");
        Serial.print(physicalCount);
        Serial.println(" physical LEDs)...");

        // Save current brightness
        u8 savedBrightness = pixels.getBrightness();

        // Set to maximum brightness for testing
        setBrightness(255);

        // Clear all first
        clear();
        show();

        // Feed watchdog during delay
        u32 startTime = millis();
        while (millis() - startTime < 500)
        {
                Watchdog::reset();
                delay(10);
        }

        // Turn on each logical pixel to white (255, 255, 255) one by one
        // Each pixel stays on, accumulating until all are lit
        for (u8 i = 0; i < logicalCount; i++)
        {
                Serial.print("Pixel ");
                Serial.print(i);
                Serial.println(" -> WHITE");

                setColor(i, 255, 255, 255);
                show();

                // Feed watchdog during delay
                startTime = millis();
                while (millis() - startTime < delayMs)
                {
                        Watchdog::reset();
                        delay(10);
                }
        }

        // Keep all on for 1 second - feed watchdog during this delay too
        startTime = millis();
        while (millis() - startTime < 1000)
        {
                Watchdog::reset();
                delay(10);
        }

        // Clear all
        clear();
        show();

        // Restore original brightness
        setBrightness(savedBrightness);

        Serial.println("=== Pixel Check Complete ===\n");
}
