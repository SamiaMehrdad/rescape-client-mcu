/************************* pixel.h *****************************
 * Pixel strip helper functions for WS2812B LED strips
 * Provides convenient wrappers around Adafruit_NeoPixel
 * Created by MSK, November 2025
 ***************************************************************/

#ifndef PIXEL_H
#define PIXEL_H

#include "msk.h"
#include <Adafruit_NeoPixel.h>

class PixelStrip
{
public:
        /**
         * Constructor
         * @param pin GPIO pin connected to pixel strip
         * @param count Number of logical groups (pixels you control)
         * @param groupSize Number of physical LEDs per logical group (default 1)
         * @param brightness Initial brightness (0-255)
         */
        PixelStrip(u8 pin, u8 count, u8 groupSize = 1, u8 brightness = 25);

        /**
         * Initialize the pixel strip
         */
        void begin();

        /**
         * Set a single logical pixel (group) color using RGB values
         * @param index Logical group index (0 to logicalCount-1)
         * @param r Red value (0-255)
         * @param g Green value (0-255)
         * @param b Blue value (0-255)
         */
        void setColor(u8 index, u8 r, u8 g, u8 b);

        /**
         * Set a single logical pixel (group) color using u32 color value
         * @param index Logical group index (0 to logicalCount-1)
         * @param color 24-bit RGB color (0xRRGGBB)
         */
        void setColor(u8 index, u32 color);

        /**
         * Set all logical pixels (groups) to the same color (RGB)
         */
        void setAll(u8 r, u8 g, u8 b);

        /**
         * Set all logical pixels (groups) to the same color (u32)
         */
        void setAll(u32 color);

        /**
         * Clear all pixels (turn off)
         */
        void clear();

        /**
         * Update the strip with current pixel values
         */
        void show();

        /**
         * Set brightness (0-255)
         */
        void setBrightness(u8 brightness);

        /**
         * Get pointer to the color buffer (for ISR updates)
         * Buffer size is logicalCount, each entry is a 24-bit RGB color
         */
        u32 *getBuffer() { return colorBuffer; }

        /**
         * Apply the color buffer to the actual NeoPixels
         * This should be called from the ISR to refresh the display
         */
        void IRAM_ATTR applyBuffer();

        /**
         * Get the number of logical pixels (groups)
         */
        u8 getCount() const { return logicalCount; }

        /**
         * Get the group size (LEDs per logical pixel)
         */
        u8 getGroupSize() const { return groupSize; }

        /**
         * Get the total number of physical LEDs
         */
        u8 getPhysicalCount() const { return physicalCount; }

        /**
         * Get direct access to underlying Adafruit_NeoPixel object
         */
        Adafruit_NeoPixel &getPixels() { return pixels; }

private:
        Adafruit_NeoPixel pixels;
        u8 physicalCount; // Total physical LEDs
        u8 groupSize;     // LEDs per logical group
        u8 logicalCount;  // Number of logical groups
        u32 *colorBuffer; // Color buffer for animations (logicalCount entries)
};

#endif // PIXEL_H
