/************************* buttons.h ***************************
 * Debounced button input handling with short/long press detection
 * Manages button state transitions and event flags
 * Created by MSK, November 2025
 * Tested and finalized on ESP32-C3 11/19/25
 ***************************************************************/

#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>
#include "msk.h"

// Button definitions
#define BTN1 0
#define BTN2 1
#define NUM_BUTTONS 2
#define DEBOUNCE_MS 50
#define LONG_PRESS_MS 1000 // Time for long press detection

// Button state structure
struct ButtonState
{
        bool current;
        bool previous;
        bool pressed;      // True for one cycle when button is first pressed
        bool released;     // True for one cycle when button is released
        bool longPressed;  // True for one cycle when long press threshold is reached
        bool wasLongPress; // Flag to prevent normal press after long press
        u32 lastChangeTime;
        u32 pressStartTime; // When the button was first pressed
};

// Global button state array
extern ButtonState buttons[NUM_BUTTONS];

// Function prototypes
void initButtons(u8 btn1Pin, u8 btn2Pin);
void updateButtons();
bool keyDown(u8 btn);
bool keyPressed(u8 btn);
bool keyReleased(u8 btn);
bool keyLongPressed(u8 btn);

#endif // BUTTONS_H
