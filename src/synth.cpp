#include "synth.h"
#include <math.h>
#include <algorithm>

static Synth *synthInstance = nullptr;

void IRAM_ATTR sampleTimerISR()
{
        if (synthInstance)
        {
                synthInstance->updateSample();
        }
}

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

void Synth::setWaveform(Waveform wave)
{
        waveform = wave;
}

void Synth::setADSR(u16 attack, u16 decay, u8 sustain, u16 release)
{
        envelope.attackMs = attack;
        envelope.decayMs = decay;
        envelope.sustainLevel = std::min<u8>(255, sustain);
        envelope.releaseMs = release;
}

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

void Synth::stopNote()
{
        playing = false;
        ledcWrite(channel, 128); // Center value (silence)
}

bool Synth::isPlaying()
{
        return playing;
}

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