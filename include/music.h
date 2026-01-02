#ifndef MUSIC_H
#define MUSIC_H

#include <Arduino.h>
#include "synth.h"

// Note duration constants (relative to 16th note)
#define DUR_16 1
#define DUR_8 2
#define DUR_4 4
#define DUR_2 8
#define DUR_1 16

struct MusicNote
{
        u16 note;           // Frequency (Hz) or NOTE_xxx
        u16 duration;       // Duration of the sound in 16th notes
        u16 advance;        // Time to wait before next note in 16th notes (0 = simultaneous/chord)
        SoundPreset preset; // Instrument to use
};

class MusicPlayer
{
private:
        Synth *synth;
        const MusicNote *currentMelody;
        u16 melodyLength;
        u16 currentNoteIndex;

        bool playing;
        u8 bpm;

        // Timing counters
        u32 samplesPerTick;     // Samples per 16th note
        u32 tickCounter;        // Counts samples for the current tick
        u32 ticksUntilNextStep; // How many ticks to wait before processing next note

        // Gap between notes for articulation (staccato/legato)
        // For now, simple full duration

public:
        MusicPlayer(Synth *s);

        // Start playing a melody
        void play(const MusicNote *melody, u16 length, u8 bpm = 120);

        // Play a predefined song
        void playSong(const struct Song &song);

        // Stop playing
        void stop();

        // Check if playing
        bool isPlaying();

        // Set Tempo
        void setBPM(u8 bpm);

        // Called by Synth ISR every sample
        // Must be IRAM_ATTR and fast!
        void IRAM_ATTR update();
};

#endif // MUSIC_H
