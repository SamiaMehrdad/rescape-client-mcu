#include "songs.h"

// --- 1. Intro / Startup Sound ---
static const MusicNote NOTES_INTRO[] = {
    {NOTE_C4, DUR_8, DUR_8, SOUND_PLUCK},
    {NOTE_E4, DUR_8, DUR_8, SOUND_PLUCK},
    {NOTE_G4, DUR_8, DUR_8, SOUND_PLUCK},
    {NOTE_C5, DUR_2, DUR_2, SOUND_FLUTE}};
const Song SONG_INTRO = {NOTES_INTRO, sizeof(NOTES_INTRO) / sizeof(MusicNote), 120};

// --- 2. Success / Win Sound ---
static const MusicNote NOTES_SUCCESS[] = {
    {NOTE_C5, DUR_16, DUR_16, SOUND_SYNTH_LEAD},
    {NOTE_E5, DUR_16, DUR_16, SOUND_SYNTH_LEAD},
    {NOTE_G5, DUR_16, DUR_16, SOUND_SYNTH_LEAD},
    {NOTE_C6, DUR_4, DUR_4, SOUND_SYNTH_LEAD},
    {0, DUR_4, 0, SOUND_DEFAULT},     // Rest
    {NOTE_C4, DUR_2, 0, SOUND_PIANO}, // Chord
    {NOTE_E4, DUR_2, 0, SOUND_PIANO},
    {NOTE_G4, DUR_2, DUR_2, SOUND_PIANO}};
const Song SONG_SUCCESS = {NOTES_SUCCESS, sizeof(NOTES_SUCCESS) / sizeof(MusicNote), 150};

// --- 3. Error / Fail Sound ---
static const MusicNote NOTES_ERROR[] = {
    {NOTE_G3, DUR_4, DUR_4, SOUND_ORGAN},
    {NOTE_CS3, DUR_2, DUR_2, SOUND_ORGAN}};
const Song SONG_ERROR = {NOTES_ERROR, sizeof(NOTES_ERROR) / sizeof(MusicNote), 100};

// --- 4. Complex Demo Tune ---
static const MusicNote NOTES_COMPLEX[] = {
    // Bar 1: C Major Chord (C3, E3, G3) + Melody C5 ascending
    {NOTE_C3, DUR_1, 0, SOUND_PIANO},     // Bass C3 (Whole note), simultaneous
    {NOTE_E3, DUR_1, 0, SOUND_PIANO},     // Chord E3 (Whole note), simultaneous
    {NOTE_G3, DUR_1, 0, SOUND_PIANO},     // Chord G3 (Whole note), simultaneous
    {NOTE_C5, DUR_4, DUR_4, SOUND_FLUTE}, // Melody C5 (Quarter), advance 4

    {NOTE_D5, DUR_4, DUR_4, SOUND_FLUTE}, // Melody D5
    {NOTE_E5, DUR_4, DUR_4, SOUND_FLUTE}, // Melody E5
    {NOTE_F5, DUR_4, DUR_4, SOUND_FLUTE}, // Melody F5

    // Bar 2: G Major Chord (G2, B2, D3) + Melody G5 descending
    {NOTE_G2, DUR_1, 0, SOUND_PIANO},     // Bass G2
    {NOTE_B2, DUR_1, 0, SOUND_PIANO},     // Chord B2
    {NOTE_D3, DUR_1, 0, SOUND_PIANO},     // Chord D3
    {NOTE_G5, DUR_4, DUR_4, SOUND_FLUTE}, // Melody G5

    {NOTE_F5, DUR_4, DUR_4, SOUND_FLUTE}, // Melody F5
    {NOTE_E5, DUR_4, DUR_4, SOUND_FLUTE}, // Melody E5
    {NOTE_D5, DUR_4, DUR_4, SOUND_FLUTE}, // Melody D5

    // Bar 3: A Minor Chord (A2, C3, E3) + Arpeggio
    {NOTE_A2, DUR_1, 0, SOUND_PIANO},     // Bass A2
    {NOTE_C3, DUR_1, 0, SOUND_PIANO},     // Chord C3
    {NOTE_E3, DUR_1, 0, SOUND_PIANO},     // Chord E3
    {NOTE_C6, DUR_8, DUR_8, SOUND_FLUTE}, // Melody C6 (Eighth)
    {NOTE_B5, DUR_8, DUR_8, SOUND_FLUTE},
    {NOTE_A5, DUR_8, DUR_8, SOUND_FLUTE},
    {NOTE_G5, DUR_8, DUR_8, SOUND_FLUTE},
    {NOTE_F5, DUR_8, DUR_8, SOUND_FLUTE},
    {NOTE_E5, DUR_8, DUR_8, SOUND_FLUTE},
    {NOTE_D5, DUR_8, DUR_8, SOUND_FLUTE},
    {NOTE_C5, DUR_8, DUR_8, SOUND_FLUTE},

    // Final Chord: C Major
    {NOTE_C3, DUR_2, 0, SOUND_PIANO},
    {NOTE_E3, DUR_2, 0, SOUND_PIANO},
    {NOTE_G3, DUR_2, 0, SOUND_PIANO},
    {NOTE_C4, DUR_2, DUR_2, SOUND_PIANO},

    {0, DUR_4, DUR_4, SOUND_DEFAULT} // Rest
};
const Song SONG_COMPLEX = {NOTES_COMPLEX, sizeof(NOTES_COMPLEX) / sizeof(MusicNote), 100};
