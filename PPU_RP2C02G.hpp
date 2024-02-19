#pragma once

#include <stdint.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <SFML/Graphics.hpp>

#include "util.hpp"

class PPU_RP2C02G
{
public:
	PPU_RP2C02G(std::shared_ptr<Mapper>& _mapper, EmulationSettings& _settings) : mapper(_mapper), settings(_settings)
	{
		screen.create(256, 240);
		screen2.create(256, 240);

		powerUp();
	}

	void powerUp()
	{
		memset(&NESPalette[0], 0, 64 * 3);
		memset(&memory_nametables[0], 0, 0x1000);
		memset(&memory_paletteRAM[0], 0, 32);
		totalCycles = randomAlignment(re);
		cycles = 0;
		scanline = 0;
		frameFinished = false;
		w = false;
		t = 0x0000;
		v = 0x0000;
		readBuffer = 0x00;
		x = 0;
		OAMAddress = 0;

		waitForNMI = false;

		reg_ppu_control = 0x00;
		reg_ppu_mask = 0x00;
		reg_ppu_status = 0b10100000;
	}

	void reset()
	{
		cycles = 0;
		scanline = 0;
		frameFinished = false;
		w = false;
		t = 0x0000;
		readBuffer = 0x00;
		x = 0;

		waitForNMI = false;

		reg_ppu_control = 0x00;
		reg_ppu_mask = 0x00;
		reg_ppu_status &= 0b10000000;
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

	uint8_t* OAMGetByte(uint8_t address)
	{
		return ((uint8_t*)&OAM[0] + (address & 0xff));
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
			if (OAM2[OAM2Size].y < 255)
				OAM2[OAM2Size].y++;

			if (entry == 0)
			{
				sprite0InScanline = true;
				sprite0LocationInOAM2 = OAM2Size;
			}
			OAM2Size++;
		}
		else // TODO: Add the bug
		{
			REG_SET_FLAG_1(reg_ppu_status, PPU_STATUS_SPRITE_OVERFLOW);
		}
	}

	uint16_t remapAddress(uint16_t address)
	{
		address &= 0x3fff;

		// Nametable mirroring
		if (mapper->mirroring == Horizontal)
		{
			if ((address >= 0x2400 && address < 0x2800) || (address >= 0x2c00 && address < 0x3000))
				address -= 0x400;
		}
		else if (mapper->mirroring == Vertical)
		{
			if (address >= 0x2800 && address < 0x3000)
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

	uint8_t read_1B(uint16_t address)
	{
		address = remapAddress(address);

		if (address < 0x2000)
			return mapper->PPU_read_1B(address);
		else if (address < 0x3000)
			return memory_nametables[address - 0x2000];
		else
			return memory_paletteRAM[address - 0x3f00];
	}

	void write_1B(uint16_t address, uint8_t value)
	{
		address = remapAddress(address);

		if (address < 0x2000)
			mapper->PPU_write_1B(address, value);
		else if (address < 0x3000)
			memory_nametables[address - 0x2000] = value;
		else
			memory_paletteRAM[address - 0x3f00] = value;
	}

	uint16_t read_2B(uint16_t address)
	{
		return ((uint16_t)read_1B(address + 1) << 8) | read_1B(address);
	}

	void write_2B(uint16_t address, uint16_t value)
	{
		write_1B(address, value & 0xff);
		write_1B(address + 1, value >> 8);
	}

	inline sf::Color NESColorToRGB(uint8_t colorCode)
	{
		return sf::Color(NESPalette[(colorCode % 64) * 3],
			NESPalette[(colorCode % 64) * 3 + 1],
			NESPalette[(colorCode % 64) * 3 + 2]);
	}

	inline sf::Color AttenuateColor(sf::Color color, bool Red, bool Green, bool Blue)
	{
		if (Red)
			color.r = uint8_t(0.816328 * color.r);
		if (Green)
			color.g = uint8_t(0.816328 * color.g);
		if (Blue)
			color.b = uint8_t(0.816328 * color.b);
		return color;
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
						bool patternTableHalf = REG_GET_FLAG(reg_ppu_control, PPU_CTRL_BG_PATTERN_TABLE_ADDRESS);
						uint8_t tile = NametableGetTile(j, i, nametableBase);

						uint8_t row_lo = GetRowFromTile(0, patternTableHalf, tile, k);
						uint8_t row_hi = GetRowFromTile(1, patternTableHalf, tile, k);

						uint8_t paletteIndex_hi = REG_GET_FLAG(row_hi, 7 - l),
							paletteIndex_lo = REG_GET_FLAG(row_lo, 7 - l);

						uint8_t attributeTileX = pix_x / 32, attributeTileY = pix_y / 32;
						uint8_t smallAttributeTileX = pix_x / 16, smallAttributeTileY = pix_y / 16;

						uint8_t attributeByte = read_1B(nametableBase + 0x3c0 + attributeTileY * 8 + attributeTileX);

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
							img.setPixel(pix_x, pix_y, NESColorToRGB(read_1B(0x3f00)));
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
		return read_1B(nametableBase + tileY * 32 + tileX);
	}

	uint16_t NametableGetBase(uint8_t nametable)
	{
		return 0x2000 + 0x400 * nametable;
	}

	uint16_t GetRowAddressFromTile(bool bitPlane, bool patternTableHalf, uint8_t tileNumber, uint8_t row)
	{
		return (patternTableHalf << 0xc) | (tileNumber << 4) | (bitPlane << 3) | (row & 0b111);
	}

	uint8_t GetRowFromTile(bool bitPlane, bool patternTableHalf, uint8_t tileNumber, uint8_t row)
	{
		return read_1B(GetRowAddressFromTile(bitPlane, patternTableHalf, tileNumber, row));
	}

	uint8_t GetColorCodeFromPalette(uint8_t paletteNumber, PaletteType paletteType, uint8_t index)
	{
		//  43210
		//	|||||
		//	|||++ - Pixel value from tile data
		//	|++-- - Palette number from attribute table or OAM
		//	+---- - Background / Sprite select

		uint16_t address = 0x3f00 | ((paletteType << 4) | ((paletteNumber & 0b11) << 2) | (index & 0b11));

		return read_1B(address);
	}

	sf::Color GetColorFromPalette(uint8_t paletteNumber, PaletteType paletteType, uint8_t index)
	{
		//  43210
		//	|||||
		//	|||++ - Pixel value from tile data
		//	|++-- - Palette number from attribute table or OAM
		//	+---- - Background / Sprite select

		uint16_t address = 0x3f00 | ((int(paletteType) << 4) | ((paletteNumber & 0b11) << 2) | (index & 0b11));

		return NESColorToRGB(read_1B(address));
	}

	void coarse_X_increment()
	{
		//if (RENDERING_ENABLED)
		if (REG_GET_FLAG(reg_ppu_mask, PPU_MASK_SHOW_BG) || REG_GET_FLAG(reg_ppu_mask, PPU_MASK_SHOW_SPRITES))
		{
			if ((v & 0x001F) == 31)
			{
				v &= ~0x001F;
				v ^= 0x0400;
			}
			else
			{
				v += 1;
			}

			v_2 = v;
			//x += 8;
			//x %= 8;
			//x_2 = x;
			//x = 0;
		}
	}

	void Y_increment()
	{
		//if (RENDERING_ENABLED)
		if (REG_GET_FLAG(reg_ppu_mask, PPU_MASK_SHOW_BG) || REG_GET_FLAG(reg_ppu_mask, PPU_MASK_SHOW_SPRITES))
		{
			if ((v & 0x7000) != 0x7000)
			{
				v += 0x1000;
			}
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

			v_2 = v;
			x_2 = x;
		}
	}

	void cycle();
	void RenderBGPixel(uint8_t& colorCode, bool& solidBG);
	void RenderSpritePixel(uint8_t& colorCode, bool& solidSpr, bool& spritePriority, bool& sprite0Visible);
	void RenderPixel();

public:
	uint8_t memory_nametables[0x1000];
	uint8_t memory_paletteRAM[32];

	sf::Image screen, screen2;
	bool frameFinished;
	uint8_t NESPalette[64 * 3];

	uint32_t totalCycles;
	uint16_t cycles;
	uint16_t scanline;
	bool w; // write latch
	uint16_t v; // VRAM address/Scrolling
	uint16_t v_2; // Scrolling
	uint16_t t; // temp VRAM address
	uint8_t readBuffer;
	uint8_t x; // fine x scroll (3 bits)
	uint8_t x_2;

	OAMEntry OAM[64];
	OAMEntry OAM2[8];
	uint8_t OAM2Size;
	uint8_t OAMAddress;
	bool sprite0InScanline;
	uint8_t sprite0LocationInOAM2;

	uint8_t reg_ppu_control;
	uint8_t reg_ppu_mask;
	uint8_t reg_ppu_status;

	bool waitForNMI;

	std::shared_ptr<Mapper>& mapper;

	EmulationSettings& settings;
};