#pragma once

#include <stdint.h>

namespace
{
	enum Mirroring
	{
		Horizontal = 0,
		Vertical = 1
	};
}

class Mapper
{
public:
	Mapper()
	{
		mirroring = Horizontal;
	}

	virtual void CPU_write_1B(uint16_t address, uint8_t value)
	{ }
	virtual uint8_t CPU_read_1B(uint16_t address)
	{
		return 0;
	}
	virtual void PPU_write_1B(uint16_t address, uint8_t value)
	{ }
	virtual uint8_t PPU_read_1B(uint16_t address)
	{
		return 0;
	}

public:
	Mirroring mirroring;
};