/************************* synth.h *****************************
 * Software synthesizer with ADSR envelope and waveform generation
 * Generates audio via PWM output with configurable parameters
 * Created by MSK, November 2025
 * Tested and finalized on ESP32-C3 11/19/25
 ***************************************************************/

#ifndef SYNTH_H
#define SYNTH_H

#include <Arduino.h>
#include "msk.h"

// Waveform types
enum Waveform
{
        WAVE_SINE,
        WAVE_SQUARE,
        WAVE_TRIANGLE,
        WAVE_SAWTOOTH,
        WAVE_NOISE
};

// ADSR envelope structure
struct ADSR
{
        u16 attackMs;    // Attack time in milliseconds
        u16 decayMs;     // Decay time in milliseconds
        u8 sustainLevel; // Sustain level (0-255)
        u16 releaseMs;   // Release time in milliseconds
};

class Synth
{
private:
        u8 pin;
        u8 channel;
        u16 sampleRate; // Samples per second
        Waveform waveform;
        ADSR envelope;

        hw_timer_t *sampleTimer;
        volatile bool playing;
        volatile u32 sampleIndex;
        volatile u32 totalSamples;
        volatile u32 noteDurationMs;
        volatile u16 frequency;
        volatile u8 baseVolume;

        // Generate waveform sample at given phase (0.0 to 1.0)
        u8 generateSample(float phase);

        // Calculate ADSR envelope amplitude (0-255) at given time
        u8 getEnvelopeAmplitude(u32 timeMs);

public:
        Synth(u8 outputPin, u8 pwmChannel);

        void begin(u16 sampleRateHz = 8000);
        void setWaveform(Waveform wave);
        void setADSR(u16 attack, u16 decay, u8 sustain, u16 release);
        void playNote(u16 freq, u16 durationMs, u8 volume = 255);
        void stopNote();
        bool isPlaying();

        // Called by timer ISR
        void updateSample();
};

#endif // SYNTH_H
#ifndef NOTES_H
#define NOTES_H

// Musical note frequencies
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494

#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988

// Octave 6
#define NOTE_C6 1047
#define NOTE_CS6 1109
#define NOTE_D6 1175
#define NOTE_DS6 1245
#define NOTE_E6 1319
#define NOTE_F6 1397
#define NOTE_FS6 1480
#define NOTE_G6 1568
#define NOTE_GS6 1661
#define NOTE_A6 1760
#define NOTE_AS6 1865
#define NOTE_B6 1976

#define REST 0

#endif // NOTES_H