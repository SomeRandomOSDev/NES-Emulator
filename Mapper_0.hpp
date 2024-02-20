#pragma once

#include "Mapper.hpp"
#include "util.hpp"

class Mapper_0 :
    public Mapper
{
public:
	Mapper_0(uint8_t _PRGROMSize) : PRGROMSize(_PRGROMSize)
    {
		memset(&CHRROM[0],    0, 8 * KB);
		memset(&PRGROM_lo[0], 0, 16 * KB);
		memset(&PRGROM_hi[0], 0, 16 * KB);
    }

	void CPU_write_1B(uint16_t address, uint8_t value) override
	{ 

	}
	uint8_t CPU_read_1B(uint16_t address) override
	{
		switch (PRGROMSize)
		{
		case 1:
			if(address >= 0x8000)
				return PRGROM_lo[(address - 0x8000) % (16 * KB)];

			return 0;

		case 2:
			if (address >= 0x8000)
			{
				if(address >= 0xc000)
					return PRGROM_hi[(address - 0xc000)];
				return PRGROM_lo[(address - 0x8000)];
			}

			return 0;

		default:
			return 0;
		}
	}

	void PPU_write_1B(uint16_t address, uint8_t value) override
	{ 
		// CHR RAM if CHRROMSize == 0 but idc
		CHRROM[address] = value;
	}
	uint8_t PPU_read_1B(uint16_t address) override
	{
		return CHRROM[address];
	}

public:
	uint8_t PRGROMSize;

	uint8_t PRGROM_lo[16 * KB];
	uint8_t PRGROM_hi[16 * KB];
	uint8_t CHRROM	 [8 * KB];
};

