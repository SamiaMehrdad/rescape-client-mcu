/************************* app_proto.cpp **********************
 * Proto Application Implementation
 * Logic for the Proto device type
 * Created by MSK, November 2025
 ***************************************************************/

#include "app_proto.h"
#include <Arduino.h>
#include "pixel.h"
#include "synth.h" // Include synth.h to access Synth class and note definitions

/************************* setup ***********************************
 * Initializes the Proto application.
 * Sets initial LED state.
 * @param context The application context.
 ***************************************************************/
void AppProto::setup(const AppContext &context)
{
        AppBase::setup(context);
        Serial.println("--- PROTOTYPING APP STARTED ---");

        // Example: Set LEDs to Red to indicate danger
        if (m_context.pixels)
        {
                m_context.pixels->setAll(0xFF0000); // Red
                m_context.pixels->show();
        }
}

/************************* loop ***********************************
 * Main loop for the Proto application.
 ***************************************************************/
void AppProto::loop()
{
        // static u32 loopCount = 0;
        // static u32 lastTime = millis();

        // loopCount++;
        // if (loopCount >= 1000)
        // {
        //         u32 now = millis();
        //         Serial.printf("Proto: Ech loop took %u ms\n", (now - lastTime) / 1000);
        //         lastTime = now;
        //         loopCount = 0;
        // }

        //  Proto logic would go here
}

/************************* handleInput ***********************************
 * Handles input events for the Proto application.
 * @param event The input event ID.
 ***************************************************************/
bool AppProto::handleInput(InputEvent event)

{
        int keyIndex = getKeypadIndex(event);
        if (keyIndex != -1)
        {
                Serial.print("Proto Key: ");
                Serial.println(keyIndex);

                // Piano on keys 0-11 (C4..B4)
                if (keyIndex >= 0 && keyIndex <= 11 && m_context.synth)
                {
                        static const u16 pianoNotes[12] = {
                            NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4,
                            NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4,
                            NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4};

                        u16 note = pianoNotes[keyIndex];
                        Serial.printf("Piano key %d -> note %u\n", keyIndex, note);
                        m_context.synth->setSoundPreset(SOUND_FLUTE);
                        m_context.synth->playNote(note, 500, 200);
                        return true;
                }

                // Percussion on keys 12-15
                if (keyIndex >= 12 && keyIndex <= 15 && m_context.synth)
                {
                        Serial.printf("Percussion key %d\n", keyIndex);
                        m_context.synth->setSoundPreset(SOUND_PERCUSSION);
                        // Map last 4 keys to different short noises
                        const u16 percFreqs[4] = {100, 200, 300, 400};
                        u16 f = percFreqs[keyIndex - 12];
                        m_context.synth->playNote(f, 120, 255);
                        return true;
                }
        }
        else
        {
                Serial.print("Proto Input: ");
                Serial.println(event);
        }
        return false; // Let Core handle default actions
}

/************************* handleCommand ***********************************
 * Handles RoomBus commands for the Proto application.
 * @param frame The received command frame.
 ***************************************************************/
void AppProto::handleCommand(const RoomFrame &frame)
{
        // Handle proto-specific commands here
}
