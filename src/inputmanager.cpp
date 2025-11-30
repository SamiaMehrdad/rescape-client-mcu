#include "inputmanager.h"

//============================================================================
// CONSTRUCTOR
//============================================================================

InputManager::InputManager(IOExpander *ioExpander)
    : m_ioExpander(ioExpander),
      m_callback(nullptr),
      m_lastSwitch1(false),
      m_lastSwitch2(false),
      m_btn1WasLongPress(false),
      m_btn2WasLongPress(false)
{
}

//============================================================================
// PUBLIC METHODS
//============================================================================

void InputManager::init()
{
        if (m_ioExpander->isPresent())
        {
                m_lastSwitch1 = m_ioExpander->readSwitch1();
                m_lastSwitch2 = m_ioExpander->readSwitch2();
        }

        m_btn1WasLongPress = false;
        m_btn2WasLongPress = false;
}

void InputManager::poll()
{
        // Always check buttons (no I2C needed)
        checkButtons();

        // Only poll I/O expander if present
        if (m_ioExpander->isPresent())
        {
                checkKeypad();
                checkSwitches();
        }
}

void InputManager::setCallback(InputCallback callback)
{
        m_callback = callback;
}

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

void InputManager::checkButtons()
{
        // Check for long presses first
        if (keyLongPressed(BTN1))
        {
                m_btn1WasLongPress = true;
                if (m_callback)
                        m_callback(INPUT_BTN1_LONG_PRESS);
        }

        if (keyLongPressed(BTN2))
        {
                m_btn2WasLongPress = true;
                if (m_callback)
                        m_callback(INPUT_BTN2_LONG_PRESS);
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

        if (keyReleased(BTN2))
        {
                if (!m_btn2WasLongPress && m_callback)
                {
                        m_callback(INPUT_BTN2_PRESS);
                }
                m_btn2WasLongPress = false; // Reset for next press
        }
}

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

void InputManager::checkSwitches()
{
        bool switch1 = m_ioExpander->readSwitch1();
        bool switch2 = m_ioExpander->readSwitch2();

        if (switch1 != m_lastSwitch1)
        {
                m_lastSwitch1 = switch1;
                if (m_callback)
                        m_callback(switch1 ? INPUT_SWITCH1_ON : INPUT_SWITCH1_OFF);
        }

        if (switch2 != m_lastSwitch2)
        {
                m_lastSwitch2 = switch2;
                if (m_callback)
                        m_callback(switch2 ? INPUT_SWITCH2_ON : INPUT_SWITCH2_OFF);
        }
}
