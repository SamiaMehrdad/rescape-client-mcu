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

// --- 4. Drum & Guitar Demo Tune ---
static const MusicNote NOTES_COMPLEX[] = {
    // Bar 1: Drum kick pattern + Guitar riff (C)
    {NOTE_C2, DUR_8, DUR_8, SOUND_DRUM},  // Kick drum C2
    {NOTE_C4, DUR_8, DUR_8, SOUND_PLUCK}, // Guitar C4
    {NOTE_C2, DUR_8, DUR_8, SOUND_DRUM},  // Kick drum
    {NOTE_E4, DUR_8, DUR_8, SOUND_PLUCK}, // Guitar E4
    {NOTE_C2, DUR_8, DUR_8, SOUND_DRUM},  // Kick drum
    {NOTE_G4, DUR_8, DUR_8, SOUND_PLUCK}, // Guitar G4
    {NOTE_C2, DUR_8, DUR_8, SOUND_DRUM},  // Kick drum
    {NOTE_C5, DUR_8, DUR_8, SOUND_PLUCK}, // Guitar C5

    // Bar 2: Drum kick pattern + Guitar riff (F)
    {NOTE_F2, DUR_8, DUR_8, SOUND_DRUM},  // Kick drum F2
    {NOTE_F4, DUR_8, DUR_8, SOUND_PLUCK}, // Guitar F4
    {NOTE_F2, DUR_8, DUR_8, SOUND_DRUM},  // Kick drum
    {NOTE_A4, DUR_8, DUR_8, SOUND_PLUCK}, // Guitar A4
    {NOTE_F2, DUR_8, DUR_8, SOUND_DRUM},  // Kick drum
    {NOTE_C5, DUR_8, DUR_8, SOUND_PLUCK}, // Guitar C5
    {NOTE_F2, DUR_8, DUR_8, SOUND_DRUM},  // Kick drum
    {NOTE_F5, DUR_8, DUR_8, SOUND_PLUCK}, // Guitar F5

    // Bar 3: Drum kick pattern + Guitar riff (G)
    {NOTE_G2, DUR_8, DUR_8, SOUND_DRUM},  // Kick drum G2
    {NOTE_G4, DUR_8, DUR_8, SOUND_PLUCK}, // Guitar G4
    {NOTE_G2, DUR_8, DUR_8, SOUND_DRUM},  // Kick drum
    {NOTE_B4, DUR_8, DUR_8, SOUND_PLUCK}, // Guitar B4
    {NOTE_G2, DUR_8, DUR_8, SOUND_DRUM},  // Kick drum
    {NOTE_D5, DUR_8, DUR_8, SOUND_PLUCK}, // Guitar D5
    {NOTE_G2, DUR_8, DUR_8, SOUND_DRUM},  // Kick drum
    {NOTE_G5, DUR_8, DUR_8, SOUND_PLUCK}, // Guitar G5

    // Bar 4: Finale - All together (C Major)
    {NOTE_C2, DUR_4, 0, SOUND_DRUM},  // Kick drum
    {NOTE_C4, DUR_4, 0, SOUND_PLUCK}, // Guitar chord (simultaneous)
    {NOTE_E4, DUR_4, 0, SOUND_PLUCK},
    {NOTE_G4, DUR_4, 0, SOUND_PLUCK},
    {NOTE_C2, DUR_4, 0, SOUND_DRUM},  // Kick drum
    {NOTE_C4, DUR_4, 0, SOUND_PLUCK}, // Guitar chord
    {NOTE_E4, DUR_4, 0, SOUND_PLUCK},
    {NOTE_G4, DUR_4, DUR_4, SOUND_PLUCK},

    {0, DUR_4, DUR_4, SOUND_DEFAULT} // Rest
};
const Song SONG_COMPLEX = {NOTES_COMPLEX, sizeof(NOTES_COMPLEX) / sizeof(MusicNote), 120};
