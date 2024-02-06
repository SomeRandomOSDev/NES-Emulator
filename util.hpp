#pragma once

#include <sstream>
#include <random>

namespace
{
	std::default_random_engine re((unsigned int)time(0));
	std::uniform_int_distribution<int> randomBool{ 0, 1 };

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

#define B  1
#define KB (B * 1024)
	//#define MB (KB * 1024)

#define LOG_ADD_LINE(str)		log += (std::string)str + "\n"; \
								logLines++;

#define GET_FLAG(bit)           ((SR & (1 << bit)) >> bit)
#define SET_FLAG_1(bit)           SR |= (1 << bit)
#define SET_FLAG_0(bit)         SR &= ~((uint8_t)(1 << bit))
#define TOGGLE_FLAG(bit)        SR ^= (1 << bit)
#define SET_FLAG(bit, value)    SET_FLAG_0(bit); SR |= (value << bit)

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

#define updateRegistersText() registers.setString("A:  #$" + HEX(emu.A) + "\n" + \
												  "X:  #$" + HEX(emu.X) + "\n" + \
												  "Y:  #$" + HEX(emu.Y) + "\n" + \
												  "SR: #$" + HEX(emu.SR) + "\n" + \
												  "S:  #$" + HEX(emu.S) + "\n" + \
												  "PC: #$" + HEX(emu.PC));

	/*124, 124, 124
	0, 0, 252
	0, 0, 188
	68, 40, 188
	148, 0, 132
	168, 0, 32
	168, 16, 0
	136, 20, 0
	80, 48, 0
	0, 120, 0
	0, 104, 0
	0, 88, 0
	0, 64, 88
	0, 0, 0
	0, 0, 0
	0, 0, 0
	188, 188, 188
	0, 120, 248
	0, 88, 248
	104, 68, 252
	216, 0, 204
	228, 0, 88
	248, 56, 0
	228, 92, 16
	172, 124, 0
	0, 184, 0
	0, 168, 0
	0, 168, 68
	0, 136, 136
	0, 0, 0
	0, 0, 0
	0, 0, 0
	248, 248, 248
	60, 188, 252
	104, 136, 252
	152, 120, 248
	248, 120, 248
	248, 88, 152
	248, 120, 88
	252, 160, 68
	248, 184, 0
	184, 248, 24
	88, 216, 84
	88, 248, 152
	0, 232, 216
	120, 120, 120
	0, 0, 0
	0, 0, 0
	252, 252, 252
	164, 228, 252
	184, 184, 248
	216, 184, 248
	248, 184, 248
	248, 164, 192
	240, 208, 176
	252, 224, 168
	248, 216, 120
	216, 248, 120
	184, 248, 184
	184, 248, 216
	0, 252, 252
	248, 216, 248
	0, 0, 0
	0, 0, 0*/
	uint8_t colorToRGB_palette[64 * 3]
	{
		124,124,124,
		0,0,252,
		0,0,188,
		68,40,188,
		148,0,132,
		168,0,32,
		168,16,0,
		136,20,0,
		80,48,0,
		0,120,0,
		0,104,0,
		0,88,0,
		0,64,88,
		0,0,0,
		0,0,0,
		0,0,0,
		188,188,188,
		0,120,248,
		0,88,248,
		104,68,252,
		216,0,204,
		228,0,88,
		248,56,0,
		228,92,16,
		172,124,0,
		0,184,0,
		0,168,0,
		0,168,68,
		0,136,136,
		0,0,0,
		0,0,0,
		0,0,0,
		248,248,248,
		60,188,252,
		104,136,252,
		152,120,248,
		248,120,248,
		248,88,152,
		248,120,88,
		252,160,68,
		248,184,0,
		184,248,24,
		88,216,84,
		88,248,152,
		0,232,216,
		120,120,120,
		0,0,0,
		0,0,0,
		252,252,252,
		164,228,252,
		184,184,248,
		216,184,248,
		248,184,248,
		248,164,192,
		240,208,176,
		252,224,168,
		248,216,120,
		216,248,120,
		184,248,184,
		184,248,216,
		0,252,252,
		248,216,248,
		0,0,0,
		0,0,0,
	};

	inline sf::Color NESColorToRGB(uint8_t color)
	{
		return sf::Color(colorToRGB_palette[color * 3], 
					 	 colorToRGB_palette[color * 3 + 1], 
						 colorToRGB_palette[color * 3 + 2]);
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
		Other
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
}