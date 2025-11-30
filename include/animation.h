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
        ANIM_SPARKLE
};

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
        void stop();
        bool isActive() const { return m_active; }

        // Get current animation type
        AnimationType getType() const { return m_type; }

private:
        PixelStrip *m_pixels;
        bool m_active;
        AnimationType m_type;
        u8 m_position;
        u8 m_frameCounter;
        u16 m_stepDelay;

        // Animation implementations
        void updateRedDotChase();
        void updateRainbowCycle();
        void updateBreathing();
        void updateSparkle();
};
