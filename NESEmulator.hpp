#pragma once

#include <stdint.h>
#include <iostream>

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
		writeMemory1B(0x4017, 0);
		writeMemory1B(0x4015, 0);
		for (uint16_t i = 0x4000; i < 0x400f; i++)
			writeMemory1B(i, 0);
		for (uint16_t i = 0x4010; i < 0x4013; i++)
			writeMemory1B(i, 0);
		cycles = 0;
	}

	void reset()
	{
		S -= 3;
		P |= 0x04;
		cycles = 0;
	}

	void load(uint16_t startAddress, uint8_t* opcodes, uint16_t size)
	{
		for (uint16_t i = 0; i < size; i++)
			writeMemory1B(startAddress + i, opcodes[i]);

		PC = startAddress;
	}

	void writeMemory1B(uint16_t address, uint8_t value)
	{
		if (address < 0x2000)
			memory[address % 0x800] = value;
	}

	uint8_t readMemory1B(uint16_t address)
	{
		if (address < 0x2000)
			return memory[address % 0x800];
		return 0;
	}

	void push(uint8_t value)
	{
		writeMemory1B(S + 0x100, value);
		S--;
	}

	uint8_t pull()
	{
		S++;
		return readMemory1B(S + 0x100);
	}

	uint8_t readIndexedIndirectX(uint8_t d)
	{
		return readMemory1B(readMemory1B((d + X) % 256) + (readMemory1B((d + X + 1) % 256) * 256));
	}

	uint8_t readIndirectIndexedY(uint8_t d, bool& pageBoundaryCrossed)
	{
		uint16_t lo = readMemory1B(d);
		pageBoundaryCrossed = lo > 0xff;
		return readMemory1B(lo + (readMemory1B((d + 1) % 256) * 256) + Y);
	}

	uint8_t zeropageIndexedXAddress(uint8_t d)
	{
		return (d + X) % 256;
	}

	uint8_t zeropageIndexedYAddress(uint8_t d)
	{
		return (d + Y) % 256;
	}

	uint16_t absoluteIndexedYAddress(uint16_t a, bool pageBoundaryCrossed)
	{
		pageBoundaryCrossed = ((a >> 8) == ((a + Y) >> 8));
		return a + Y;
	}

	std::string cycle()
	{
		std::string str = "";
		if (cycles == 0)
		{
			uint8_t opcode = readMemory1B(PC);
			uint8_t arg8 = readMemory1B(PC + 1);
			uint16_t arg16 = (((uint16_t)readMemory1B(PC + 2) << 8) | arg8);
			uint16_t tmp16;
			uint8_t tmp8;
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

				tmp8 = readMemory1B(arg8);

				A |= tmp8;

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 3;
				PC += 2;

				break;

			case 0x06: // ASL zeropage
				str += "ASL $" + HEX(arg8);

				tmp8 = readMemory1B(arg8);

				SET_FLAG(FLAG_C, tmp8 >> 7);

				tmp8 <<= 1;

				writeMemory1B(arg8, tmp8);

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

				A |= readMemory1B(arg16);

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 4;
				PC += 3;

				break;

			case 0x0e: // ASL absolute
				str += "ASL $" + HEX(arg16);

				tmp8 = readMemory1B(arg16);

				SET_FLAG(FLAG_C, tmp8 >> 7);

				tmp8 <<= 1;

				writeMemory1B(arg16, tmp8);

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

				A |= readMemory1B(zeropageIndexedXAddress(arg8));

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 4;
				PC += 2;

				break;

			case 0x16: // ASL zeropage, X
				str += "ASL $" + HEX(arg8) + ", X";

				tmp8 = readMemory1B(zeropageIndexedXAddress(arg8));

				SET_FLAG(FLAG_C, tmp8 >> 7);

				tmp8 <<= 1;

				writeMemory1B(zeropageIndexedXAddress(arg8), tmp8);

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

				A |= readMemory1B(absoluteIndexedYAddress(arg16, pageBoundaryCrossed));

				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));

				cycles = 4 + pageBoundaryCrossed;
				PC += 3;

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

				writeMemory1B(arg8, A);

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
	uint8_t memory[2 * KB];
	uint8_t A, X, Y, P, S;
	uint16_t PC;

	uint8_t cycles;
	uint8_t mapper;
};