#pragma once

#include <sstream>
#include <random>
#include <algorithm>
#include <SFML/Graphics.hpp>

namespace
{
	std::default_random_engine re((unsigned int)time(0));
	//std::uniform_int_distribution<int> randomBool{ 0, 1 };
	std::uniform_int_distribution<unsigned int> randomColor{ 0, 63 };
	std::uniform_int_distribution<unsigned int> randomByte{ 0, 255 };
	std::uniform_int_distribution<unsigned int> randomAlignment{ 0, 2 };

	std::string HEX(uint32_t value)
	{
		std::stringstream ss;
		ss << std::hex << (int)value;
		return ss.str();
	}

	std::string HEX_1B(uint8_t value)
	{
		std::stringstream ss;
		ss << std::hex << (int)value;
		std::string str = ss.str();
		return (value < 0x10 ? "0" + str : str);
	}

	std::string HEX_2B(uint16_t value)
	{
		return HEX_1B(value >> 8) + HEX_1B(value & 0xff);
	}

#define KB 1024
	//#define MB (KB * 1024)

#define PI 3.14159265358979323846f

#define REG_GET_FLAG(reg, bit)           ((reg >> (bit)) & 1)
#define REG_SET_FLAG_1(reg, bit)         reg |= (1 << (bit))
#define REG_SET_FLAG_0(reg, bit)         reg &= ~((uint8_t)(1 << (bit)))
#define REG_TOGGLE_FLAG(reg, bit)        reg ^= (1 << (bit))
#define REG_SET_FLAG(reg, bit, value)    REG_SET_FLAG_0(reg, bit); reg |= (((value) & 1) << (bit))

#define GET_FLAG(bit)           REG_GET_FLAG(SR, bit)
#define SET_FLAG_1(bit)         REG_SET_FLAG_1(SR, bit)
#define SET_FLAG_0(bit)         REG_SET_FLAG_0(SR, bit)
#define TOGGLE_FLAG(bit)        REG_TOGGLE_FLAG(SR, bit)
#define SET_FLAG(bit, value)    REG_SET_FLAG(SR, bit, value)

#define FLAG_C 0
#define FLAG_Z 1
#define FLAG_I 2
#define FLAG_D 3
#define FLAG_B 4
#define FLAG_1 5
#define FLAG_V 6
#define FLAG_N 7

#define FLAG_CARRY				FLAG_C
#define FLAG_ZERO				FLAG_Z
#define FLAG_INTERRUPT_DISABLE	FLAG_I
#define FLAG_DECIMAL			FLAG_D
#define FLAG_OVERFLOW			FLAG_V
#define FLAG_NEGATIVE			FLAG_N

#define NMI_VECTOR				0xfffa	
#define RESET_VECTOR			0xfffc
#define IRQ_VECTOR				0xfffe
#define BRK_VECTOR				IRQ_VECTOR

#define PPU_STATUS_SPRITE_OVERFLOW 5
#define PPU_STATUS_SPRITE_0_HIT    6
#define PPU_STATUS_VBLANK          7

#define PPU_MASK_GRAYSCALE         0
#define PPU_MASK_SHOW_BG_LEFT      1
#define PPU_MASK_SHOW_SPRITES_LEFT 2
#define PPU_MASK_SHOW_BG           3
#define PPU_MASK_SHOW_SPRITES      4
#define PPU_MASK_EMPHASIZE_RED     5
#define PPU_MASK_EMPHASIZE_GREEN   6
#define PPU_MASK_EMPHASIZE_BLUE    7

#define PPU_CTRL_NAMETABLE_ADDRESS_X					0
#define PPU_CTRL_NAMETABLE_ADDRESS_Y					1
#define PPU_CTRL_VRAM_ADDRESS_INCREMENT_DIRECTION		2
#define PPU_CTRL_SPRITE_PATTERN_TABLE_ADDRESS_8x8		3
#define PPU_CTRL_BG_PATTERN_TABLE_ADDRESS				4
#define PPU_CTRL_SPRITE_SIZE						    5
#define PPU_CTRL_MASTER_SLAVE						    6
#define PPU_CTRL_NMI									7

#define LOOPY_GET_COARSE_X(reg)		(reg & 0b11111)
#define LOOPY_GET_COARSE_Y(reg)		((reg >> 5) & 0b11111)
#define LOOPY_GET_NAMETABLE(reg)	((reg >> 10) & 0b11)
#define LOOPY_GET_NAMETABLE_X(reg)	((reg >> 10) & 1)
#define LOOPY_GET_NAMETABLE_Y(reg)	((reg >> 11) & 1)
#define LOOPY_GET_FINE_Y(reg)		((reg >> 12) & 0b111)

#define LOOPY_SET_COARSE_X(reg, val)		reg &= ~((uint16_t)          0b11111);	reg |= ((val) & 0b11111)
#define LOOPY_SET_COARSE_Y(reg, val)		reg &= ~((uint16_t)     0b1111100000);	reg |= (((uint16_t)(val) & 0b11111) << 5)
#define LOOPY_SET_NAMETABLE(reg, val)		reg &= ~((uint16_t)   0b110000000000);	reg |= (((uint16_t)(val) & 0b11) << 10)
#define LOOPY_SET_NAMETABLE_X(reg, val)		reg &= ~((uint16_t)1 << 10);	reg |= (((uint16_t)(val) & 1) << 10)
#define LOOPY_SET_NAMETABLE_Y(reg, val)		reg &= ~((uint16_t)1 << 11);	reg |= (((uint16_t)(val) & 1) << 11)
#define LOOPY_SET_FINE_Y(reg, val)			reg &= ~((uint16_t)0b111000000000000);	reg |= (((uint16_t)(val) & 0b111) << 12)

#define OAM_ATTRIBUTES_SPRITE_PRIORITY		5 // (0: in front of background; 1: behind background)
#define OAM_ATTRIBUTES_FLIP_HORIZONTALLY	6
#define OAM_ATTRIBUTES_FLIP_VERTICALLY		7

#define updateRegistersText() registers.setString("A:  #$" + HEX_1B(emu.cpu.A) + "\n" + \
												  "X:  #$" + HEX_1B(emu.cpu.X) + "\n" + \
												  "Y:  #$" + HEX_1B(emu.cpu.Y) + "\n" + \
												  "SR: #$" + HEX_1B(emu.cpu.SR) + "\n" + \
												  "S:  #$" + HEX_1B(emu.cpu.S) + "\n" + \
												  "PC: #$" + HEX_2B(emu.cpu.PC));

#define HANDLE_KEY(var, key) 		if (sf::Keyboard::isKeyPressed(sf::Keyboard::key)) \
										var++;									 \
									else                                             \
										var = 0;
	
	//uint8_t NESPalette[64 * 3] =
	//{
	//	124,124,124,
	//	0,0,252,	
	//	0,0,188,
	//	68,40,188,
	//	148,0,132,
	//	168,0,32,
	//	168,16,0,
	//	136,20,0,
	//	80,48,0,
	//	0,120,0,
	//	0,104,0,
	//	0,88,0,
	//	0,64,88,
	//	0,0,0,
	//	0,0,0,
	//	0,0,0,
	//	188,188,188,
	//	0,120,248,
	//	0,88,248,
	//	104,68,252,
	//	216,0,204,
	//	228,0,88,
	//	248,56,0,
	//	228,92,16, // 153,78,0,
	//	172,124,0,
	//	0,184,0,
	//	0,168,0,
	//	0,168,68,
	//	0,136,136,
	//	0,0,0,
	//	0,0,0,
	//	0,0,0,
	//	248,248,248,
	//	60,188,252,
	//	104,136,252,
	//	152,120,248,
	//	248,120,248,
	//	248,88,152,
	//	248,120,88,
	//	252,160,68,
	//	248,184,0,
	//	184,248,24,
	//	88,216,84,
	//	88,248,152,
	//	0,232,216,
	//	120,120,120,
	//	0,0,0,
	//	0,0,0,
	//	252,252,252,
	//	164,228,252,
	//	184,184,248,
	//	216,184,248,
	//	248,184,248,
	//	248,164,192,
	//	240,208,176,
	//	252,224,168,
	//	248,216,120,
	//	216,248,120,
	//	184,248,184,
	//	184,248,216,
	//	0,252,252,
	//	248,216,248,
	//	0,0,0,
	//	0,0,0
	//};

	//const uint8_t LUT_2C04_1[64] = 
	//{
	//	0x35,0x23,0x16,0x22,0x1C,0x09,0x1D,0x15,0x20,0x00,0x27,0x05,0x04,0x28,0x08,0x20,
	//	0x21,0x3E,0x1F,0x29,0x3C,0x32,0x36,0x12,0x3F,0x2B,0x2E,0x1E,0x3D,0x2D,0x24,0x01,
	//	0x0E,0x31,0x33,0x2A,0x2C,0x0C,0x1B,0x14,0x2E,0x07,0x34,0x06,0x13,0x02,0x26,0x2E,
	//	0x2E,0x19,0x10,0x0A,0x39,0x03,0x37,0x17,0x0F,0x11,0x0B,0x0D,0x38,0x25,0x18,0x3A
	//};

	inline sf::Color RotateHue(const sf::Color& in, const float angle)
	{
		sf::Color out;
		const float cosA = cos(angle * 3.14159265f / 180);
		const float sinA = sin(angle * 3.14159265f / 180);

		float matrix[3][3] = { {cosA + (1.f - cosA) / 3.f, 1.f / 3.f * (1.f - cosA) - sqrtf(1.f / 3.f) * sinA, 1.f / 3.f * (1.f - cosA) + sqrtf(1.f / 3.f) * sinA},
			{1.f / 3.f * (1.f - cosA) + sqrtf(1.f / 3.f) * sinA, cosA + 1.f / 3.f * (1.f - cosA), 1.f / 3.f * (1.f - cosA) - sqrtf(1.f / 3.f) * sinA},
			{1.f / 3.f * (1.f - cosA) - sqrtf(1.f / 3.f) * sinA, 1.f / 3.f * (1.f - cosA) + sqrtf(1.f / 3.f) * sinA, cosA + 1.f / 3.f * (1.f - cosA)} };

		out.r = (uint8_t)std::clamp(in.r * matrix[0][0] + in.g * matrix[0][1] + in.b * matrix[0][2], 0.f, 255.f);
		out.g = (uint8_t)std::clamp(in.r * matrix[1][0] + in.g * matrix[1][1] + in.b * matrix[1][2], 0.f, 255.f);
		out.b = (uint8_t)std::clamp(in.r * matrix[2][0] + in.g * matrix[2][1] + in.b * matrix[2][2], 0.f, 255.f);

		return out;
	}

	inline float sqr(float x)
	{
		return x * x;
	}

	enum PaletteType
	{
		Background = 0,
		Sprite	   = 1
	};

	enum Mapper
	{
		Mapper0_NROM_128,
		Mapper0_NROM_256,
		Mapper1_MMC1,
		Other
	};

	enum Mirroring
	{
		Horizontal = 0,
		Vertical = 1
	};

	struct iNESHeader // 16B
	{
		uint8_t NES[4];		// $4E $45 $53 $1A
		uint8_t PRGROMSize; // Size of PRG ROM in 16 KB units
		uint8_t CHRROMSize; // Size of CHR ROM in 8 KB units (value 0 means the board uses CHR RAM)
		uint8_t flags6;		// Mapper, mirroring, battery, trainer
		uint8_t flags7;		// Mapper, VS/Playchoice, NES 2.0
		uint8_t flags8;		// PRG-RAM size (rarely used extension)
		uint8_t flags9;		// TV system (rarely used extension)
		uint8_t flags10;	// TV system, PRG-RAM presence (unofficial, rarely used extension)
		uint8_t padding[5];	// Unused padding (should be filled with zero, but some rippers put their name across bytes 7-15)
	};

	sf::Keyboard::Key keyMap[8] =
	{
		sf::Keyboard::Right,	// RIGHT
		sf::Keyboard::Left,		// LEFT
		sf::Keyboard::Down,		// DOWN
		sf::Keyboard::Up,		// UP
		sf::Keyboard::LShift,	// START
		sf::Keyboard::LControl,	// SELECT
		sf::Keyboard::X,		// B
		sf::Keyboard::C			// A
	};

	struct EmulationSettings
	{
		bool emulateDifferentialPhaseDistortion; 
		bool blockImpossibleInputs;
	};

	enum WindowState
	{
		Screen = 0,
		CPUDebug = 1,
		PPUDebug = 2,
		PPUDebug_Patterntables = 3,
		PPUDebug_Nametables = 4
	};

#define PACK(decl) __pragma(pack(push, 1)) decl __pragma(pack(pop))

	PACK(struct OAMEntry
	{
		uint8_t y;
		uint8_t tileIndex;
		uint8_t attributes;
		uint8_t x;
	};)
}