/************************* synth.cpp ***************************
 * Audio Synthesizer Implementation
 * PWM-based audio synthesis with ADSR envelope
 * Created by MSK, November 2025
 * Supports multiple waveforms and sound presets
 ***************************************************************/

#include "synth.h"
#include <math.h>
#include <algorithm>

//============================================================================
// CONFIGURATION
//============================================================================

// Sound preset lookup table [preset][waveform, attack, decay, sustain, release]
static const u16 SOUND_PRESETS[][5] = {
    {WAVE_TRIANGLE, 5, 80, 50, 120},    // SOUND_PLUCK: Triangle, quick percussive
    {WAVE_SINE, 150, 200, 180, 300},    // SOUND_SOFT_PAD: Sine, gentle pad
    {WAVE_SQUARE, 0, 0, 255, 50},       // SOUND_ORGAN: Square, instant sustain
    {WAVE_TRIANGLE, 10, 150, 120, 200}, // SOUND_PIANO: Triangle, natural decay
    {WAVE_NOISE, 1, 30, 0, 50},         // SOUND_PERCUSSION: Noise, very short
    {WAVE_SQUARE, 0, 5, 200, 10},       // SOUND_BEEP: Square, instant beep
    {WAVE_SAWTOOTH, 20, 100, 150, 150}, // SOUND_SYNTH_LEAD: Sawtooth, balanced
    {WAVE_TRIANGLE, 5, 120, 100, 180}   // SOUND_DEFAULT: Triangle, general purpose
};

//============================================================================
// ISR AND GLOBAL INSTANCE
//============================================================================

static Synth *synthInstance = nullptr;

/************************* sampleTimerISR *********************************
 * Timer ISR: dispatch to active Synth instance.
 ***************************************************************/
void IRAM_ATTR sampleTimerISR()
{
        if (synthInstance)
        {
                synthInstance->updateSample();
        }
}

//============================================================================
// CONSTRUCTOR
//============================================================================

/************************* Synth constructor ******************************
 * Construct Synth with output pin and PWM channel.
 ***************************************************************/
Synth::Synth(u8 outputPin, u8 pwmChannel)
    : pin(outputPin), channel(pwmChannel), sampleRate(8000),
      waveform(WAVE_SINE), playing(false), sampleIndex(0), noteDurationMs(0)
{

        // Default ADSR envelope
        envelope.attackMs = 10;
        envelope.decayMs = 50;
        envelope.sustainLevel = 200;
        envelope.releaseMs = 100;

        synthInstance = this;
}

//============================================================================
// PUBLIC METHODS
//============================================================================

/************************* init *******************************************
 * Initialize synth with default sample rate and preset.
 ***************************************************************/
void Synth::init(SoundPreset preset)
{
        begin(20000); // Fixed 20kHz sample rate for optimal quality
        setSoundPreset(preset);
}

/************************* setSoundPreset *********************************
 * Apply a preset waveform and ADSR envelope.
 ***************************************************************/
void Synth::setSoundPreset(SoundPreset preset)
{
        const u16 *params = SOUND_PRESETS[preset];
        setWaveform(static_cast<Waveform>(params[0]));
        setADSR(params[1], params[2], params[3], params[4]);
}

/************************* begin *******************************************
 * Configure PWM and timer for given sample rate.
 ***************************************************************/
void Synth::begin(u16 sampleRateHz)
{
        sampleRate = sampleRateHz;

        // Setup PWM for audio output
        ledcSetup(channel, sampleRate, 8); // 8-bit resolution
        ledcAttachPin(pin, channel);

        // Setup sample timer (generates samples at sampleRate Hz)
        sampleTimer = timerBegin(1, 80, true); // Timer 1, prescaler 80 (1MHz)
        timerAttachInterrupt(sampleTimer, &sampleTimerISR, true);
        timerAlarmWrite(sampleTimer, 1000000 / sampleRate, true); // Period in microseconds
        timerAlarmEnable(sampleTimer);
}

/************************* setWaveform ************************************
 * Select output waveform.
 ***************************************************************/
void Synth::setWaveform(Waveform wave)
{
        waveform = wave;
}

/************************* setADSR ***************************************
 * Configure envelope attack/decay/sustain/release.
 ***************************************************************/
void Synth::setADSR(u16 attack, u16 decay, u8 sustain, u16 release)
{
        envelope.attackMs = attack;
        envelope.decayMs = decay;
        envelope.sustainLevel = std::min<u8>(255, sustain);
        envelope.releaseMs = release;
}

//============================================================================
// WAVEFORM GENERATION
//============================================================================

/************************* generateSample *********************************
 * Generate one waveform sample for a phase [0,1).
 ***************************************************************/
u8 Synth::generateSample(float phase)
{
        u8 sample = 128; // Center value

        switch (waveform)
        {
        case WAVE_SINE:
                sample = (u8)(128 + 127 * sin(2 * PI * phase));
                break;

        case WAVE_SQUARE:
                sample = (phase < 0.5) ? 255 : 0;
                break;

        case WAVE_TRIANGLE:
                if (phase < 0.5)
                {
                        sample = (u8)(phase * 4 * 255);
                }
                else
                {
                        sample = (u8)(255 - (phase - 0.5) * 4 * 255);
                }
                break;

        case WAVE_SAWTOOTH:
                sample = (u8)(phase * 255);
                break;

        case WAVE_NOISE:
                sample = random(0, 256);
                break;
        }

        return sample;
}

//============================================================================
// ADSR ENVELOPE
//============================================================================

/************************* getEnvelopeAmplitude ***************************
 * Compute ADSR amplitude at time in ms.
 ***************************************************************/
u8 Synth::getEnvelopeAmplitude(u32 timeMs)
{
        const u32 attackMs = std::max<u16>(1, envelope.attackMs);
        const u32 decayMs = std::max<u16>(1, envelope.decayMs);
        const u32 attackEnd = attackMs;
        const u32 decayEnd = attackEnd + decayMs;
        const u32 sustainLevel = envelope.sustainLevel;

        const u32 sustainStart = decayEnd;
        const u32 durationMs = noteDurationMs;
        const u32 releaseStart = std::max(durationMs, sustainStart);
        const u32 releaseDuration = envelope.releaseMs;
        const u32 releaseDivisor = releaseDuration == 0 ? 1 : releaseDuration;
        const u32 releaseEnd = releaseStart + releaseDuration;

        if (timeMs < attackEnd)
        {
                return (u8)((255u * timeMs) / attackMs);
        }
        if (timeMs < decayEnd)
        {
                u32 decayTime = timeMs - attackEnd;
                return (u8)(255u - ((255u - sustainLevel) * decayTime) / decayMs);
        }
        if (timeMs < releaseStart)
        {
                return sustainLevel;
        }
        if (releaseDuration == 0 || timeMs >= releaseEnd)
        {
                return 0;
        }

        u32 releaseTime = timeMs - releaseStart;
        u8 remaining = sustainLevel - ((sustainLevel * releaseTime) / releaseDivisor);
        return remaining;
}

/************************* playNote **************************************
 * Start a note with frequency, duration, and base volume.
 ***************************************************************/
void Synth::playNote(u16 freq, u16 durationMs, u8 volume)
{
        frequency = freq;
        baseVolume = volume;
        noteDurationMs = durationMs;
        u32 totalDurationMs = durationMs + envelope.releaseMs;
        totalSamples = (sampleRate * totalDurationMs) / 1000;
        if (totalSamples == 0)
        {
                totalSamples = 1;
        }
        sampleIndex = 0;
        playing = true;
}

/************************* stopNote **************************************
 * Stop playback immediately.
 ***************************************************************/
void Synth::stopNote()
{
        playing = false;
        ledcWrite(channel, 128); // Center value (silence)
}

/************************* isPlaying *************************************
 * Check whether a note is active.
 ***************************************************************/
bool Synth::isPlaying()
{
        return playing;
}

//============================================================================
// SAMPLE GENERATION (ISR)
//============================================================================

/************************* updateSample ***********************************
 * ISR: generate and output next audio sample.
 ***************************************************************/
void IRAM_ATTR Synth::updateSample()
{
        if (!playing)
        {
                return;
        }

        if (sampleIndex >= totalSamples)
        {
                stopNote();
                return;
        }

        // Calculate current phase in waveform (0.0 to 1.0)
        float phase = fmod((float)(sampleIndex * frequency) / sampleRate, 1.0);

        // Generate waveform sample
        u8 waveValue = generateSample(phase);

        // Apply ADSR envelope
        u32 timeMs = (sampleIndex * 1000) / sampleRate;
        u8 envelopeAmp = getEnvelopeAmplitude(timeMs);

        // Apply volume and envelope
        u16 finalValue = ((waveValue - 128) * baseVolume * envelopeAmp) / (255 * 255) + 128;

        // Output to PWM
        ledcWrite(channel, finalValue);

        sampleIndex++;
}