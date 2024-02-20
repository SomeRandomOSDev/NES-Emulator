#pragma once

#include "Mapper.hpp"
#include "util.hpp"

class Mapper_3 :
	public Mapper
{
public:
	Mapper_3(uint8_t _PRGROMSize, uint8_t _CHRROMSize) : PRGROMSize(_PRGROMSize), CHRROMSize(_CHRROMSize)
	{
		CHRROM.clear();
		for (uint8_t i = 0; i < PRGROMSize; i++)
			CHRROM.push_back(std::vector<uint8_t>(8 * KB, 0));

		memset(&PRGROM_lo[0], 0, 16 * KB);
		memset(&PRGROM_hi[0], 0, 16 * KB);

		bankSelect = 0;						// First PRG ROM bank
		bankSelect_fixed = PRGROMSize - 1;	// Last PRG ROM bank
	}

	void CPU_write_1B(uint16_t address, uint8_t value) override
	{
		if (address >= 0x8000)
			bankSelect = value;
	}
	uint8_t CPU_read_1B(uint16_t address) override
	{
		if (address < 0x8000) // 0x0000 - 0x8000
			return 0;
		if (address < 0xc000) // 0x8000 - 0xc000
			return PRGROM_lo[address - 0x8000];
		else                  // 0xc000 - 0xffff
		{
			if (PRGROMSize == 1)
				return PRGROM_lo[address - 0xc000];
			else if (PRGROMSize == 2)
				return PRGROM_hi[address - 0xc000];
			else
				return 0;
		}
	}

	void PPU_write_1B(uint16_t address, uint8_t value) override
	{
		// CHR RAM if CHRROMSize == 0 but idc
		//CHRROM[address] = value;
	}
	uint8_t PPU_read_1B(uint16_t address) override
	{
		return CHRROM[bankSelect][address];
	}

public:
	uint8_t PRGROMSize, CHRROMSize;

	uint8_t PRGROM_lo[16 * KB];
	uint8_t PRGROM_hi[16 * KB];
	std::vector<std::vector<uint8_t>> CHRROM;

	uint8_t bankSelect_fixed;
	uint8_t bankSelect;
};