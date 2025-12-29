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

// Combined sound presets (waveform + ADSR)
enum SoundPreset
{
        SOUND_PLUCK,      // Triangle wave, short percussive (guitar, harp)
        SOUND_FLUTE,      // Sine wave, gentle attack and release (pad, strings)
        SOUND_ORGAN,      // Square wave, instant attack, sustain (organ, synth)
        SOUND_PIANO,      // Triangle wave, medium attack, natural decay
        SOUND_PERCUSSION, // Noise, very short, sharp (drum, click)
        SOUND_BEEP,       // Square wave, instant on/off (beep, alert)
        SOUND_SYNTH_LEAD, // Sawtooth, balanced envelope (lead synth)
        SOUND_DEFAULT     // Triangle wave, balanced general purpose
};

// ADSR envelope structure
struct ADSR
{
        u16 attackMs;    // Attack time in milliseconds
        u16 decayMs;     // Decay time in milliseconds
        u8 sustainLevel; // Sustain level (0-255)
        u16 releaseMs;   // Release time in milliseconds
};

// Voice structure for polyphony
struct Voice
{
        bool active;
        u16 frequency;
        u32 phaseAccumulator;
        u32 phaseIncrement;

        // ADSR State
        enum EnvState
        {
                IDLE,
                ATTACK,
                DECAY,
                SUSTAIN,
                RELEASE
        } envState;

        u32 envLevel; // Fixed point 16.16 (0..255 << 16)

        // Pre-calculated rates (fixed point 16.16 per sample)
        u32 attackRate;
        u32 decayRate;
        u32 releaseRate;
        u32 sustainLevelFixed;

        u32 samplesUntilRelease; // Counter for note duration

        u8 baseVolume;
        Waveform waveform;
};

// Number of simultaneous polyphonic voices
// WARNING: Increasing this increases ISR execution time.
// On ESP32-C3 at 40kHz sample rate:
// - 4 Channels: Safe (~30% load)
// - 8 Channels: Likely Safe (~60% load)
// - 16 Channels: RISKY (May trigger Watchdog or audio stutter)
#define NUM_CHANNELS 4

class Synth
{
private:
        u8 pin;
        u8 channel;
        u8 pin2 = 8;     // GPIO8 for secondary output (default)
        u8 channel2 = 1; // Use LEDC channel 1 for GPIO8
        u16 sampleRate;  // Samples per second
        Waveform waveform;
        ADSR envelope;

        hw_timer_t *sampleTimer;

        // Polyphonic voices
        Voice voices[NUM_CHANNELS];

        // Generate waveform sample at given phase (0-255)
        u8 generateSample(u8 phase, Waveform wave);

public:
        Synth(u8 outputPin, u8 pwmChannel);

        // Initialize synthesizer with a complete sound preset
        void init(SoundPreset preset = SOUND_DEFAULT);

        // Individual configuration methods (if needed for advanced customization)
        void begin(u16 sampleRateHz = 8000);
        void setWaveform(Waveform wave);
        void setADSR(u16 attack, u16 decay, u8 sustain, u16 release);
        void setSoundPreset(SoundPreset preset);
        void playNote(u16 freq, u16 durationMs, u8 volume = 255);
        void stopNote();
        bool isPlaying();

        // Called by timer ISR
        void updateSample();
        // Optionally allow custom pin/channel for secondary output in future
        void setSecondaryOutput(u8 pin, u8 channel);
};

#endif // SYNTH_H
#ifndef NOTES_H
#define NOTES_H

// Musical note frequencies
// Octave 3
#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247

// Octave 4
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