/************************* animation.cpp ***********************
 * LED Animation System Implementation
 * Provides buffer-based animations for WS2812B LED strips
 * Created by MSK, November 2025
 * Supports chase, rainbow, breathing, and sparkle effects
 ***************************************************************/

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

/************************* Animation constructor ****************************
 * Construct the animation controller for a pixel strip.
 ***************************************************************/
Animation::Animation(PixelStrip *pixelStrip)
    : m_pixels(pixelStrip),
      m_active(false),
      m_type(ANIM_NONE),
      m_position(0),
      m_frameCounter(0),
      m_stepDelay(FRAME_DIVISOR),
      m_currentBitmap(nullptr),
      m_bitmapLoop(false),
      m_lastFrameTime(0)
{
}

//============================================================================
// PUBLIC METHODS
//============================================================================

/************************* init *******************************************
 * Reset animation state and disable any running effect.
 ***************************************************************/
void Animation::init()
{
        m_active = false;
        m_type = ANIM_NONE;
        m_position = 0;
        m_frameCounter = 0;
        m_currentBitmap = nullptr;
}

/************************* start ******************************************
 * Start a built-in animation type.
 * @param type Animation type enum.
 ***************************************************************/
void Animation::start(AnimationType type)
{
        m_type = type;
        m_active = true;
        m_position = 0;
        m_frameCounter = 0;
}

/************************* startBitmap ************************************
 * Start a bitmap animation sequence.
 * @param animData Pointer to bitmap frames.
 * @param loop Whether to loop when finished.
 ***************************************************************/
void Animation::startBitmap(const BitmapAnimation *animData, bool loop)
{
        if (!animData || !animData->data)
                return;

        m_currentBitmap = animData;
        m_bitmapLoop = loop;
        m_type = ANIM_BITMAP;
        m_active = true;
        m_position = 0; // Current frame index
        m_lastFrameTime = millis();

        // Force immediate update of first frame
        updateBitmap();
        m_pixels->applyBuffer();
}

/************************* stop *******************************************
 * Stop any running animation.
 * @param clearPixels Optionally clear the strip.
 ***************************************************************/
void Animation::stop(bool clearPixels)
{
        m_active = false;
        m_type = ANIM_NONE;
        if (clearPixels)
        {
                m_pixels->clear();
        }
}

/************************* refresh ****************************************
 * Tick animations when the ISR flag is set.
 * @param flag ISR-raised refresh flag (cleared here).
 ***************************************************************/
void Animation::refresh(volatile bool &flag)
{
        if (flag)
        {
                flag = false;
                update();
        }
}

/************************* update *****************************************
 * Advance the current animation one frame.
 ***************************************************************/
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
        case ANIM_BITMAP:
                updateBitmap();
                break;
        default:
                break;
        }

        m_pixels->applyBuffer();
}

//============================================================================
// ANIMATION IMPLEMENTATIONS
//============================================================================

/************************* updateBitmap ***********************************
 * Render the current bitmap frame and advance timing.
 ***************************************************************/
void Animation::updateBitmap()
{
        if (!m_currentBitmap)
                return;

        // Check timing
        u32 now = millis();
        u32 frameDelay = 1000 / m_currentBitmap->frameRate;

        if (now - m_lastFrameTime < frameDelay && m_position > 0)
        {
                return; // Not time for next frame yet
        }

        // Update frame time
        m_lastFrameTime = now;

        // Calculate offset in data array
        // Layout: [Frame0_LED0, Frame0_LED1, ... Frame1_LED0...]
        u32 offset = (u32)m_position * m_currentBitmap->ledCount;

        // Copy pixels
        for (u16 i = 0; i < m_currentBitmap->ledCount; i++)
        {
                // Ensure we don't write past the strip length
                if (i < m_pixels->getCount())
                {
                        m_pixels->setColor(i, m_currentBitmap->data[offset + i]);
                }
        }

        // Advance frame
        m_position++;
        if (m_position >= m_currentBitmap->frameCount)
        {
                if (m_bitmapLoop)
                {
                        m_position = 0;
                }
                else
                {
                        // Stop at last frame or disable?
                        // Usually one-shot means stop at end or turn off.
                        // Let's hold the last frame.
                        m_position = m_currentBitmap->frameCount - 1;
                        // Optional: m_active = false; if we want to stop updating
                }
        }
}
//============================================================================

/************************* updateRedDotChase *******************************
 * Red dot chase with blue background.
 ***************************************************************/
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

/************************* updateRainbowCycle ******************************
 * Rainbow cycling effect across the strip.
 ***************************************************************/
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

/************************* updateBreathing *******************************
 * Simple breathing/pulse white effect.
 ***************************************************************/
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

/************************* updateSparkle **********************************
 * Random sparkle effect with single white pixel.
 ***************************************************************/
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
