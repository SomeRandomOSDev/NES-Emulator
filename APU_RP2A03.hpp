#pragma once

#include <SFML/Audio.hpp>
#include "audio.hpp"

class APU_RP2A03 : public sf::SoundStream
{
public:
    APU_RP2A03()
    {
        t = 0;
        initialize(1, 44100);
    }

private:
    virtual bool onStart()
    {
        t = 0;
        return true;
    }

    sf::Int16 Pulse_1(float x)
    {
        pulse_1.frequency = 1789773.f / (16 * (pulse_1.t + 1));
        sf::Int16 sample = SquareWave(x, pulse_1.frequency, pulse_1.duty, 10, pulse_1.volume);
        if (t < 8)
            sample = 0;
        return sample;
    }

    virtual bool onGetData(Chunk& chunk)
    {
        samples.clear();
        for (uint16_t i = 0; i < 44100 / 60; i++)
        {
            float x = float(t + i);
            sf::Int16 sample = 0;
            sample += Pulse_1(x);
            samples.push_back(sample);
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
    PulseWave pulse_1;
};