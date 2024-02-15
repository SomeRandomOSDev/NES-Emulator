#pragma once

#include <stdint.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <SFML/Graphics.hpp>

#include "util.hpp"

class NESEmulator // NTSC NES with RP2A03/2C02G
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
		stopCPU = true;

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
		memset(&PPU_memory[0], 0, 0x4000);

		frameFinished = false;

		PPU_CTRL = 0x00;
		PPU_MASK = 0x00;
		PPU_STATUS = 0b10100000;
		w = false;
		t = 0x0000;
		v = 0x0000;
		ppuReadBuffer = 0x00;
		x = 0;
		OAMAddress = 0;

		controllerLatch1 = false;

		LOG_ADD_LINE("POWER_UP");
	}

	void reset()
	{
		stopCPU = true;

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
		t = 0x0000;
		v = 0x0000;
		ppuReadBuffer = 0x00;
		x = 0;
		OAMAddress = 0;

		controllerLatch1 = false;

		LOG_ADD_LINE("RESET");
	}

	//void loadFromBuffer(uint16_t startAddress, uint8_t* opcodes, uint16_t size)
	//{
	//	LOG_ADD_LINE("Loading from buffer...");

	//	memcpy(&CPU_memory[startAddress], opcodes, size);

	//	PC = startAddress;
	//	CPU_memory[RESET_VECTOR] = startAddress & 0xff;
	//	CPU_memory[RESET_VECTOR + 1] = startAddress >> 8;

	//	LOG_ADD_LINE("Loaded successfully at address 0x" + HEX(startAddress));
	//}

	void loadFromiNES(std::string fileName)
	{
		LOG_ADD_LINE("Loading \"" + fileName + "\"...");

		std::ifstream f(fileName, std::ios::binary);
		if (!f) return;
		std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(f), {});
		f.close();

		iNESHeader header;
		memcpy(&header, &buffer[0], /*sizeof(header)*/16 * sizeof(uint8_t));

		uint8_t mapper_lo = (header.flags6 >> 4), mapper_hi = (header.flags7 >> 4);
		uint8_t mapperNb = (mapper_lo | (mapper_hi << 4));

		bool trainer = (header.flags6 >> 2) & 1;

		PRGRAM = (header.flags6 >> 1) & 1;

		uint32_t PRGROM_location = 16 + (512 * trainer), PRGROM_size = 16384 * header.PRGROMSize;
		uint32_t CHRROM_location = PRGROM_location + PRGROM_size, CHRROM_size = 8192 * header.CHRROMSize;

		stopCPU = false;
		mapper = Other;
		switch (mapperNb)
		{
		case 0:
			mapper = header.PRGROMSize == 1 ? Mapper0_NROM_128 : Mapper0_NROM_256;

			mirroring = Mirroring(header.flags6 & 1);

			memcpy(&CPU_memory[0x8000], &buffer[PRGROM_location], PRGROM_size);
			memcpy(&PPU_memory[0], &buffer[CHRROM_location], CHRROM_size);

			LOG_ADD_LINE("Loaded successfully : ");
			LOG_ADD_LINE("mapper 0 | " + 
			(mirroring == Horizontal ? "Horizontal" : "Vertical") + " mirroring.");
			
			break;

		//case 1:
		//	mapper = Mapper1_MMC1;

		//	mirroring = Mirroring(header.flags6 & 1);

		//	memcpy(&CPU_memory[0x8000], &buffer[PRGROM_location], PRGROM_size);
		//	memcpy(&PPU_memory[0], &buffer[CHRROM_location], CHRROM_size);

		//	LOG_ADD_LINE("Loaded successfully : ");
		//	LOG_ADD_LINE("mapper 1 | " +
		//	(mirroring == Horizontal ? "Horizontal" : "Vertical") + " mirroring.");

		//	break;

		default:
			LOG_ADD_LINE("Unknown mapper (mapper " + std::to_string(mapperNb) + ").");
			stopCPU = true;
			break;
		}

		PC = CPU_readMemory2B(RESET_VECTOR);
	}

	bool loadPaletteFromPAL(std::string fileName)
	{
		std::ifstream f(fileName, std::ios::binary);
		if (!f) return false;
		std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(f), {});
		f.close();

		memcpy(&NESPalette[0], &buffer[0], 192);

		return true;
	}

	uint16_t PPU_HANDLE_ADDRESS(uint16_t address)
	{
		address &= 0x3fff;

		// Nametable mirroring
		if (mirroring == Horizontal)
		{
			if ((address >= 0x2400 && address < 0x2800) || (address >= 0x2c00 && address < 0x3000))
				address -= 0x400;
		}
		else if (mirroring == Vertical)
		{
			if ((address >= 0x2800 && address < 0x2c00) || (address >= 0x2c00 && address < 0x3000))
				address -= 0x800;
		}

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

		return address;
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
			switch (address % 8)
			{
			case 0x0000: // PPU CTRL
				PPU_CTRL = value;

				LOOPY_SET_NAMETABLE(t, 
				REG_GET_FLAG(PPU_CTRL, PPU_CTRL_NAMETABLE_ADDRESS_Y) * 2 +
				REG_GET_FLAG(PPU_CTRL, PPU_CTRL_NAMETABLE_ADDRESS_X));

				break;

			case 0x0001: // PPU MASK
				PPU_MASK = value;

				break;

			case 0x0003:
				OAMAddress = value;

				break;

			case 0x0004:
				*OAMGetByte(OAMAddress) = value;
				OAMAddress++;

				break;

			case 0x0005: // PPU SCROLL
				if (w == 0) // X
				{
					x = (value & 0b111);
					LOOPY_SET_COARSE_X(t, (value >> 3));
				}
				else        // Y
				{
					LOOPY_SET_FINE_Y(t, (value & 0b111));
					LOOPY_SET_COARSE_Y(t, (value >> 3));
				}

				w ^= true;

				break;

			case 0x0006: // PPU ADDRESS
				if (w == 0)
				{
					t &= 0x00ff;
					t |= (uint16_t)(value & 0b01111111) << 8;
				}
				else
				{
					t &= 0xff00;
					t |= value;
					v = t;
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
		{
			switch (address)
			{
			case 0x4014:
				for (unsigned int i = 0; i < 256; i++)
				{
					uint16_t dmaAddress = ((uint16_t)value << 8) | i;

					*OAMGetByte(OAMAddress + i) = CPU_readMemory1B(dmaAddress);
				}

				CPU_cycles = 513; // or 514

				break;

			case 0x4016:
				controllerLatch1 = (value & 1);

				break;
			}

			return;
		}

		else if (address < 0x4020) // APU and I/O functionality that is normally disabled
			return;

		// Cartridge space

		else if (mapper == Mapper0_NROM_128 || mapper == Mapper0_NROM_256)
		{
			if (address < 0x8000) // Family BASIC only
				CPU_memory[address] = value; // 8 KB PRG RAM

			//if (address < 0xc000) // First 16 KB of ROM.
			//	; // CPU_memory[address] = value;
			//if (mapper == Mapper0_NROM_128) // Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
			//	; //CPU_memory[address - 16384] = value;
			//if (mapper == Mapper0_NROM_256) // Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
			//	; //CPU_memory[address] = value;
			return;
		}
		else if (mapper == Mapper1_MMC1)
		{
			if (address < 0x6000)
				return;
			if(address < 0x8000)
				CPU_memory[address] = value; // 8KB PRG RAM
			// ROM
			return;
		}
		else
		{
			//;
			return;
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
			case 0x0002: // PPU STATUS
			{
				w = false;

				uint8_t value = PPU_STATUS;

				REG_SET_FLAG_0(PPU_STATUS, PPU_STATUS_VBLANK);

				return value;
			}

			case 0x0004:
				return *OAMGetByte(OAMAddress);

			case 0x0007:	// PPU DATA
				if (address <= 0x3eff)
				{
					uint8_t value = ppuReadBuffer;
					ppuReadBuffer = PPU_readMemory1B(v);
					v += REG_GET_FLAG(PPU_CTRL, PPU_CTRL_VRAM_ADDRESS_INCREMENT_DIRECTION) ? 32 : 1;
					return value;
				}
				else
				{
					ppuReadBuffer = PPU_readMemory1B(v);
					v += REG_GET_FLAG(PPU_CTRL, PPU_CTRL_VRAM_ADDRESS_INCREMENT_DIRECTION) ? 32 : 1;
					return ppuReadBuffer;
				}
			}
		}

		if (address < 0x4018) // APU and IO registers
		{
			switch (address)
			{
			case 0x4016:
				uint8_t value = (controller1ShiftRegister & 1);
				controller1ShiftRegister >>= 1;
				return value;
			}
		}

		if (address < 0x4020) // APU and I/O functionality that is normally disabled
			return 0;

		// Cartridge space
		if (mapper == Mapper0_NROM_128 || mapper == Mapper0_NROM_256)
		{
			if (address < 0x8000) // Family BASIC only
				return CPU_memory[address]; // 8 KB PRG RAM

			if (address < 0xc000) // First 16 KB of ROM.
				return CPU_memory[address];
			if (mapper == Mapper0_NROM_128) // Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
				return CPU_memory[address - 16384];
			if (mapper == Mapper0_NROM_256) // Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
				return CPU_memory[address];
		}
		else if (mapper == Mapper1_MMC1)
		{
			if (address < 0x6000)
				return 0;
			if (address < 0x8000)
				return CPU_memory[address]; // 8KB PRG RAM
			if (address < 0xc000)		
				return CPU_memory[address]; // 16KB ROM
			return CPU_memory[address]; // 16KB ROM
		}

		return 0;
	}

	void PPU_writeMemory1B(uint16_t address, uint8_t value)
	{
		address = PPU_HANDLE_ADDRESS(address);

		PPU_memory[address] = value;
	}

	uint8_t PPU_readMemory1B(uint16_t address)
	{
		address = PPU_HANDLE_ADDRESS(address);

		return PPU_memory[address];
	}

	uint16_t CPU_readMemory2B(uint16_t address)
	{
		return (((uint16_t)CPU_readMemory1B(address + 1) << 8) | CPU_readMemory1B(address));
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

	void PPU_coarse_X_increment()
	{
		if (RENDERING_ENABLED)
		{
			if ((v & 0x001F) == 31)
			{
				v &= ~0x001F;
				v ^= 0x0400;
			}
			else
				v += 1;
		}
	}

	void PPU_Y_increment()
	{
		if (RENDERING_ENABLED)
		{
			if ((v & 0x7000) != 0x7000)
				v += 0x1000;
			else
			{
				v &= ~0x7000;
				int y = (v & 0x03E0) >> 5;
				if (y == 29)
				{
					y = 0;
					v ^= 0x0800;
				}
				else if (y == 31)
					y = 0;
				else
					y += 1;

				v = (v & ~0x03E0) | (y << 5);
			}
		}
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

		return (((uint16_t)hi << 8) | lo);
	}

	uint8_t zeropageIndexedXAddress(uint8_t d)
	{
		return d + X;
	}

	uint8_t zeropageIndexedYAddress(uint8_t d)
	{
		return d + Y;
	}

	uint16_t absoluteIndexedXAddress(uint16_t a, bool pageBoundaryCrossed)
	{
		pageBoundaryCrossed = ((a >> 8) != ((a + X) >> 8));
		return a + X;
	}

	uint16_t absoluteIndexedYAddress(uint16_t a, bool pageBoundaryCrossed)
	{
		pageBoundaryCrossed = ((a >> 8) != ((a + Y) >> 8));
		return a + Y;
	}

	uint16_t indexedIndirectXAddress(uint8_t d)
	{
		return CPU_readMemory1B((d + X) & 0xff) + ((uint16_t)CPU_readMemory1B((d + X + 1) & 0xff) << 8);
	}

	uint16_t indirectIndexedYAddress(uint8_t d)
	{
		/*uint16_t lo = CPU_readMemory1B(d) + Y;
		pageBoundaryCrossed = lo > 0xff;
		return lo + (CPU_readMemory1B((d + 1) % 256) * 256);*/
		return CPU_readMemory1B(d) + Y + ((uint16_t)CPU_readMemory1B((d + 1) & 0xff) << 8);
	}

	uint8_t readIndexedIndirectX(uint8_t d)
	{
		return CPU_readMemory1B(indexedIndirectXAddress(d));
	}

	uint8_t readIndirectIndexedY(uint8_t d, bool& pageBoundaryCrossed)
	{
		uint16_t lo = CPU_readMemory1B(d);
		pageBoundaryCrossed = lo > 0xff;
		return CPU_readMemory1B(lo + ((uint16_t)CPU_readMemory1B((d + 1) % 256) << 8) + Y);
	}

	inline sf::Color NESColorToRGB(uint8_t colorCode)
	{
		return sf::Color(NESPalette[(colorCode % 64) * 3],
						 NESPalette[(colorCode % 64) * 3 + 1],
						 NESPalette[(colorCode % 64) * 3 + 2]);
	}

	uint8_t GetColorCodeFromPalette(uint8_t paletteNumber, PaletteType paletteType, uint8_t index)
	{
		//  43210
		//	|||||
		//	|||++ - Pixel value from tile data
		//	|++-- - Palette number from attribute table or OAM
		//	+---- - Background / Sprite select

		uint16_t address = 0x3f00 | ((paletteType << 4) | ((paletteNumber & 0b11) << 2) | (index & 0b11));

		return PPU_readMemory1B(address);
	}

	sf::Color GetColorFromPalette(uint8_t paletteNumber, PaletteType paletteType, uint8_t index)
	{
		//  43210
		//	|||||
		//	|||++ - Pixel value from tile data
		//	|++-- - Palette number from attribute table or OAM
		//	+---- - Background / Sprite select

		uint16_t address = 0x3f00 | ((int(paletteType) << 4) | ((paletteNumber & 0b11) << 2) | (index & 0b11));

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

						img.setPixel(j * 8 + l, i * 8 + k, GetColorFromPalette(palette, PaletteType((palette >> 2) & 1), index));
					}
				}
			}
		}
		return img;
	}

	sf::Image GetNametable(uint8_t nb)
	{
		sf::Image img;
		img.create(256, 240);
		for (unsigned int i = 0; i < 30; i++)
		{
			for (unsigned int j = 0; j < 32; j++)
			{
				for (unsigned int k = 0; k < 8; k++)
				{
					for (unsigned int l = 0; l < 8; l++)
					{
						uint8_t pix_x = j * 8 + l, pix_y = i * 8 + k;

						uint8_t palette = 0;
						uint8_t paletteIndex = 0;
						uint16_t nametableBase = NametableGetBase(nb);
						bool patternTableHalf = REG_GET_FLAG(PPU_CTRL, PPU_CTRL_BG_PATTERN_TABLE_ADDRESS);
						uint8_t tile = NametableGetTile(j, i, nametableBase);

						uint8_t row_lo = GetRowFromTile(0, patternTableHalf, tile, k);
						uint8_t row_hi = GetRowFromTile(1, patternTableHalf, tile, k);

						uint8_t paletteIndex_hi = REG_GET_FLAG(row_hi, 7 - l),
								paletteIndex_lo = REG_GET_FLAG(row_lo, 7 - l);

						uint8_t attributeTileX = pix_x / 32, attributeTileY = pix_y / 32;
						uint8_t smallAttributeTileX = pix_x / 16, smallAttributeTileY = pix_y / 16;

						uint8_t attributeByte = PPU_readMemory1B(nametableBase + 0x3c0 + attributeTileY * 8 + attributeTileX);

						uint8_t palette_bottomRight = (attributeByte >> 6),
								palette_bottomLeft = (attributeByte >> 4) & 0b11,
								palette_topRight = (attributeByte >> 2) & 0b11,
								palette_topLeft = attributeByte & 0b11;

						if (smallAttributeTileY & 1)
						{
							if (smallAttributeTileX & 1)
								palette = palette_bottomRight;
							else
								palette = palette_bottomLeft;
						}
						else
						{
							if (smallAttributeTileX & 1)
								palette = palette_topRight;
							else
								palette = palette_topLeft;
						}

						paletteIndex = (paletteIndex_hi << 1) | paletteIndex_lo;
						if (paletteIndex == 0)
							img.setPixel(pix_x, pix_y, NESColorToRGB(PPU_readMemory1B(0x3f00)));
						else
							img.setPixel(pix_x, pix_y, GetColorFromPalette(palette, Background, paletteIndex));
					}
				}
			}
		}
		return img;
	}

	uint8_t NametableGetTile(uint8_t tileX, uint8_t tileY, uint16_t nametableBase)
	{
		return PPU_readMemory1B(nametableBase + tileY * 32 + tileX);
	}

	uint16_t NametableGetBase(uint8_t nametable)
	{
		return 0x2000 + 0x400 * nametable;
	}

	sf::Color AttenuateColor(sf::Color color, bool Red, bool Green, bool Blue)
	{
		if (Red)
			color.r = uint8_t(0.816328 * color.r);
		if (Green)
			color.g = uint8_t(0.816328 * color.g);
		if (Blue)
			color.b = uint8_t(0.816328 * color.b);
		return color;
	}

	uint8_t* OAMGetByte(uint8_t address)
	{
		return ((uint8_t*) &OAM[0] + (address & 0xff));
	}

	uint8_t* OAM2GetByte(uint8_t address)
	{
		return ((uint8_t*)&OAM2[0] + (address % 32));
	}

	void copyOAMEntryToOAM2(uint8_t entry)
	{
		if (OAM2Size < 8)
		{
			OAM2[OAM2Size] = OAM[entry];
			//if(OAM2[OAM2Size].y < 255)
			//	OAM2[OAM2Size].y++;
			OAM2Size++;
		}
		else // TODO: Add the bug
		{
			REG_SET_FLAG_1(PPU_STATUS, PPU_STATUS_SPRITE_OVERFLOW);
		}
	}

	void RenderBGPixel(uint8_t& colorCode, bool& solidBG)
	{
		if (REG_GET_FLAG(PPU_MASK, PPU_MASK_SHOW_BG))
		{
			//uint16_t nametableBase = 0x2000 + 0x400 * (nametableY * 2 + nametableX);

			uint8_t x_pos = x + 8 * LOOPY_GET_COARSE_X(v), y_pos = LOOPY_GET_FINE_Y(v) + 8 * LOOPY_GET_COARSE_Y(v);

			//uint8_t tileX = x_pos / 8, tileY = y_pos / 8;
			uint8_t attributeTileX = x_pos / 32, attributeTileY = y_pos / 32;
			uint8_t smallAttributeTileX = x_pos / 16, smallAttributeTileY = y_pos / 16;

			uint8_t attributeByte =
				PPU_readMemory1B(0x23c0 | (v & 0x0c00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07));

			uint8_t palette_bottomRight = (attributeByte >> 6),
					palette_bottomLeft = (attributeByte >> 4) & 0b11,
					palette_topRight = (attributeByte >> 2) & 0b11,
					palette_topLeft = attributeByte & 0b11;

			uint8_t palette;
			if (smallAttributeTileY & 1)
			{
				if (smallAttributeTileX & 1)
					palette = palette_bottomRight;
				else
					palette = palette_bottomLeft;
			}
			else
			{
				if (smallAttributeTileX & 1)
					palette = palette_topRight;
				else
					palette = palette_topLeft;
			}

			uint8_t bgTile = PPU_readMemory1B(0x2000 | (v & 0x0fff));

			//colorCode = v & 0x0fff;
			//return;

			//colorCode = (LOOPY_GET_NAMETABLE(v) + 0x22);
			//return;

			//colorCode = (tileX | (tileY << 5));
			//return;

			bool patternTable = REG_GET_FLAG(PPU_CTRL, PPU_CTRL_BG_PATTERN_TABLE_ADDRESS);

			uint8_t row_lo = GetRowFromTile(0, patternTable, bgTile, LOOPY_GET_FINE_Y(v)),
				row_hi = GetRowFromTile(1, patternTable, bgTile, LOOPY_GET_FINE_Y(v));

			uint8_t paletteIndex_lo = REG_GET_FLAG(row_lo, 7 - x),
				paletteIndex_hi = REG_GET_FLAG(row_hi, 7 - x);
			uint8_t paletteIndex = (paletteIndex_hi << 1) | paletteIndex_lo;

			if (paletteIndex != 0)
			{
				colorCode = GetColorCodeFromPalette(palette, Background, paletteIndex);

				solidBG = true;

				//colorCode = LOOPY_GET_NAMETABLE(v);
			}

			if (settings.debugBGPalette)
				colorCode = palette + 0x11;
		}
	}

	void RenderSpritePixel(uint8_t& colorCode, bool& solidSpr)
	{
		if (REG_GET_FLAG(PPU_MASK, PPU_MASK_SHOW_SPRITES))
		{
			uint8_t pos_x = (uint8_t)PPU_cycles, pos_y = (uint8_t)PPU_scanline;

			for (unsigned int i = 0; i < 8; i++)
			{
				OAMEntry sprite = OAM2[7 - i];
				if (REG_GET_FLAG(PPU_CTRL, PPU_CTRL_SPRITE_SIZE)) // 8x16
				{
					if (pos_x >= sprite.x && pos_y >= sprite.y &&
						pos_x < (sprite.x + 8) && pos_y < (sprite.y + 16))
					{
						uint8_t pix_x = pos_x - sprite.x, pix_y = pos_y - sprite.y;

						bool half = pix_y / 8; // 0 - top; 1 - bottom

						pix_y %= 8;

						//colorCode = pix_y * 8 + pix_x;
						//return;

						bool patternTableHalf = (sprite.tileIndex & 1);

						bool flipX = REG_GET_FLAG(sprite.attributes, OAM_ATTRIBUTES_FLIP_HORIZONTALLY);
						bool flipY = REG_GET_FLAG(sprite.attributes, OAM_ATTRIBUTES_FLIP_VERTICALLY);

						half ^= flipY;

						uint8_t tileRow_lo = GetRowFromTile(0, patternTableHalf, (sprite.tileIndex & 0b11111110) + half, flipY ? (7 - pix_y) : pix_y);
						uint8_t tileRow_hi = GetRowFromTile(1, patternTableHalf, (sprite.tileIndex & 0b11111110) + half, flipY ? (7 - pix_y) : pix_y);

						uint8_t palette = (sprite.attributes & 0b11);
						uint8_t paletteIndex_lo = REG_GET_FLAG(tileRow_lo, flipX ? pix_x : (7 - pix_x)),
							paletteIndex_hi = REG_GET_FLAG(tileRow_hi, flipX ? pix_x : (7 - pix_x));
						uint8_t paletteIndex = (paletteIndex_hi << 1) | paletteIndex_lo;

						if (paletteIndex != 0)
						{
							solidSpr = true;
							colorCode = GetColorCodeFromPalette(palette, Sprite, paletteIndex);
							//colorCode = palette;
						}
					}
				}
				else // 8x8
				{
					if (pos_x >= sprite.x && pos_y >= sprite.y &&
						pos_x < (sprite.x + 8) && pos_y < (sprite.y + 8))
					{
						uint8_t pix_x = pos_x - sprite.x, pix_y = pos_y - sprite.y;

						//colorCode = pix_y * 8 + pix_x;
						//return;

						bool patternTableHalf = REG_GET_FLAG(PPU_CTRL, PPU_CTRL_SPRITE_PATTERN_TABLE_ADDRESS_8x8);

						bool flipX = REG_GET_FLAG(sprite.attributes, OAM_ATTRIBUTES_FLIP_HORIZONTALLY);
						bool flipY = REG_GET_FLAG(sprite.attributes, OAM_ATTRIBUTES_FLIP_VERTICALLY);

						uint8_t tileRow_lo = GetRowFromTile(0, patternTableHalf, sprite.tileIndex, flipY ? (7 - pix_y) : pix_y);
						uint8_t tileRow_hi = GetRowFromTile(1, patternTableHalf, sprite.tileIndex, flipY ? (7 - pix_y) : pix_y);

						uint8_t palette = (sprite.attributes & 0b11);
						uint8_t paletteIndex_lo = REG_GET_FLAG(tileRow_lo, flipX ? pix_x : (7 - pix_x)),
							paletteIndex_hi = REG_GET_FLAG(tileRow_hi, flipX ? pix_x : (7 - pix_x));
						uint8_t paletteIndex = (paletteIndex_hi << 1) | paletteIndex_lo;

						if (paletteIndex != 0)
						{
							solidSpr = true;
							colorCode = GetColorCodeFromPalette(palette, Sprite, paletteIndex);
							//colorCode = palette;
						}
					}
				}
			}
		}
	}

	void RenderPixel()
	{
		uint8_t colorCode = PPU_readMemory1B(0x3f00);

		if (settings.debugBGPalette)
			colorCode = 0;

		bool solidBG = false, solidSpr = false;

		if (OAM2Size != 0)
		{
			if (REG_GET_FLAG(OAM2[0].attributes, OAM_ATTRIBUTES_SPRITE_PRIORITY))
			{
				RenderSpritePixel(colorCode, solidSpr);
				RenderBGPixel(colorCode, solidBG);
			}
			else
			{
				RenderBGPixel(colorCode, solidBG);
				RenderSpritePixel(colorCode, solidSpr);
			}
		}
		else
			RenderBGPixel(colorCode, solidBG);

		if (solidBG && solidSpr && PPU_cycles != 255 && !sprite0HitAlreadyHappened)
		{
			sprite0HitAlreadyHappened = true;
			REG_SET_FLAG_1(PPU_STATUS, PPU_STATUS_SPRITE_0_HIT);
		}

		bool grayscale = REG_GET_FLAG(PPU_MASK, PPU_MASK_GRAYSCALE);

		uint8_t chroma = (colorCode & 0x0f);
		uint8_t luma = (colorCode >> 4);
		if (grayscale)
			colorCode &= 0x30; // colorCode = (luma << 4);

		sf::Color color = NESColorToRGB(colorCode);

		bool attenuateRed, attenuateGreen, attenuateBlue;
		attenuateRed = REG_GET_FLAG(PPU_MASK, PPU_MASK_EMPHASIZE_GREEN) |
			REG_GET_FLAG(PPU_MASK, PPU_MASK_EMPHASIZE_BLUE);
		attenuateGreen = REG_GET_FLAG(PPU_MASK, PPU_MASK_EMPHASIZE_RED) |
			REG_GET_FLAG(PPU_MASK, PPU_MASK_EMPHASIZE_BLUE);
		attenuateBlue = REG_GET_FLAG(PPU_MASK, PPU_MASK_EMPHASIZE_RED) |
			REG_GET_FLAG(PPU_MASK, PPU_MASK_EMPHASIZE_GREEN);
		if (chroma < 0x0d)
			color = AttenuateColor(color, attenuateRed, attenuateGreen, attenuateBlue);

		if (settings.emulateDifferentialPhaseDistortion)
			color = RotateHue(color, 5.f * (colorCode >> 4));

		screen2.setPixel(PPU_cycles, PPU_scanline, color);
	}

	void cycle();
	void PPU_cycle();
	void INTERRUPT(uint16_t returnAddress, uint16_t isrAddress, bool B_FLAG);
	void NMI();
	std::string CPU_cycle();

public:
	uint8_t CPU_memory[64 * KB];
	uint8_t PPU_memory[16 * KB];
	uint8_t A, X, Y, SR, S;
	uint16_t PC;

	uint16_t CPU_cycles;
	uint16_t PPU_cycles;
	uint8_t cycleCounter;
	uint16_t PPU_scanline;
	Mapper mapper;
	Mirroring mirroring;
	bool PRGRAM;

	sf::Image screen, screen2;
	bool frameFinished;
	uint8_t NESPalette[64 * 3];

	std::string log;
	uint32_t logLines;

	bool w; // write latch
	uint16_t v; // VRAM address
	uint16_t t; // temp VRAM address
	uint8_t ppuReadBuffer;
	uint8_t x; // fine x scroll (3 bits)
	bool sprite0HitAlreadyHappened;

	OAMEntry OAM[64];
	OAMEntry OAM2[8];
	uint8_t OAM2Size;
	uint8_t OAMAddress;

	bool controllerLatch1;
	uint8_t controller1ShiftRegister;

	bool stopCPU;

	EmulationSettings settings;
};