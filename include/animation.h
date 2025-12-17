#pragma once

#include "msk.h"
#include "pixel.h"

// Animation types
enum AnimationType
{
        ANIM_NONE,
        ANIM_RED_DOT_CHASE,
        ANIM_RAINBOW_CYCLE,
        ANIM_BREATHING,
        ANIM_SPARKLE,
        ANIM_BITMAP
};

// Bitmap animation data structure
// Self-contained descriptor for memory-mapped RGB animation data
struct BitmapAnimation
{
        const u32 *data; // Pointer to flat array of RGB values (0x00RRGGBB)
                         // Layout: [Frame0_LED0, Frame0_LED1, ... Frame1_LED0...]
        u16 frameCount;  // Total number of frames
        u16 ledCount;    // Number of LEDs per frame
        u8 frameRate;    // Playback speed in Frames Per Second
};

// Template helper to create animation from 2D array
// Automatically deduces frameCount and ledCount from the array dimensions
template <size_t Frames, size_t Leds>
constexpr BitmapAnimation createBitmapAnimation(const u32 (&data)[Frames][Leds], u8 frameRate)
{
        return {&data[0][0], (u16)Frames, (u16)Leds, frameRate};
}

class Animation
{
public:
        // Constructor
        Animation(PixelStrip *pixelStrip);

        // Initialize the animation system
        void init();

        // Refresh animation (checks flag, clears it, and updates if needed)
        void refresh(volatile bool &flag);

        // Update animation state (call from main loop when flagged)
        void update();

        // Start/stop animation
        void start(AnimationType type);

        // Start a bitmap animation
        // animData: Pointer to the bitmap descriptor
        // loop: true for infinite loop, false for one-shot
        void startBitmap(const BitmapAnimation *animData, bool loop = true);

        // Stop animation
        // clearPixels: true to turn off LEDs, false to leave them as-is (pause)
        void stop(bool clearPixels = true);

        // Pause animation (stops updating but keeps current frame)
        void pause() { stop(false); }

        bool isActive() const { return m_active; }

        // Get current animation type
        AnimationType getType() const { return m_type; }

private:
        PixelStrip *m_pixels;
        bool m_active;
        AnimationType m_type;
        u16 m_position; // Changed from u8 to u16 to support larger frame counts
        u16 m_frameCounter;
        u16 m_stepDelay;

        // Bitmap animation state
        const BitmapAnimation *m_currentBitmap;
        bool m_bitmapLoop;
        u32 m_lastFrameTime;

        // Animation implementations
        void updateRedDotChase();
        void updateRainbowCycle();
        void updateBreathing();
        void updateSparkle();
        void updateBitmap();
};
