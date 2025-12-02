/************************* buttons.cpp *************************
 * Button Debouncing and State Management
 * Professional button handling with short and long press detection
 * Created by MSK, November 2025
 * Uses 3-read debouncing for reliable input detection
 ***************************************************************/

#include "buttons.h"

// Button state shared across modules
ButtonState buttons[NUM_BUTTONS] = {
    {false, false, false, false, false, false, 0, 0}};

// Pin assignments stored during initialization
static u8 buttonPins[NUM_BUTTONS];

// Internal helper -----------------------------------------------------------
// Applies debounce, short-press, and long-press detection to a single button.
static void updateButton(u8 index, u8 pin, u32 now)
{
        ButtonState &button = buttons[index];
        bool isPressed = digitalRead(pin) == LOW; // Inputs are active-low

        if (isPressed != button.current && (now - button.lastChangeTime) > DEBOUNCE_MS)
        {
                button.previous = button.current;
                button.current = isPressed;
                button.lastChangeTime = now;

                if (button.current && !button.previous)
                {
                        // Button transitioned from idle to pressed
                        button.pressStartTime = now;
                        button.wasLongPress = false;
                        button.pressed = true; // Mark as pressed immediately
                        button.longPressed = false;
                }
                else if (!button.current && button.previous)
                {
                        // Button transitioned from pressed to released
                        button.released = true;
                        button.pressStartTime = 0;

                        if (button.wasLongPress)
                        {
                                button.wasLongPress = false; // Prepare for next interaction
                                button.pressed = false;
                        }
                }
        }

        // Detect a long press while the button remains held
        if (button.current && !button.wasLongPress && button.pressStartTime > 0)
        {
                if ((now - button.pressStartTime) >= LONG_PRESS_MS)
                {
                        button.longPressed = true;
                        button.wasLongPress = true;
                        button.pressed = false; // Suppress the short-press event
                }
        }
}

// Public API ----------------------------------------------------------------

// Initialize button pins
void initButtons(u8 btn1Pin)
{
        buttonPins[BTN1] = btn1Pin;

        u32 now = millis();

        for (int i = 0; i < NUM_BUTTONS; i++)
        {
                pinMode(buttonPins[i], INPUT_PULLUP); // Inputs idle HIGH, pressed LOW
                bool isPressed = digitalRead(buttonPins[i]) == LOW;

                buttons[i].current = isPressed;
                buttons[i].previous = isPressed;
                buttons[i].pressed = false;
                buttons[i].released = false;
                buttons[i].longPressed = false;
                buttons[i].wasLongPress = false;
                buttons[i].lastChangeTime = now;
                buttons[i].pressStartTime = 0;
        }
}

// Update button states (call from a timer ISR or frequently in loop)
void updateButtons()
{
        u32 now = millis();
        updateButton(BTN1, buttonPins[BTN1], now);
}

// Query whether a button is currently held down (active-low)
bool keyDown(u8 btn)
{
        if (btn >= NUM_BUTTONS)
                return false;
        return buttons[btn].current;
}

// Report a single short-press event, latched until read
bool keyPressed(u8 btn)
{
        if (btn >= NUM_BUTTONS)
                return false;

        bool result = buttons[btn].pressed;
        if (result)
        {
                buttons[btn].pressed = false;
        }
        return result;
}

// Report a single release event, regardless of press duration
bool keyReleased(u8 btn)
{
        if (btn >= NUM_BUTTONS)
                return false;

        bool result = buttons[btn].released;
        if (result)
        {
                buttons[btn].released = false;
        }
        return result;
}

// Report when the long-press threshold is reached while the button is held
bool keyLongPressed(u8 btn)
{
        if (btn >= NUM_BUTTONS)
                return false;

        bool result = buttons[btn].longPressed;
        if (result)
        {
                buttons[btn].longPressed = false;
        }
        return result;
}
