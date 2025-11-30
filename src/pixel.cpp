/*
    re-escape room project
    pixel.cpp - Pixel strip helper implementation
*/

#include "pixel.h"

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
