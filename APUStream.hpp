#pragma once

#include <SFML/Audio.hpp>
#include "audio.hpp"

class APUStream : public sf::SoundStream
{
public:
    APUStream()
    {
        initialize(1, 44100);
    }

private:
    virtual bool onStart()
    {
        t = 0;
        return true;
    }

    virtual bool onGetData(Chunk& chunk)
    {
        samples.clear();
        for (uint16_t i = 0; i < 44100 / 60; i++)
        {
            float x = t + i * frequency;
            sf::Int16 sample;
            sample = SineWave_Approx_2(x, 1, 0.5f);
            //sample = SquareWave(x, 1, 0.5f, 20, 0.5f);
            samples.push_back(sample);
        }

        chunk.samples = &samples[0];
        chunk.sampleCount = samples.size();

        t += uint64_t(frequency * 44100 / 60);

        return true;
    }

    virtual void onSeek(sf::Time offset)
    {
        t = uint64_t(offset.asSeconds() * 44100);
    }

    uint64_t t;
    std::vector<sf::Int16> samples;

public:
    float frequency;
};