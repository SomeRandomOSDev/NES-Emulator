#pragma once

#include "Mapper.hpp"
#include "util.hpp"

class Mapper_2 :
    public Mapper
{
public:
	Mapper_2(uint8_t _PRGROMSize) : PRGROMSize(_PRGROMSize)
	{
		memset(&CHRROM[0], 0, 8 * KB);

		PRGROM.clear();
		for (uint8_t i = 0; i < PRGROMSize; i++)
			PRGROM.push_back(std::vector<uint8_t>(16 * KB, 0));

		bankSelect = 0;						// First PRG ROM bank
		bankSelect_fixed = PRGROMSize - 1;	// Last PRG ROM bank
	}

	void CPU_write_1B(uint16_t address, uint8_t value) override
	{
		if (address >= 0x8000)
		{
			bankSelect = (value & 0x0f);
		}
	}
	uint8_t CPU_read_1B(uint16_t address) override
	{
		if (address < 0x8000) // 0x0000 - 0x8000
			return 0;
		if (address < 0xc000) // 0x8000 - 0xc000
			return PRGROM[bankSelect][address - 0x8000];
		else                  // 0xc000 - 0xffff
			return PRGROM[bankSelect_fixed][address - 0xc000];
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

	uint8_t CHRROM[8 * KB];
	std::vector<std::vector<uint8_t>> PRGROM;

	uint8_t bankSelect_fixed;
	uint8_t bankSelect;
};

