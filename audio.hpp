#pragma once

#include <SFML/Audio.hpp>
#include "util.hpp"

namespace
{
	float SineWave(float i, float frequency, float amplitude)
	{
		return float(amplitude * sin(2 * PI * i * frequency / 44100.f));
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

	inline float sine_approx_2(float x)
	{
		x -= (int)x;
		return (x < 0.5f ? sine_approx_2_half(x) : -sine_approx_2_half(x - 0.5f));
	}

	inline float SineWave_Approx(const float i, const float frequency, const float amplitude)
	{
		return float(amplitude * sine_approx(i * frequency / 44100.f));
	}

	inline float SineWave_Approx_2(const float i, const float frequency, const float amplitude)
	{
		return float(amplitude * sine_approx_2(i * frequency / 44100.f));
	}

	float PulseWave(const float i, const float frequency, const float duty, const uint8_t harmonics, const float amplitude)
	{
		float f = 0, g = 0;

		for (uint8_t j = 1; j <= harmonics; j++)
			f += SineWave(i * j, frequency, amplitude / j);
		for (uint8_t j = 1; j <= harmonics; j++)
			g += SineWave((i + duty) * j, frequency, amplitude / j);
		float r = f - g;
		return r;
	}

	float PulseWave_Approx(const float i, const float frequency, const float duty, const uint8_t harmonics, const float amplitude)
	{
		float f = 0, g = 0;

		for (uint8_t j = 1; j <= harmonics; j++)
			f += SineWave_Approx(i * j, frequency, amplitude / j);
		for (uint8_t j = 1; j <= harmonics; j++)
			g += SineWave_Approx((i + duty) * j, frequency, amplitude / j);
		float r = f - g;
		return r;
	}

	float PulseWave_Approx_2(const float i, const float frequency, const float duty, const uint8_t harmonics, const float amplitude)
	{
		float f = 0, g = 0;

		for (uint8_t j = 1; j <= harmonics; j++)
			f += SineWave_Approx_2(i * j, frequency, amplitude / j);
		for (uint8_t j = 1; j <= harmonics; j++)
			g += SineWave_Approx_2((i + duty) * j, frequency, amplitude / j);
		float r = f - g;
		return r;
	}

	// \frac{8}{\pi^{2}}\sum_{i=0}^{N-1}\left(\left(-1\right)^{i}n^{-2}\sin\left(2\pi f_{0}nt\right)\right)
	float TriangleWave(const float x, const float frequency, const uint8_t harmonics, const float amplitude)
	{
		float sum = 0;

		float sign = 1;
		for (uint8_t i = 0; i < harmonics; i++)
		{
			const float n = float(2 * i + 1);

			sum += sign * (1 / sqr(n)) * sin(2 * PI * frequency * n * x);

			sign *= -1;
		}

		return sum * 8.f / sqr(PI);
	}
}