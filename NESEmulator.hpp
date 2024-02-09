#pragma once

#include <stdint.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <SFML/Graphics.hpp>

#include "util.hpp"

class NESEmulator
{
public:
	NESEmulator()
	{ 
		screen.create(256, 240);
		screen2.create(256, 240);

		log = "";
		logLines = 0;

		powerUp();
	}

	void powerUp()
	{
		A = X = Y = 0;
		SR = 0x34;
		S = 0xfd;
		CPU_writeMemory1B(0x4017, 0);
		CPU_writeMemory1B(0x4015, 0);
		for (uint16_t i = 0x4000; i < 0x400f; i++)
			CPU_writeMemory1B(i, 0);
		for (uint16_t i = 0x4010; i < 0x4013; i++)
			CPU_writeMemory1B(i, 0);
		CPU_cycles = 0;
		cycleCounter = 0;

		PPU_cycles = 0;
		PPU_scanline = -1;

		frameFinished = false;

		PPU_CTRL = 0x00;
		PPU_MASK = 0x00;
		PPU_STATUS = 0b10100000;
		w = false;
		v = 0x0000;
		ppuReadBuffer = 0x00;

		LOG_ADD_LINE("POWER_UP");
	}

	void reset()
	{
		S -= 3;
		SR |= 0x04;
		CPU_cycles = 9;
		PC = CPU_readMemory2B(RESET_VECTOR);
		PPU_cycles = 0;
		PPU_scanline = -1;
		cycleCounter = 0;
		frameFinished = false;

		PPU_CTRL = 0x00;
		PPU_MASK = 0x00;
		PPU_STATUS &= 0b10000000;
		w = false;
		ppuReadBuffer = 0x00;

		LOG_ADD_LINE("RESET");
	}

	void loadFromBuffer(uint16_t startAddress, uint8_t* opcodes, uint16_t size)
	{
		for (uint16_t i = 0; i < size; i++)
			CPU_writeMemory1B(startAddress + i, opcodes[i]);

		PC = startAddress;
		CPU_writeMemory2B(RESET_VECTOR, startAddress);
	}

	void loadFromiNES(std::string fileName)
	{
		LOG_ADD_LINE("Loading " + fileName + "...");

		std::ifstream f(fileName, std::ios::binary);
		if (!f) return;
		std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(f), {});
		f.close();

		iNESHeader header;
		memcpy(&header, &buffer[0], /*sizeof(header)*/16 * sizeof(uint8_t));

		uint8_t mapper_lo = (header.flags6 >> 4), mapper_hi = (header.flags7 >> 4);
		uint8_t mapperNb = (mapper_lo | (mapper_hi << 4));

		if (mapperNb == 0)
		{
			mapper = (mapperNb == 0 ? (header.PRGROMSize == 1 ? Mapper0_NROM_128 : Mapper0_NROM_256) : Other);

			//std::cout << HEX(mapper) << std::endl;

			bool trainer = (header.flags6 >> 2) & 1;

			uint16_t PRGROM_location = 16 + (512 * trainer), PRGROM_size = 16384 * header.PRGROMSize;
			uint16_t CHRROM_location = PRGROM_location + PRGROM_size, CHRROM_size = 8192 * header.CHRROMSize;

			memcpy(&CPU_memory[0x8000], &buffer[PRGROM_location], PRGROM_size);
			memcpy(&PPU_memory[0], &buffer[CHRROM_location], CHRROM_size);

			LOG_ADD_LINE("Loaded successfully (mapper 0).");
		}
		else
			LOG_ADD_LINE("Unknown mapper (mapper " + std::to_string(mapperNb) + ").");

		PC = CPU_readMemory2B(RESET_VECTOR);
	}

	void CPU_writeMemory1B(uint16_t address, uint8_t value)
	{
		if (address < 0x2000)
		{
			CPU_memory[address % 0x800] = value;
			return;
		}

		else if (address < 0x4000) // PPU registers
		{
			uint8_t regNb = address % 8;

			switch (regNb)
			{
			case 0x0000: // PPU CTRL
				PPU_CTRL = value;

				break;

			case 0x0001: // PPU MASK
				PPU_MASK = value;

				break;

			case 0x0006: // PPU ADDRESS
				if (w == 0)
				{
					v &= 0x00ff;
					v |= (uint16_t)value << 8;
				}
				else
				{
					v &= 0xff00;
					v |= value;
				}

				w ^= true;

				break;

			case 0x0007: // PPU DATA
				PPU_writeMemory1B(v, value);

				v += REG_GET_FLAG(PPU_CTRL, PPU_CTRL_VRAM_ADDRESS_INCREMENT_DIRECTION) ? 32 : 1;

				break;
			}

			return;
		}

		else if (address < 0x4018) // APU and IO registers
			;

		else if (address < 0x4020) // APU and I/O functionality that is normally disabled
			;

		// Cartridge space

		else if (mapper == Mapper0_NROM_128 || mapper == Mapper0_NROM_256)
		{
			//if (address < 0x8000) // Family BASIC only
			//	;

			//if (address < 0xc000) // First 16 KB of ROM.
			//	; // CPU_memory[address] = value;
			//if (mapper == Mapper0_NROM_128) // Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
			//	; //CPU_memory[address - 16384] = value;
			//if (mapper == Mapper0_NROM_256) // Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
			//	; //CPU_memory[address] = value;
		}
		else
		{
			//;
		}
	}

	uint8_t CPU_readMemory1B(uint16_t address)
	{
		if (address < 0x2000)
			return CPU_memory[address % 0x800];

		if (address < 0x4000) // PPU registers
		{
			uint8_t regNb = address % 8;

			switch (regNb)
			{
			case 0x0002:
			{
				w = false;

				uint8_t value = PPU_STATUS;

				REG_SET_FLAG_0(PPU_STATUS, PPU_STATUS_VBLANK);

				return value;
			}

			case 0x0007:
				if (address <= 0x3eff)
				{
					uint8_t value = ppuReadBuffer;
					ppuReadBuffer = PPU_readMemory1B(v);
					return value;
				}
				else
				{
					ppuReadBuffer = PPU_readMemory1B(v);
					return PPU_readMemory1B(v);
				}
			}
		}

		if (address < 0x4018) // APU and IO registers
			return 0;

		if (address < 0x4020) // APU and I/O functionality that is normally disabled
			return 0;

		// Cartridge space

		if (mapper == Mapper0_NROM_128 || mapper == Mapper0_NROM_256)
		{
			if (address < 0x8000) // Family BASIC only
				return 0;

			if (address < 0xc000) // First 16 KB of ROM.
				return CPU_memory[address];
			if (mapper == Mapper0_NROM_128) // Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
				return CPU_memory[address - 16384];
			if (mapper == Mapper0_NROM_256) // Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
				return CPU_memory[address];
		}

		return 0;
	}

	void PPU_writeMemory1B(uint16_t address, uint8_t value)
	{
		address %= 0x4000;

		if (address >= 0x3000 && address <= 0x3eff) // mirrors of 0x2000 - 0x2eff
			address -= 0x1000;
		while (address >= 0x3f20) // mirrors of 0x3f00 - 0x3f1f
			address -= 0x0020;

		if (address == 0x3f10)
			address = 0x3f00;
		if (address == 0x3f14)
			address = 0x3f04;
		if (address == 0x3f18)
			address = 0x3f08;
		if (address == 0x3f1c)
			address = 0x3f0c;

		PPU_memory[address] = value;
	}

	uint8_t PPU_readMemory1B(uint16_t address)
	{
		address %= 0x4000;

		if (address >= 0x3000 && address <= 0x3eff) // mirrors of 0x2000 - 0x2eff
			address -= 0x1000;
		while (address >= 0x3f20) // mirrors of 0x3f00 - 0x3f1f
			address -= 0x0020;

		if (address == 0x3f10)
			address = 0x3f00;
		if (address == 0x3f14)
			address = 0x3f04;
		if (address == 0x3f18)
			address = 0x3f08;
		if (address == 0x3f1c)
			address = 0x3f0c;

		return PPU_memory[address];
	}

	uint16_t CPU_readMemory2B(uint16_t address)
	{
		return ((uint16_t)CPU_readMemory1B(address + 1) << 8) | CPU_readMemory1B(address);
	}

	void CPU_writeMemory2B(uint16_t address, uint16_t value)
	{
		CPU_writeMemory1B(address, value & 0xff);
		CPU_writeMemory1B(address + 1, value >> 8);
	}

	uint16_t PPU_readMemory2B(uint16_t address)
	{
		return ((uint16_t)PPU_readMemory1B(address + 1) << 8) | PPU_readMemory1B(address);
	}

	void PPU_writeMemory2B(uint16_t address, uint16_t value)
	{
		PPU_writeMemory1B(address, value & 0xff);
		PPU_writeMemory1B(address + 1, value >> 8);
	}

	void push1B(uint8_t value)
	{
		CPU_writeMemory1B(S + 0x100, value);
		S--;
	}

	uint8_t pull1B()
	{
		S++;
		return CPU_readMemory1B(S + 0x100);
	}

	void push2B(uint16_t value)
	{
		push1B(value >> 8);
		push1B(value & 0xff);
	}

	uint16_t pull2B()
	{
		uint8_t lo = pull1B();
		uint8_t hi = pull1B();

		return ((uint16_t)hi << 8) | lo;
	}

	uint8_t readIndexedIndirectX(uint8_t d)
	{
		return CPU_readMemory1B(CPU_readMemory1B((d + X) % 256) + (CPU_readMemory1B((d + X + 1) % 256) * 256));
	}

	uint8_t readIndirectIndexedY(uint8_t d, bool& pageBoundaryCrossed)
	{
		uint16_t lo = CPU_readMemory1B(d);
		pageBoundaryCrossed = lo > 0xff;
		return CPU_readMemory1B(lo + (CPU_readMemory1B((d + 1) % 256) * 256) + Y);
	}

	uint8_t zeropageIndexedXAddress(uint8_t d)
	{
		return (d + X) % 256;
	}

	uint8_t zeropageIndexedYAddress(uint8_t d)
	{
		return (d + Y) % 256;
	}

	uint16_t absoluteIndexedXAddress(uint16_t a, bool pageBoundaryCrossed)
	{
		pageBoundaryCrossed = ((a >> 8) == ((a + Y) >> 8));
		return a + X;
	}

	uint16_t absoluteIndexedYAddress(uint16_t a, bool pageBoundaryCrossed)
	{
		pageBoundaryCrossed = ((a >> 8) == ((a + Y) >> 8));
		return a + Y;
	}

	uint16_t indexedIndirectXAddress(uint16_t d)
	{
		return CPU_readMemory1B((d + X) % 256) + (CPU_readMemory1B((d + X + 1) % 256) * 256);
	}

	uint16_t indirectIndexedYAddress(uint8_t d)
	{
		uint16_t lo = CPU_readMemory1B(d);
		/*pageBoundaryCrossed = lo > 0xff;*/
		return lo + (CPU_readMemory1B((d + 1) % 256) * 256) + Y;
	}

	sf::Color GetColorFromPalette(uint8_t paletteNumber, PaletteType paletteType, uint8_t index)
	{
		//  43210
		//	|||||
		//	|||++ - Pixel value from tile data
		//	|++-- - Palette number from attribute table or OAM
		//	+---- - Background / Sprite select

		uint16_t address = 0x3f00 | ((paletteType << 4) | ((paletteNumber & 0b11) << 2) | (index & 0b11));

		return NESColorToRGB(PPU_readMemory1B(address));
	}

	uint16_t GetRowAddressFromTile(bool bitPlane, bool patternTableHalf, uint8_t tileNumber, uint8_t row)
	{
		return (patternTableHalf << 0xc) | (tileNumber << 4) | (bitPlane << 3) | (row & 0b111);
	}

	uint8_t GetRowFromTile(bool bitPlane, bool patternTableHalf, uint8_t tileNumber, uint8_t row)
	{
		return PPU_readMemory1B(GetRowAddressFromTile(bitPlane, patternTableHalf, tileNumber, row));
	}

	sf::Image GetPatternTable(bool patternTableHalf, uint8_t palette)
	{
		sf::Image img;
		img.create(128, 128);
		for (unsigned int i = 0; i < 16; i++)
		{
			for (unsigned int j = 0; j < 16; j++)
			{
				uint8_t tileIndex = i * 16 + j;

				for (unsigned int k = 0; k < 8; k++)
				{
					uint8_t lo = GetRowFromTile(0, patternTableHalf, tileIndex, k);
					uint8_t hi = GetRowFromTile(1, patternTableHalf, tileIndex, k);

					for (unsigned int l = 0; l < 8; l++)
					{
						uint8_t index = REG_GET_FLAG(lo, 7 - l) | (REG_GET_FLAG(hi, 7 - l) << 1);

						img.setPixel(j * 8 + l, i * 8 + k, GetColorFromPalette(palette, Background, index));
					}
				}
			}
		}
		return img;
	}

	void cycle(bool printLog);
	void PPU_cycle();
	void INTERRUPT(uint16_t returnAddress, uint16_t isrAddress, bool B_FLAG);
	void NMI();
	std::string CPU_cycle();

public:
	uint8_t CPU_memory[64 * KB];
	uint8_t PPU_memory[16 * KB];
	uint8_t A, X, Y, SR, S;
	uint16_t PC;

	uint8_t CPU_cycles;
	uint16_t PPU_cycles;
	uint8_t cycleCounter;
	int16_t PPU_scanline;
	Mapper mapper;

	sf::Image screen, screen2;
	bool frameFinished;

	std::string log;
	uint32_t logLines;

	bool w; // write latch
	uint16_t v; // VRAM address
	uint8_t ppuReadBuffer;
};