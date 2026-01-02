/************************* synth.cpp ***************************
 * Audio Synthesizer Implementation
 * PWM-based audio synthesis with ADSR envelope
 * Created by MSK, November 2025
 * Supports multiple waveforms and sound presets
 ***************************************************************/

#include "synth.h"
#include "music.h" // Include for MusicPlayer definition
#include <math.h>
#include <algorithm>

//============================================================================
// CONFIGURATION
//============================================================================

// Sound preset lookup table [preset][waveform, attack, decay, sustain, release]
static const u16 SOUND_PRESETS[][5] = {
    {WAVE_TRIANGLE, 5, 80, 50, 120},    // SOUND_PLUCK: Triangle, quick percussive
    {WAVE_SINE, 150, 200, 180, 300},    // SOUND_FLUTE: Sine, gentle pad
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
    : pin(outputPin), channel(pwmChannel), sampleRate(8000), waveform(WAVE_SINE),
      presetEchoEnabled(false), delayBuffer(nullptr), delayBufferLen(0), delayWriteIndex(0),
      musicPlayer(nullptr)
{
        // Default ADSR envelope
        envelope.attackMs = 10;
        envelope.decayMs = 50;
        envelope.sustainLevel = 200;
        envelope.releaseMs = 100;

        // Default Echo
        echo.globalEnabled = false;
        echo.delayMs = 300;
        echo.feedback = 100;
        echo.mix = 100;

        // Initialize voices
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
                voices[i].active = false;
                voices[i].enableEcho = false;
                voices[i].envState = Voice::IDLE;
        }

        synthInstance = this;
}

Synth::~Synth()
{
        if (delayBuffer)
        {
                delete[] delayBuffer;
                delayBuffer = nullptr;
        }
}

void Synth::setSecondaryOutput(u8 p, u8 ch)
{
        pin2 = p;
        channel2 = ch;
}

void Synth::setMusicPlayer(MusicPlayer *player)
{
        musicPlayer = player;
}

//============================================================================
// PUBLIC METHODS
//============================================================================

/************************* init *******************************************
 * Initialize synth with default sample rate and preset.
 ***************************************************************/
void Synth::init(SoundPreset preset)
{
        begin(40000); // 40kHz sample rate for higher quality
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

        // Configure Echo Property for this sound
        switch (preset)
        {
        case SOUND_PLUCK:
        case SOUND_FLUTE:
        case SOUND_SYNTH_LEAD:
                presetEchoEnabled = true;
                break;
        case SOUND_PERCUSSION:
        case SOUND_BEEP:
        default:
                presetEchoEnabled = false;
                break;
        }
}

/************************* begin *******************************************
 * Configure PWM and timer for given sample rate.
 ***************************************************************/
void Synth::begin(u16 sampleRateHz)
{
        sampleRate = sampleRateHz;

        // Allocate delay buffer
        if (delayBuffer != nullptr)
                delete[] delayBuffer;
        // Calculate buffer size needed for max delay (e.g. 600ms) or fixed size
        delayBufferLen = MAX_DELAY_BUFFER_SIZE;
        delayBuffer = new u8[delayBufferLen];
        memset(delayBuffer, 128, delayBufferLen); // Fill with silence (128 for 8-bit audio)
        delayWriteIndex = 0;

        // Setup PWM for audio output (main)
        ledcSetup(channel, sampleRate * 3, 8); // 8-bit resolution
        ledcAttachPin(pin, channel);

        // Setup PWM for secondary output (GPIO8, channel2)
        ledcSetup(channel2, sampleRate * 3, 8);
        ledcAttachPin(pin2, channel2);

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

/************************* setEcho ***************************************
 * Configure Echo Effect parameters.
 ***************************************************************/
void Synth::setEcho(bool enabled, u16 delayMs, u8 feedback, u8 mix)
{
        echo.globalEnabled = enabled;
        echo.delayMs = delayMs;
        echo.feedback = feedback;
        echo.mix = mix;
}

//============================================================================
// WAVEFORM GENERATION
//============================================================================

// Pre-calculated Sine Table (256 values)
static const u8 SINE_TABLE[256] = {
    128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173,
    176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215,
    218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244,
    245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255,
    255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 246,
    245, 244, 243, 241, 240, 238, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220,
    218, 215, 213, 211, 208, 206, 203, 201, 198, 196, 193, 190, 188, 185, 182, 179,
    176, 173, 170, 167, 165, 162, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131,
    128, 124, 121, 118, 115, 112, 109, 106, 103, 100, 97, 93, 90, 88, 85, 82,
    79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40,
    37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11,
    10, 9, 7, 6, 5, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 5, 6, 7, 9,
    10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35,
    37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76,
    79, 82, 85, 88, 90, 93, 97, 100, 103, 106, 109, 112, 115, 118, 121, 124};

/************************* generateSample *********************************
 * Generate one waveform sample for a phase [0, 255].
 * Optimized for integer math.
 ***************************************************************/
u8 Synth::generateSample(u8 phase, Waveform wave)
{
        u8 sample = 128; // Center value

        switch (wave)
        {
        case WAVE_SINE:
                sample = SINE_TABLE[phase];
                break;

        case WAVE_SQUARE:
                sample = (phase < 128) ? 255 : 0;
                break;

        case WAVE_TRIANGLE:
                // 0-127: Rise from 0 to 255
                // 128-255: Fall from 255 to 0
                if (phase < 128)
                {
                        sample = phase * 2;
                }
                else
                {
                        sample = (255 - phase) * 2;
                }
                break;

        case WAVE_SAWTOOTH:
                sample = phase;
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

/************************* playNote **************************************
 * Start a note with frequency, duration, and base volume.
 * Finds a free voice or steals the oldest one (simple round-robin for now).
 ***************************************************************/
void Synth::playNote(u16 freq, u16 durationMs, u8 volume)
{
        // Find free voice
        int voiceIndex = -1;
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
                if (!voices[i].active)
                {
                        voiceIndex = i;
                        break;
                }
        }

        // If no free voice, steal one (simple strategy: just take 0 for now, or round robin)
        // Better: steal the one in RELEASE state, or oldest.
        if (voiceIndex == -1)
        {
                // Simple stealing: find first one in RELEASE
                for (int i = 0; i < NUM_CHANNELS; i++)
                {
                        if (voices[i].envState == Voice::RELEASE)
                        {
                                voiceIndex = i;
                                break;
                        }
                }
                // If still none, steal 0
                if (voiceIndex == -1)
                        voiceIndex = 0;
        }

        Voice &v = voices[voiceIndex];
        v.active = true;
        v.enableEcho = presetEchoEnabled;
        v.frequency = freq;
        v.baseVolume = volume;
        v.waveform = waveform; // Use current global waveform setting
        v.phaseAccumulator = 0;
        // Use 64-bit math to compute phase increment for a 32-bit accumulator.
        // Desired: phaseIncrement = freq * 2^32 / sampleRate
        v.phaseIncrement = (u32)(((uint64_t)freq << 32) / sampleRate);

        // Setup ADSR
        v.envState = Voice::ATTACK;
        v.envLevel = 0;
        v.samplesUntilRelease = (durationMs * sampleRate) / 1000;

        // Calculate rates (fixed point 16.16)
        // Max level is 255 << 16
        u32 maxLevel = 255 << 16;

        // Attack Rate: How much to add per sample
        u32 attackSamples = (envelope.attackMs * sampleRate) / 1000;
        if (attackSamples == 0)
                attackSamples = 1;
        v.attackRate = maxLevel / attackSamples;

        // Decay Rate: How much to subtract per sample
        u32 decaySamples = (envelope.decayMs * sampleRate) / 1000;
        if (decaySamples == 0)
                decaySamples = 1;
        v.sustainLevelFixed = (u32)envelope.sustainLevel << 16;
        if (maxLevel > v.sustainLevelFixed)
                v.decayRate = (maxLevel - v.sustainLevelFixed) / decaySamples;
        else
                v.decayRate = 0; // Should not happen if sustain <= 255

        // Release Rate: How much to subtract per sample
        u32 releaseSamples = (envelope.releaseMs * sampleRate) / 1000;
        if (releaseSamples == 0)
                releaseSamples = 1;
        v.releaseRate = v.sustainLevelFixed / releaseSamples;
}

/************************* stopNote **************************************
 * Stop all playback immediately (panic button).
 ***************************************************************/
void Synth::stopNote()
{
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
                voices[i].active = false;
                voices[i].envState = Voice::IDLE;
        }
        ledcWrite(channel, 128); // Center value (silence)
}

/************************* isPlaying *************************************
 * Check whether any note is active.
 ***************************************************************/
bool Synth::isPlaying()
{
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
                if (voices[i].active)
                        return true;
        }
        return false;
}

//============================================================================
// SAMPLE GENERATION (ISR)
//============================================================================

/************************* updateSample ***********************************
 * ISR: generate and output next audio sample.
 * Optimized for integer math (no float).
 * Polyphonic mixing.
 ***************************************************************/
void IRAM_ATTR Synth::updateSample()
{
        // --- 0. Update Music Player ---
        if (musicPlayer)
        {
                musicPlayer->update();
        }

        int32_t mixedSample = 0;
        int32_t echoSendSample = 0;
        int activeVoices = 0;

        for (int i = 0; i < NUM_CHANNELS; i++)
        {
                Voice &v = voices[i];
                if (!v.active)
                        continue;

                activeVoices++;

                // --- 1. Update Phase (DDS) ---
                v.phaseAccumulator += v.phaseIncrement;
                u8 phase = v.phaseAccumulator >> 24; // Top 8 bits

                // --- 2. Generate Waveform ---
                u8 waveValue = generateSample(phase, v.waveform);

                // --- 3. Update ADSR Envelope ---
                switch (v.envState)
                {
                case Voice::ATTACK:
                        v.envLevel += v.attackRate;
                        if (v.envLevel >= (255 << 16))
                        {
                                v.envLevel = (255 << 16);
                                v.envState = Voice::DECAY;
                        }
                        break;

                case Voice::DECAY:
                        if (v.envLevel > v.sustainLevelFixed + v.decayRate)
                                v.envLevel -= v.decayRate;
                        else
                                v.envLevel = v.sustainLevelFixed;

                        // Check if we reached sustain level
                        if (v.envLevel <= v.sustainLevelFixed)
                        {
                                v.envLevel = v.sustainLevelFixed;
                                v.envState = Voice::SUSTAIN;
                        }
                        break;

                case Voice::SUSTAIN:
                        // Hold level
                        if (v.samplesUntilRelease > 0)
                        {
                                v.samplesUntilRelease--;
                        }
                        else
                        {
                                v.envState = Voice::RELEASE;
                        }
                        break;

                case Voice::RELEASE:
                        if (v.envLevel > v.releaseRate)
                                v.envLevel -= v.releaseRate;
                        else
                        {
                                v.envLevel = 0;
                                v.active = false; // Note finished
                                v.envState = Voice::IDLE;
                        }
                        break;

                case Voice::IDLE:
                        v.active = false;
                        break;
                }

                // --- 4. Apply Volume & Envelope ---
                // waveValue is 0..255. Center is 128.
                // Convert to signed -128..127
                int32_t sampleSigned = (int32_t)waveValue - 128;

                // Apply Envelope (16.16 fixed point -> 0..255)
                u32 envAmp = v.envLevel >> 16; // 0..255

                // Apply Base Volume (0..255)
                // Result = sample * env * vol
                // Scale: sample(8bit) * env(8bit) * vol(8bit) = 24 bits approx
                // We need to shift back down to 8 bits range (-128..127)
                // Divisor = 255 * 255 = 65025 (approx 16 bits)

                int32_t processedSample = (sampleSigned * (int32_t)envAmp * (int32_t)v.baseVolume) >> 16;

                // Accumulate
                mixedSample += processedSample;

                // Layer 1: Echo Send
                if (v.enableEcho)
                {
                        echoSendSample += processedSample;
                }
        }

        // --- Layer 2: Echo Processing ---
        if (echo.globalEnabled && delayBuffer != nullptr)
        {
                // Calculate read position based on delayMs
                // delayMs * sampleRate / 1000
                u32 delaySamples = ((u32)echo.delayMs * sampleRate) / 1000;
                if (delaySamples > delayBufferLen)
                        delaySamples = delayBufferLen;
                if (delaySamples == 0)
                        delaySamples = 1; // Avoid reading same sample

                int32_t readIndex = (int32_t)delayWriteIndex - (int32_t)delaySamples;
                if (readIndex < 0)
                        readIndex += delayBufferLen;

                // Read from delay line (convert 0..255 to -128..127)
                int32_t delayedSignal = (int32_t)delayBuffer[readIndex] - 128;

                // Calculate Feedback: Input + (Delayed * Feedback)
                // Feedback is 0..255 (representing 0.0 to ~1.0)
                int32_t feedbackSignal = echoSendSample + ((delayedSignal * echo.feedback) >> 8);

                // Clip and write back to buffer (convert back to 0..255)
                if (feedbackSignal > 127)
                        feedbackSignal = 127;
                if (feedbackSignal < -128)
                        feedbackSignal = -128;
                delayBuffer[delayWriteIndex] = (u8)(feedbackSignal + 128);

                // Advance buffer
                delayWriteIndex++;
                if (delayWriteIndex >= delayBufferLen)
                        delayWriteIndex = 0;

                // Mix Echo into Total: Dry + (Delayed * Mix)
                // Mix is 0..255
                mixedSample += (delayedSignal * echo.mix) >> 8;
        }

        // --- 5. Final Mix & Output ---
        static bool pwmActive = false;
        // Keep active if voices are playing OR echo is enabled (to hear tails)
        if (activeVoices > 0 || (echo.globalEnabled && delayBuffer != nullptr))
        {
                // If PWM was stopped, reattach
                bool justAttached = false;
                if (!pwmActive)
                {
                        ledcAttachPin(pin, channel);
                        ledcAttachPin(pin2, channel2);
                        pwmActive = true;
                        justAttached = true;
                }
                // Simple limiter/clipper
                // Divide by 2 to prevent clipping when multiple voices sum up
                // With echo, we might need more headroom, but let's stick to /2 for now
                mixedSample = mixedSample / 2;
                if (mixedSample > 127)
                        mixedSample = 127;
                if (mixedSample < -128)
                        mixedSample = -128;
                u8 pwmVal = (u8)(mixedSample + 128);

                // Lightweight IIR smoothing to reduce stepping/quantization noise
                // smoothed = (7/8)*smoothed + (1/8)*pwmVal
                static int smoothed = 128;
                if (justAttached)
                {
                        // Initialize smoothing state to current value to avoid clicks
                        smoothed = pwmVal;
                }
                else
                {
                        smoothed = ((smoothed * 7) + pwmVal) >> 3;
                }

                ledcWrite(channel, (u8)smoothed);
                ledcWrite(channel2, 255 - (u8)smoothed); // Complementary output
        }
        else
        {
                // Silence: detach PWM and set both pins LOW
                if (pwmActive)
                {
                        ledcDetachPin(pin);
                        ledcDetachPin(pin2);
                        pwmActive = false;
                }
                digitalWrite(pin, LOW);
                digitalWrite(pin2, LOW);
        }
}