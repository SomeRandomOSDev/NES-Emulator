#include "PPU_RP2C02G.hpp"

void PPU_RP2C02G::cycle()
{
	if (REG_GET_FLAG(reg_ppu_mask, PPU_MASK_SHOW_BG) || REG_GET_FLAG(reg_ppu_mask, PPU_MASK_SHOW_SPRITES))
	{
		if (cycles == 257)
		{
			LOOPY_SET_COARSE_X(v, LOOPY_GET_COARSE_X(t));
			LOOPY_SET_NAMETABLE_X(v, LOOPY_GET_NAMETABLE_X(t));

			v_2 = v;
			x_2 = x;
		}
		if (cycles >= 280 && cycles <= 304 && scanline == 261) // 280 - 304
		{
			LOOPY_SET_COARSE_Y(v, LOOPY_GET_COARSE_Y(t));
			LOOPY_SET_FINE_Y(v, LOOPY_GET_FINE_Y(t));
			LOOPY_SET_NAMETABLE_Y(v, LOOPY_GET_NAMETABLE_Y(t));
		}

		if (cycles == 256)
			Y_increment();
	}

	if (cycles == 0) // Wrong sprite evaluation
	{
		for (uint8_t i = 0; i < 32; i++)
			*OAM2GetByte(i) = 0xff;
		OAM2Size = 0;
		sprite0InScanline = false;
		for (unsigned int i = 0; i < 64; i++)
		{
			uint8_t entry = 63 - i;
			if (REG_GET_FLAG(reg_ppu_control, PPU_CTRL_SPRITE_SIZE)) // 8x16
			{
				if (scanline >= OAM[entry].y && scanline < (OAM[entry].y + 16))
					copyOAMEntryToOAM2(entry);
			}
			else // 8x8
			{
				if (scanline >= OAM[entry].y && scanline < (OAM[entry].y + 8))
					copyOAMEntryToOAM2(entry);
			}
		}
	}

	if (cycles < 256 && scanline < 240)
		RenderPixel();

	if (scanline == 241 && cycles == 1)
	{
		REG_SET_FLAG_1(reg_ppu_status, PPU_STATUS_VBLANK);
		waitForNMI = true;
	}

	if (scanline == 261 && cycles == 1)
	{
		REG_SET_FLAG_0(reg_ppu_status, PPU_STATUS_VBLANK);
		REG_SET_FLAG_0(reg_ppu_status, PPU_STATUS_SPRITE_0_HIT);
		REG_SET_FLAG_0(reg_ppu_status, PPU_STATUS_SPRITE_OVERFLOW);
	}

	cycles++;
	if (cycles > 340)
	{
		cycles = 0;
		scanline++;
		if (scanline > 261)
		{
			scanline = 0;
			screen = screen2;
			frameFinished = true;
		}
	}

	totalCycles++;
}

void PPU_RP2C02G::RenderBGPixel(uint8_t& colorCode, bool& solidBG)
{
	if (!(!REG_GET_FLAG(reg_ppu_mask, PPU_MASK_SHOW_BG) || (cycles < 8 && !REG_GET_FLAG(reg_ppu_mask, PPU_MASK_SHOW_BG_LEFT))))
	{
		//uint16_t nametableBase = 0x2000 + 0x400 * (nametableY * 2 + nametableX);

		uint8_t x_pos = x_2 + 8 * LOOPY_GET_COARSE_X(v_2),
			y_pos = LOOPY_GET_FINE_Y(v_2) + 8 * LOOPY_GET_COARSE_Y(v_2);

		//uint8_t tileX = x_pos / 8, tileY = y_pos / 8;
		uint8_t attributeTileX = x_pos / 32, attributeTileY = y_pos / 32;
		uint8_t smallAttributeTileX = x_pos / 16, smallAttributeTileY = y_pos / 16;

		uint8_t attributeByte =
			read_1B(0x23c0 | (v_2 & 0x0c00) | ((v_2 >> 4) & 0x38) | ((v_2 >> 2) & 0x07));

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

		uint8_t bgTile = read_1B(0x2000 | (v_2 & 0x0fff));

		//colorCode = v & 0x0fff;
		//return;

		//colorCode = (LOOPY_GET_NAMETABLE(v) + 0x22);
		//return;

		//colorCode = (tileX | (tileY << 5));
		//return;

		bool patternTable = REG_GET_FLAG(reg_ppu_control, PPU_CTRL_BG_PATTERN_TABLE_ADDRESS);

		uint8_t row_lo = GetRowFromTile(0, patternTable, bgTile, LOOPY_GET_FINE_Y(v_2)),
			row_hi = GetRowFromTile(1, patternTable, bgTile, LOOPY_GET_FINE_Y(v_2));

		uint8_t paletteIndex_lo = REG_GET_FLAG(row_lo, 7 - x_2),
			paletteIndex_hi = REG_GET_FLAG(row_hi, 7 - x_2);
		uint8_t paletteIndex = (paletteIndex_hi << 1) | paletteIndex_lo;

		if (paletteIndex != 0)
		{
			colorCode = GetColorCodeFromPalette(palette, Background, paletteIndex);

			solidBG = true;

			//colorCode = LOOPY_GET_NAMETABLE(v);
		}

		//if (settings.debugBGPalette)
		//	colorCode = palette + 0x11;

		x_2++;
		x_2 %= 8;

		if (x_2 == 0)
			coarse_X_increment();
	}
}

void PPU_RP2C02G::RenderSpritePixel(uint8_t& colorCode, bool& solidSpr, bool& spritePriority, bool& sprite0Visible)
{
	sprite0Visible = false;
	if (!(!REG_GET_FLAG(reg_ppu_mask, PPU_MASK_SHOW_SPRITES) || (cycles < 8 && !REG_GET_FLAG(reg_ppu_mask, PPU_MASK_SHOW_SPRITES_LEFT))))
	{
		uint8_t pos_x = (uint8_t)cycles, pos_y = (uint8_t)scanline;

		for (unsigned int i = 0; i < 8; i++)
		{
			OAMEntry sprite = OAM2[i];
			if (REG_GET_FLAG(reg_ppu_control, PPU_CTRL_SPRITE_SIZE)) // 8x16
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
						spritePriority = REG_GET_FLAG(OAM2[i].attributes, OAM_ATTRIBUTES_SPRITE_PRIORITY);
					}

					//if (sprite.sprite0)
					//	sprite0Visible = true;
					if (sprite0InScanline && (i == sprite0LocationInOAM2))
						sprite0Visible = true;
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

					bool patternTableHalf = REG_GET_FLAG(reg_ppu_control, PPU_CTRL_SPRITE_PATTERN_TABLE_ADDRESS_8x8);

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

						spritePriority = REG_GET_FLAG(OAM2[i].attributes, OAM_ATTRIBUTES_SPRITE_PRIORITY);
					}

					//if (sprite.sprite0)
					//	sprite0Visible = true;
					if (sprite0InScanline && (i == sprite0LocationInOAM2))
						sprite0Visible = true;
				}
			}
		}
	}
}

void PPU_RP2C02G::RenderPixel()
{
	uint8_t colorCode;
	uint8_t bgColor = read_1B(0x3f00);
	uint8_t spriteColor = 0xff;

	bool solidBG = false, solidSpr = false;

	bool sprite0Visible = false;

	RenderBGPixel(bgColor, solidBG);
	bool spritePriority = false;
	RenderSpritePixel(spriteColor, solidSpr, spritePriority, sprite0Visible);

	if (spritePriority)
	{
		if (solidBG)
			colorCode = bgColor;
		else
			colorCode = spriteColor;
	}
	else
	{
		if (spriteColor == 0xff)
			colorCode = bgColor;
		else
			colorCode = spriteColor;
	}

	bool grayscale = REG_GET_FLAG(reg_ppu_mask, PPU_MASK_GRAYSCALE);

	uint8_t chroma = (colorCode & 0x0f);
	uint8_t luma = (colorCode >> 4);
	if (grayscale)
		colorCode &= 0x30; // colorCode = (luma << 4);

	sf::Color color = NESColorToRGB(colorCode);

	bool attenuateRed, attenuateGreen, attenuateBlue;
	attenuateRed = REG_GET_FLAG(reg_ppu_mask, PPU_MASK_EMPHASIZE_GREEN) |
		REG_GET_FLAG(reg_ppu_mask, PPU_MASK_EMPHASIZE_BLUE);
	attenuateGreen = REG_GET_FLAG(reg_ppu_mask, PPU_MASK_EMPHASIZE_RED) |
		REG_GET_FLAG(reg_ppu_mask, PPU_MASK_EMPHASIZE_BLUE);
	attenuateBlue = REG_GET_FLAG(reg_ppu_mask, PPU_MASK_EMPHASIZE_RED) |
		REG_GET_FLAG(reg_ppu_mask, PPU_MASK_EMPHASIZE_GREEN);
	if (chroma < 0x0d)
		color = AttenuateColor(color, attenuateRed, attenuateGreen, attenuateBlue);

	if (settings.emulateDifferentialPhaseDistortion)
		color = RotateHue(color, 5.f * luma);

	screen2.setPixel(cycles, scanline, color);

	if (solidBG && solidSpr && sprite0Visible && (cycles >= 2 && cycles < 255))
	{
		REG_SET_FLAG_1(reg_ppu_status, PPU_STATUS_SPRITE_0_HIT);
		//colorCode = 0x24;
	}
}