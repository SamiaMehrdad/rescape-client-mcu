#ifndef SONGS_H
#define SONGS_H

#include "music.h"

// Structure to hold song data and length
struct Song
{
        const MusicNote *notes;
        u16 length;
        u8 bpm;
};

// Available Songs
extern const Song SONG_INTRO;
extern const Song SONG_SUCCESS;
extern const Song SONG_ERROR;
extern const Song SONG_COMPLEX;

#endif // SONGS_H
