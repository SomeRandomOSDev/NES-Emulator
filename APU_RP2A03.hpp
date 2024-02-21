#pragma once

#include <SFML/Audio.hpp>
#include "audio.hpp"

#define HANDLE_SWEEP(channel)   if (channel.sweepEnabled && !(channel.sweepShiftCount == 0)) \
                                { \
                                    if (channel.sweepTargetPeriod < channel.t) \
                                    { \
                                        if ((t % channel.sweepPeriod) == 0) \
                                            channel.t--; \
                                    } \
                                    else if (channel.sweepTargetPeriod > channel.t) \
                                    { \
                                        if ((t % channel.sweepPeriod) == 0) \
                                            channel.t++; \
                                    } \
                                }

class APU_RP2A03 : public sf::SoundStream
{
public:
    APU_RP2A03()
    {
        t = 0;
        squareWaveHarmonics = 5;
        //pulse_1.constantVolume = true;
        //pulse_2.constantVolume = true;
        //pulse_1.lengthCounter = 1;
        //pulse_2.lengthCounter = 1;
        //pulse_1.lengthCounterHalt = true;
        //pulse_2.lengthCounterHalt = true;
        cycles = 0;
        initialize(1, 44100);
    }

    void cycle()
    {
        if (frameCounterMode == 0)
        {
            uint32_t frameCycles = (cycles % 14915);
            if (frameCycles == 3728 || frameCycles == 11185) // 3728.5 | 11185.5
                quarterFrame();

            if (frameCycles == 7456 || frameCycles == 14914) // 7456.5 | 14914.5 
            {
                quarterFrame();
                halfFrame();
            }

            if (frameCycles == 14914)
                frameInterrupt = true;
        }
        else
        {
            uint32_t frameCycles = (cycles % 18641);
            if (frameCycles == 3728 || frameCycles == 11185) // 3728.5 | 11185
                quarterFrame();

            if (frameCycles == 7456 || frameCycles == 18640) // 7456.5 | 18640.5
            {
                quarterFrame();
                halfFrame();
            }
        }

        if (interruptInhibit)
            frameInterrupt = false;

        cycles++;
    }

private:
    virtual bool onStart()
    {
        t = 0;
        return true;
    }

    float Pulse_1(float x)
    {
        float sample = PulseWave_Approx(x, pulse_1.frequency, pulse_1.duty, squareWaveHarmonics, pulse_1.volume);
        if (pulse_1.t < 8)
            sample = 0;
        return sample * !(pulse_1.lengthCounter == 0 && pulse_1.constantVolume);
    }

    float Pulse_2(float x)
    {
        float sample = PulseWave_Approx(x, pulse_2.frequency, pulse_2.duty, squareWaveHarmonics, pulse_2.volume);
        if (pulse_2.t < 8)
            sample = 0;
        return sample * !(pulse_2.lengthCounter == 0 && pulse_2.constantVolume);
    }

    float PulseMix(float sum)
    {
        return 95.52f / ((8128.f / sum) + 100);
    }

    float TndMix(float triangle, float noise, float dmc)
    {
        return 159.79f / ((1 / ((triangle / 8227.f) + (noise / 12241.f) + (dmc / 22638.f))) + 100);
    }

    void quarterFrame()
    {
        if (!pulse_1.constantVolume)
        {
            if (pulse_1.volume > 0)
                pulse_1.volume -= (1 / 15.f);
            else
            {
                if (pulse_1.envelopeLoop)
                    pulse_1.volume = 1;
                else
                    pulse_1.volume = 0;
            }
        }

        if (!pulse_2.constantVolume)
        {
            if (pulse_2.volume > 0)
                pulse_2.volume -= (1 / 15.f);
            else
            {
                if (pulse_2.envelopeLoop)
                    pulse_2.volume = 1;
                else
                    pulse_2.volume = 0;
            }
        }
    }

    void halfFrame()
    {
        //int16_t changeAmount = (pulse_1.t >> pulse_1.sweepShiftCount);
        //if (pulse_1.sweepNegate)
        //    changeAmount = -changeAmount - 1;
        //uint16_t targetPeriod = std::max(0, pulse_1.t + changeAmount);
        //pulse_1.sweepTargetPeriod = targetPeriod;

        //changeAmount = (pulse_2.t >> pulse_2.sweepShiftCount);
        //if (pulse_2.sweepNegate)
        //    changeAmount = -changeAmount;
        //targetPeriod = std::max(0, pulse_2.t + changeAmount);
        //pulse_2.sweepTargetPeriod = targetPeriod;

        //HANDLE_SWEEP(pulse_1);
        //HANDLE_SWEEP(pulse_2);

        if (pulse_1.t > 0)
            pulse_1.t--;
        if (pulse_2.t > 0)
            pulse_2.t--;

        //if (pulse_1.lengthCounter > 0 && (!pulse_1.lengthCounterHalt) && pulse_1.constantVolume)
        //    pulse_1.lengthCounter--;
        //if (pulse_2.lengthCounter > 0 && (!pulse_2.lengthCounterHalt) && pulse_2.constantVolume)
        //    pulse_2.lengthCounter--;

        pulse_1.lengthCounter *= pulse_1.enable;
        pulse_2.lengthCounter *= pulse_2.enable;
    }

    virtual bool onGetData(Chunk& chunk)
    {
        samples.clear();
        for (uint16_t i = 0; i < 44100 / 60; i++)
        {
            float x = float(t + i);
            float pulse_out = PulseMix((Pulse_1(x) + Pulse_2(x)) * 75);
            float tnd_out = TndMix(0, 0, 0);
            float sample = std::clamp((pulse_out + tnd_out) * 0.8f, 0.f, 1.f);
            samples.push_back(sf::Int16(32767.f * sample));
        }

        chunk.samples = &samples[0];
        chunk.sampleCount = samples.size();

        t += uint64_t(44100 / 60);

        return true;
    }

    virtual void onSeek(sf::Time offset)
    {
        t = uint64_t(offset.asSeconds() * 44100);
    }

    uint64_t t;
    std::vector<sf::Int16> samples;

public:
    PulseWaveSequencer pulse_1, pulse_2;
    uint32_t cycles;
    bool frameCounterMode;
    bool interruptInhibit;
    bool frameInterrupt;
    uint8_t squareWaveHarmonics;
};