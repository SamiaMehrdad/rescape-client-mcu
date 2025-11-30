#include "animation.h"
#include "colors.h"

//============================================================================
// CONFIGURATION
//============================================================================

// Animation timing constants
constexpr u8 ANIM_REFRESH_MS = 40; // 40 ms = 25 Hz pixel buffer update
constexpr u16 ANIM_STEP_MS = 50;   // 50 ms per step = 20 Hz animation movement
constexpr u8 FRAME_DIVISOR = ANIM_STEP_MS / ANIM_REFRESH_MS;

//============================================================================
// CONSTRUCTOR
//============================================================================

Animation::Animation(PixelStrip *pixelStrip)
    : m_pixels(pixelStrip),
      m_active(false),
      m_type(ANIM_NONE),
      m_position(0),
      m_frameCounter(0),
      m_stepDelay(FRAME_DIVISOR)
{
}

//============================================================================
// PUBLIC METHODS
//============================================================================

void Animation::init()
{
        m_active = false;
        m_type = ANIM_NONE;
        m_position = 0;
        m_frameCounter = 0;
}

void Animation::start(AnimationType type)
{
        m_type = type;
        m_active = true;
        m_position = 0;
        m_frameCounter = 0;
}

void Animation::stop()
{
        m_active = false;
        m_type = ANIM_NONE;
        m_pixels->clear();
}

void Animation::refresh(volatile bool &flag)
{
        if (flag)
        {
                flag = false;
                update();
        }
}

void Animation::update()
{
        if (!m_active || m_type == ANIM_NONE)
        {
                return;
        }

        // Update animation based on type
        switch (m_type)
        {
        case ANIM_RED_DOT_CHASE:
                updateRedDotChase();
                break;
        case ANIM_RAINBOW_CYCLE:
                updateRainbowCycle();
                break;
        case ANIM_BREATHING:
                updateBreathing();
                break;
        case ANIM_SPARKLE:
                updateSparkle();
                break;
        default:
                break;
        }

        m_pixels->applyBuffer();
}

//============================================================================
// ANIMATION IMPLEMENTATIONS
//============================================================================

void Animation::updateRedDotChase()
{
        // Update position based on frame counter
        m_frameCounter++;
        if (m_frameCounter >= m_stepDelay)
        {
                m_frameCounter = 0;
                m_position++;
                if (m_position >= m_pixels->getCount())
                {
                        m_position = 0;
                }
        }

        // Set all pixels to blue, except the current position which is red
        u32 *buffer = m_pixels->getBuffer();
        for (u8 i = 0; i < m_pixels->getCount(); i++)
        {
                if (i == m_position)
                {
                        buffer[i] = CLR_RD; // Red dot
                }
                else
                {
                        buffer[i] = CLR_BL; // Blue background
                }
        }
}

void Animation::updateRainbowCycle()
{
        // Cycle through colors
        m_frameCounter++;
        if (m_frameCounter >= m_stepDelay)
        {
                m_frameCounter = 0;
                m_position++;
        }

        // Create rainbow effect
        constexpr u32 rainbow[] = {CLR_RD, CLR_OR, CLR_YL, CLR_GR, CLR_CY, CLR_BL, CLR_PR, CLR_MG};
        constexpr u8 rainbowSize = sizeof(rainbow) / sizeof(rainbow[0]);

        u32 *buffer = m_pixels->getBuffer();
        for (u8 i = 0; i < m_pixels->getCount(); i++)
        {
                buffer[i] = rainbow[(i + m_position) % rainbowSize];
        }
}

void Animation::updateBreathing()
{
        // Simple breathing effect using brightness modulation
        // This is a placeholder - you'd need to implement brightness control in PixelStrip
        m_frameCounter++;
        if (m_frameCounter >= 2)
        {
                m_frameCounter = 0;
                m_position++;
                if (m_position >= 100)
                        m_position = 0;
        }

        // For now, just pulse white
        u8 brightness = (m_position < 50) ? m_position * 2 : (100 - m_position) * 2;
        u32 color = (brightness << 16) | (brightness << 8) | brightness;
        m_pixels->setAll(color);
}

void Animation::updateSparkle()
{
        // Random sparkle effect
        m_frameCounter++;
        if (m_frameCounter >= 3)
        {
                m_frameCounter = 0;
                m_position = (m_position + 7) % m_pixels->getCount(); // Pseudo-random
        }

        u32 *buffer = m_pixels->getBuffer();
        for (u8 i = 0; i < m_pixels->getCount(); i++)
        {
                if (i == m_position)
                {
                        buffer[i] = CLR_WT; // White sparkle
                }
                else
                {
                        buffer[i] = 0; // Off
                }
        }
}
