#pragma once

#include <stdint.h>
#include <iostream>
#include <fstream>
#include <vector>

#include "util.hpp"

class NESEmulator
{
public:
	NESEmulator()
	{ 
		powerUp();
	}

	void powerUp()
	{
		A = X = Y = 0;
		P = 0x34;
		S = 0xfd;
		CPU_writeMemory1B(0x4017, 0);
		CPU_writeMemory1B(0x4015, 0);
		for (uint16_t i = 0x4000; i < 0x400f; i++)
			CPU_writeMemory1B(i, 0);
		for (uint16_t i = 0x4010; i < 0x4013; i++)
			CPU_writeMemory1B(i, 0);
		cycles = 0;
	}

	void reset()
	{
		S -= 3;
		P |= 0x04;
		cycles = 9;
		PC = ((uint16_t)CPU_readMemory1B(0xfffd) << 8) | CPU_readMemory1B(0xfffc);
	}

	void loadFromBuffer(uint16_t startAddress, uint8_t* opcodes, uint16_t size)
	{
		for (uint16_t i = 0; i < size; i++)
			CPU_writeMemory1B(startAddress + i, opcodes[i]);

		PC = startAddress;

		CPU_writeMemory1B(0xfffd, (uint8_t)(PC >> 8));
		CPU_writeMemory1B(0xfffc, (uint8_t)PC);
	}

	void loadFromiNES(std::string fileName)
	{
		std::ifstream f(fileName, std::ios::binary);
		if (!f) return;
		std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(f), {});
		f.close();

		iNESHeader header;
		memcpy(&header, &buffer[0], /*sizeof(header)*/16 * sizeof(uint8_t));

		uint8_t mapper_lo = (header.flags6 >> 4), mapper_hi = (header.flags7 >> 4);
		uint8_t mapperNb = (mapper_lo | (mapper_hi << 4));
		mapper = (mapperNb == 0 ? (header.PRGROMSize == 1 ? Mapper0_NROM_128 : Mapper0_NROM_256) : Other);

		//std::cout << HEX(mapper) << std::endl;

		bool trainer = (header.flags6 >> 2) & 1;

		uint16_t PRGROM_location = 16 + (512 * trainer), PRGROM_size = 16384 * header.PRGROMSize;

		memcpy(&CPU_memory[0x8000], &buffer[PRGROM_location], PRGROM_size * sizeof(uint8_t));

		PC = ((uint16_t)CPU_readMemory1B(0xfffd) << 8) | CPU_readMemory1B(0xfffc);
	}

	void CPU_writeMemory1B(uint16_t address, uint8_t value)
	{
		if (address < 0x2000)
			CPU_memory[address % 0x800] = value;

		else if (address < 0x4000) // PPU registers
			;

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
			return 0;

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

	void push(uint8_t value)
	{
		CPU_writeMemory1B(S + 0x100, value);
		S--;
	}

	uint8_t pull()
	{
		S++;
		return CPU_readMemory1B(S + 0x100);
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

	void cycle()
	{
		PPU_cycle();

		if(cycleCounter == 0)
			CPU_cycle();

		cycleCounter++;
		cycleCounter %= 3;
	}

	void PPU_cycle()
	{
		;
	}

	std::string CPU_cycle()
	{
		std::string str = "";
		if (cycles == 0)
		{
			uint8_t opcode = CPU_readMemory1B(PC);
			uint8_t arg8 = CPU_readMemory1B(PC + 1);
			uint16_t arg16 = (((uint16_t)CPU_readMemory1B(PC + 2) << 8) | arg8);
			uint16_t tmp16;
			uint8_t tmp8;
			bool tmp1;
			uint8_t PCPage = (PC >> 8);
			bool pageBoundaryCrossed = false;

			str += "$" + HEX(PC) + " : $" + HEX(opcode) + " : ";

			// opcodes to add : BRK
			switch (opcode)
			{
			case 0x01: // ORA (indirect, X)
				str += "ORA ($" + HEX(arg8) + ", X)";

				tmp8 = readIndexedIndirectX(arg8);

				A |= tmp8;

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 6;
				PC += 2;

				break;

			case 0x05: // ORA zeropage
				str += "ORA $" + HEX(arg8);

				tmp8 = CPU_readMemory1B(arg8);

				A |= tmp8;

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 3;
				PC += 2;

				break;

			case 0x06: // ASL zeropage
				str += "ASL $" + HEX(arg8);

				tmp8 = CPU_readMemory1B(arg8);

				SET_FLAG(FLAG_C, tmp8 >> 7);

				tmp8 <<= 1;

				CPU_writeMemory1B(arg8, tmp8);

				SET_FLAG(FLAG_N, (tmp8 >> 7));
				SET_FLAG(FLAG_Z, (tmp8 == 0));

				cycles = 5;
				PC += 2;

				break;

			case 0x08: // PHP
				str += "PHP";

				tmp8 = (P | (1 << FLAG_B) | (1 << FLAG_1));
				push(tmp8);

				cycles = 3;
				PC += 1;

				break;

			case 0x09: // ORA imm8
				str += "ORA #$" + HEX(arg8);

				A |= arg8;

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 2;
				PC += 2;

				break;

			case 0x0a: // ASL A
				str += "ASL A";

				SET_FLAG(FLAG_C, A >> 7);

				A <<= 1;

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 2;
				PC += 1;

				break;

			case 0x0d: // ORA absolute
				str += "ORA $" + HEX(arg16);

				A |= CPU_readMemory1B(arg16);

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 4;
				PC += 3;

				break;

			case 0x0e: // ASL absolute
				str += "ASL $" + HEX(arg16);

				tmp8 = CPU_readMemory1B(arg16);

				SET_FLAG(FLAG_C, tmp8 >> 7);

				tmp8 <<= 1;

				CPU_writeMemory1B(arg16, tmp8);

				SET_FLAG(FLAG_N, (tmp8 >> 7));
				SET_FLAG(FLAG_Z, (tmp8 == 0));

				cycles = 6;
				PC += 3;

				break;

			case 0x10: // BPL relative
				str += "BPL $" + HEX(arg8);

				if (!GET_FLAG(FLAG_N))
					PC += (int8_t)arg8;

				cycles = 3 + (PCPage != (PC >> 8)); //2**
				PC += 2;

				break;
				
			case 0x11: // ORA (indirect), Y
				str += "ORA ($" + HEX(arg8) + "), Y";

				A |= readIndirectIndexedY(arg8, pageBoundaryCrossed);

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 5 + pageBoundaryCrossed;
				PC += 3;

				break;

			case 0x15: // ORA zeropage, X
				str += "ORA $" + HEX(arg8) + ", X";

				A |= CPU_readMemory1B(zeropageIndexedXAddress(arg8));

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 4;
				PC += 2;

				break;

			case 0x16: // ASL zeropage, X
				str += "ASL $" + HEX(arg8) + ", X";

				tmp8 = CPU_readMemory1B(zeropageIndexedXAddress(arg8));

				SET_FLAG(FLAG_C, tmp8 >> 7);

				tmp8 <<= 1;

				CPU_writeMemory1B(zeropageIndexedXAddress(arg8), tmp8);

				SET_FLAG(FLAG_N, (tmp8 >> 7));
				SET_FLAG(FLAG_Z, (tmp8 == 0));

				cycles = 6;
				PC += 2;

				break;

			case 0x18: // CLC
				str += "CLC";

				SET_FLAG_0(FLAG_C);

				cycles = 2;
				PC += 1;

				break;

			case 0x19: // ORA absolute, Y
				str += "ORA $" + HEX(arg16) + ", Y";

				A |= CPU_readMemory1B(absoluteIndexedYAddress(arg16, pageBoundaryCrossed));

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 4 + pageBoundaryCrossed;
				PC += 3;

				break;

			case 0x1d: // ORA absolute, X
				str += "ORA $" + HEX(arg16) + ", X";

				A |= CPU_readMemory1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed));

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 4 + pageBoundaryCrossed;
				PC += 3;

				break;

			case 0x1e: // ASL absolute, X
				str += "ASL $" + HEX(arg16) + ", X";

				tmp8 = CPU_readMemory1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed));

				SET_FLAG(FLAG_C, tmp8 >> 7);

				tmp8 <<= 1;

				CPU_writeMemory1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed), tmp8);

				SET_FLAG(FLAG_N, (tmp8 >> 7));
				SET_FLAG(FLAG_Z, (tmp8 == 0));

				cycles = 7;
				PC += 3;

				break;

			case 0x20: // JSR absolute
				str += "JSR $" + HEX(arg16);

				push(PC + 2);

				PC = arg16;

				cycles = 6;

				break;

			case 0x21: // AND (indirect, X)
				str += "AND ($" + HEX(arg8) + ", X)";

				A &= readIndexedIndirectX(arg8);

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 6;
				PC += 2;

				break;

			case 0x24: // BIT zeropage
				str += "BIT $" + HEX(arg8);

				tmp8 = CPU_readMemory1B(arg8);

				SET_FLAG(6, (tmp8 >> 6));
				SET_FLAG(7, (tmp8 >> 7));

				SET_FLAG(FLAG_Z, (tmp8 & A) == 0);

				cycles = 3;
				PC += 2;

				break;

			case 0x25: // AND zeropage
				str += "AND $" + HEX(arg8);

				A &= CPU_readMemory1B(arg8);

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 3;
				PC += 2;

				break;

			case 0x26: // ROL zeropage
				str += "ROL $" + HEX(arg8);

				tmp8 = CPU_readMemory1B(arg8);

				tmp1 = GET_FLAG(FLAG_C);

				SET_FLAG(FLAG_C, tmp8 >> 7);
				tmp8 <<= 1;
				tmp8 |= (uint8_t)tmp1;

				CPU_writeMemory1B(arg8, tmp8);

				SET_FLAG(FLAG_N, (tmp8 >> 7));
				SET_FLAG(FLAG_Z, (tmp8 == 0));

				cycles = 5;
				PC += 2;

				break;

			case 0x28: // PLP
				str += "PLP";

				P = pull();

				cycles = 4;
				PC += 1;

				break;

			case 0x29: // AND imm8
				str += "AND #$" + HEX(arg8);

				A &= arg8;

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 2;
				PC += 2;

				break;

			case 0x2a: // ROL A
				str += "ROL A";

				tmp1 = GET_FLAG(FLAG_C);

				SET_FLAG(FLAG_C, (A >> 7));
				A <<= 1;
				A |= (uint8_t)tmp1;

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 2;
				PC += 1;

				break;

			case 0x2c: // BIT absolute
				str += "BIT $" + HEX(arg16);

				tmp8 = CPU_readMemory1B(arg16);

				SET_FLAG(6, (tmp8 >> 6));
				SET_FLAG(7, (tmp8 >> 7));

				SET_FLAG(FLAG_Z, (tmp8 & A) == 0);

				cycles = 4;
				PC += 3;

				break;

			case 0x2d: // AND absolute
				str += "AND $" + HEX(arg16);

				A &= CPU_readMemory1B(arg16);

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 4;
				PC += 3;

				break;

			case 0x2e: // ROL absolute
				str += "ROL $" + HEX(arg16);

				tmp8 = CPU_readMemory1B(arg16);

				tmp1 = GET_FLAG(FLAG_C);

				SET_FLAG(FLAG_C, tmp8 >> 7);
				tmp8 <<= 1;
				tmp8 |= (uint8_t)tmp1;

				CPU_writeMemory1B(arg16, tmp8);

				SET_FLAG(FLAG_N, (tmp8 >> 7));
				SET_FLAG(FLAG_Z, (tmp8 == 0));

				cycles = 6;
				PC += 3;

				break;

			case 0x30: // BMI relative
				str += "BMI $" + HEX(arg8);

				if (GET_FLAG(FLAG_N))
					PC += (int8_t)arg8;

				cycles = 3 + (PCPage != (PC >> 8)); //2**
				PC += 2;

				break;

			case 0x31: // AND (indirect), Y
				str += "AND ($" + HEX(arg8) + "), Y";

				A &= readIndirectIndexedY(arg8, pageBoundaryCrossed);

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 5 + pageBoundaryCrossed;
				PC += 2;

				break;

			case 0x35: // AND zeropage, Y
				str += "AND $" + HEX(arg8) + ", Y";

				A &= CPU_readMemory1B(zeropageIndexedYAddress(arg8));

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 4;
				PC += 2;

				break;

			case 0x36: // ROL zeropage, X
				str += "ROL $" + HEX(arg8) + ", X";

				tmp8 = CPU_readMemory1B(zeropageIndexedXAddress(arg8));

				tmp1 = GET_FLAG(FLAG_C);

				SET_FLAG(FLAG_C, tmp8 >> 7);
				tmp8 <<= 1;
				tmp8 |= (uint8_t)tmp1;

				CPU_writeMemory1B(zeropageIndexedXAddress(arg8), tmp8);

				SET_FLAG(FLAG_N, (tmp8 >> 7));
				SET_FLAG(FLAG_Z, (tmp8 == 0));

				cycles = 6;
				PC += 2;

				break;

			case 0x38: // SEC
				str += "SEC";

				SET_FLAG_1(FLAG_CARRY);

				cycles = 2;
				PC += 1;

				break;

			case 0xa9: // LDA imm8
				str += "LDA #$" + HEX(arg8);

				A = arg8;

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 2;
				PC += 2;

				break;

			case 0x69: // ADC imm8
				str += "ADC #$" + HEX(arg8);

				tmp16 = (uint16_t)A + (uint16_t)arg8 + (uint16_t)GET_FLAG(FLAG_C);
				A = (uint8_t)tmp16;

				SET_FLAG(FLAG_C, (tmp16 > 0xff));
				SET_FLAG(FLAG_V, ((~((uint16_t)A ^ (uint16_t)arg8) & ((uint16_t)A ^ (uint16_t)tmp16)) >> 7));
				SET_FLAG(FLAG_Z, (tmp16 == 0));
				SET_FLAG(FLAG_N, (A >> 7));

				cycles = 2;
				PC += 2;

				break;

			case 0x85: // STA zeropage
				str += "STA $" + HEX(arg8);

				CPU_writeMemory1B(arg8, A);

				cycles = 3;
				PC += 2;

				break;

			default:
				str += "Unhandled opcode";

				break;
			}

			str += "\n";
		}

		cycles--;

		return str;
	}

public:
	uint8_t CPU_memory[64 * KB];
	uint8_t PPU_memory[16 * KB];
	uint8_t A, X, Y, P, S;
	uint16_t PC;

	uint8_t cycles;
	uint8_t cycleCounter;
	Mapper mapper;
};