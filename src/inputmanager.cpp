/************************* inputmanager.cpp ********************
 * Input Manager Implementation
 * Unified input handling for buttons, keypad, and switches
 * Created by MSK, November 2025
 * Provides event-driven input system with callbacks
 ***************************************************************/

#include "inputmanager.h"

//============================================================================
// CONSTRUCTOR
//============================================================================

/************************* InputManager constructor ************************
 * Construct with IOExpander dependency.
 ***************************************************************/
InputManager::InputManager(IOExpander *ioExpander)
    : m_ioExpander(ioExpander),
      m_callback(nullptr),
      m_btn1WasLongPress(false)
{
}

//============================================================================
// PUBLIC METHODS
//============================================================================

/************************* init *******************************************
 * Initialize internal input state.
 ***************************************************************/
void InputManager::init()
{
        m_btn1WasLongPress = false;
}

/************************* poll *******************************************
 * Poll buttons and keypad (if present) and dispatch events.
 ***************************************************************/
void InputManager::poll()
{
        // Always check buttons (no I2C needed)
        checkButtons();

        // Only poll I/O expander if present
        if (m_ioExpander->isPresent())
        {
                checkKeypad();
        }
}

/************************* setCallback ************************************
 * Register input event callback.
 ***************************************************************/
void InputManager::setCallback(InputCallback callback)
{
        m_callback = callback;
}

/************************* getKeypadNote **********************************
 * Map keypad event enum to note index (0-15).
 ***************************************************************/
u8 InputManager::getKeypadNote(InputEvent event) const
{
        if (event >= INPUT_KEYPAD_0 && event <= INPUT_KEYPAD_15)
        {
                return event - INPUT_KEYPAD_0;
        }
        return 0;
}

//============================================================================
// INPUT POLLING METHODS
//============================================================================

/************************* checkButtons ***********************************
 * Poll BTN1 for short/long presses and fire callbacks.
 ***************************************************************/
void InputManager::checkButtons()
{
        // Check for long presses first
        if (keyLongPressed(BTN1))
        {
                m_btn1WasLongPress = true;
                if (m_callback)
                        m_callback(INPUT_BTN1_LONG_PRESS);
        }

        // Check for short presses on button RELEASE (not press)
        // Only trigger if it wasn't a long press
        if (keyReleased(BTN1))
        {
                if (!m_btn1WasLongPress && m_callback)
                {
                        m_callback(INPUT_BTN1_PRESS);
                }
                m_btn1WasLongPress = false; // Reset for next press
        }
}

/************************* checkKeypad ************************************
 * Scan keypad via IOExpander and fire events.
 ***************************************************************/
void InputManager::checkKeypad()
{
        u8 keyIndex = m_ioExpander->scanKeypad();
        if (keyIndex != 255 && keyIndex <= 15)
        {
                InputEvent event = static_cast<InputEvent>(INPUT_KEYPAD_0 + keyIndex);
                if (m_callback)
                        m_callback(event);
        }
}
