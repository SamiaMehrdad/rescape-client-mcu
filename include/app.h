#pragma once

#include "msk.h"
#include "pixel.h"
#include "synth.h"
#include "animation.h"
#include "inputmanager.h"
#include "roomserial.h"
#include "colors.h"

// Application modes
enum AppMode
{
        MODE_INTERACTIVE, // User button/keypad control
        MODE_ANIMATION,   // Automated animation
        MODE_REMOTE       // Remote control via Room Bus
};

class Application
{
public:
        // Constructor
        Application(PixelStrip *pixels, Synth *synth, Animation *animation,
                    InputManager *inputManager, RoomSerial *roomBus);

        // Initialize application
        void init();

        // Main application update loop
        void update();

        // Set application mode
        void setMode(AppMode mode);
        AppMode getMode() const { return m_mode; }

private:
        // Hardware references
        PixelStrip *m_pixels;
        Synth *m_synth;
        Animation *m_animation;
        InputManager *m_inputManager;
        RoomSerial *m_roomBus;

        // Application state
        AppMode m_mode;
        int m_colorIndex;

        // Color palette - defined in app.cpp
        static const u32 kColors[];
        static const size_t kColorCount;

        // MIDI note mapping for keypad - defined in app.cpp
        static const int kNoteMap[16];

        // Event handlers
        static void onInputEvent(InputEvent event);
        void handleInputEvent(InputEvent event);
        void handleButton1Press();
        void handleButton2Press();
        void handleButtonLongPress();
        void handleKeypadPress(u8 keyIndex);
        void handleRoomBusFrame(const RoomFrame &frame);

        // Singleton instance for static callback
        static Application *s_instance;
};
