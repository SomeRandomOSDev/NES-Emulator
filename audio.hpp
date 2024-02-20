#pragma once

#include <SFML/Audio.hpp>
#include "util.hpp"

namespace
{
	sf::Int16 SineWave(float i, float frequency, float amplitude)
	{
		return sf::Int16(32767 * amplitude * sin(2 * PI * i * frequency / 44100.f));
	}

	float sine_approx_half(float t)
	{
		return 16 * t * (PI - t) / (5 * sqr(PI) - 4 * t * (PI - t));
	}

	float sine_approx(float x)
	{
		x -= (int)x;
		x *= 2 * PI;
		return (x < PI ? sine_approx_half(x) : -sine_approx_half(x - PI));
	}

	float sine_approx_2_half(float t)
	{
		return 16 * (0.5f - t);// 0.405 * (PI - t);
	}

	float sine_approx_2(float x)
	{
		x -= (int)x;
		return (x < 0.5f ? sine_approx_2_half(x) : -sine_approx_2_half(x - 0.5f));
	}

	sf::Int16 SineWave_Approx(float i, float frequency, float amplitude)
	{
		return sf::Int16(32767 * amplitude * sine_approx(i * frequency / 44100.f));
	}

	sf::Int16 SineWave_Approx_2(float i, float frequency, float amplitude)
	{
		return sf::Int16(32767 * amplitude * sine_approx(i * frequency / 44100.f));
	}

	sf::Int16 SquareWave(float i, float frequency, float duty, uint8_t harmonics, float amplitude)
	{
		float f = 0, g = 0;

		for (uint8_t j = 1; j <= harmonics; j++)
			f += SineWave_Approx_2(i * j, frequency, amplitude / j);
		for (uint8_t j = 1; j <= harmonics; j++)
			g += SineWave_Approx_2((i + duty) * j, frequency, amplitude / j);
		float r = f - g;
		return sf::Int16(r);
	}

	struct PulseWave
	{
		float frequency;		//	fCPU / (16 × (t + 1))
		uint16_t t;	
		float duty;
		float volume;
	};
}