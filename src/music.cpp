#include "music.h"
#include "songs.h"

MusicPlayer::MusicPlayer(Synth *s) : synth(s), playing(false), bpm(120)
{
        currentMelody = nullptr;
        melodyLength = 0;
        currentNoteIndex = 0;
        samplesPerTick = 0;
        msPerTick = 0;
        tickCounter = 0;
        ticksUntilNextStep = 0;
}

void MusicPlayer::play(const MusicNote *melody, u16 length, u8 newBpm)
{
        currentMelody = melody;
        melodyLength = length;
        currentNoteIndex = 0;
        setBPM(newBpm);

        // Reset counters
        // Force immediate update on next sample
        tickCounter = samplesPerTick;
        ticksUntilNextStep = 0;

        if (melodyLength > 0 && synth)
        {
                playing = true;
        }
}

void MusicPlayer::playSong(const Song &song)
{
        play(song.notes, song.length, song.bpm);
}

void MusicPlayer::stop()
{
        playing = false;
        if (synth)
        {
                synth->stopNote();
        }
}

bool MusicPlayer::isPlaying()
{
        return playing;
}

void MusicPlayer::setBPM(u8 newBpm)
{
        bpm = newBpm;
        // Calculate samples per 16th note
        // Samples per minute = sampleRate * 60
        // Beats per minute = BPM
        // 16th notes per minute = BPM * 4
        // Samples per 16th note = (sampleRate * 60) / (BPM * 4) = (sampleRate * 15) / BPM

        // Get actual sample rate from Synth
        u32 sampleRate = synth ? synth->getSampleRate() : 40000;
        samplesPerTick = (sampleRate * 15) / bpm;

        // Pre-calculate ms per tick to avoid division in ISR
        msPerTick = (15000 + (bpm >> 1)) / bpm; // +rounding
}

// This runs in ISR context!
void IRAM_ATTR MusicPlayer::update()
{
        if (!playing || !currentMelody)
                return;

        tickCounter++;

        // Check if a 16th note tick has passed
        if (tickCounter >= samplesPerTick)
        {
                tickCounter = 0;

                // Time to advance sequencer?
                if (ticksUntilNextStep > 0)
                {
                        ticksUntilNextStep--;
                }

                // Process notes as long as we are playing and there is no wait time
                while (ticksUntilNextStep == 0 && playing)
                {
                        if (currentNoteIndex >= melodyLength)
                        {
                                playing = false;
                                break;
                        }

                        MusicNote note = currentMelody[currentNoteIndex];

                        // Play the note
                        if (note.note > 0)
                        {
                                synth->setSoundPreset(note.preset);
                                // Use pre-calculated msPerTick instead of dividing in ISR
                                u32 durationMs = (u32)note.duration * msPerTick;
                                synth->playNote(note.note, durationMs);
                        }

                        // Setup wait for next step
                        ticksUntilNextStep = note.advance;

                        // Move to next note
                        currentNoteIndex++;

                        // If advance is > 0, the loop condition (ticksUntilNextStep == 0) will fail
                        // and we will exit, waiting for the next ticks.
                        // If advance is 0, we loop again immediately (chord/simultaneous note)
                }
        }
}
