#pragma once

#include "msk.h"
#include "buttons.h"
#include "ioexpander.h"

// Input event types
enum InputEvent
{
        INPUT_NONE,
        INPUT_BTN1_PRESS,
        INPUT_BTN1_LONG_PRESS,
        INPUT_KEYPAD_0,
        INPUT_KEYPAD_1,
        INPUT_KEYPAD_2,
        INPUT_KEYPAD_3,
        INPUT_KEYPAD_4,
        INPUT_KEYPAD_5,
        INPUT_KEYPAD_6,
        INPUT_KEYPAD_7,
        INPUT_KEYPAD_8,
        INPUT_KEYPAD_9,
        INPUT_KEYPAD_10,
        INPUT_KEYPAD_11,
        INPUT_KEYPAD_12,
        INPUT_KEYPAD_13,
        INPUT_KEYPAD_14,
        INPUT_KEYPAD_15
};

// Callback function type for input events
typedef void (*InputCallback)(InputEvent event);

class InputManager
{
public:
        // Constructor
        InputManager(IOExpander *ioExpander);

        // Initialize input manager
        void init();
        void poll();

        // Set callback for input events
        void setCallback(InputCallback callback);

        // Get keypad note index (0-15) for musical applications
        u8 getKeypadNote(InputEvent event) const;

private:
        IOExpander *m_ioExpander;
        InputCallback m_callback;
        bool m_btn1WasLongPress;

        void checkButtons();
        void checkKeypad();
};