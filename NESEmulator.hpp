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

	std::string cycle()
	{
		std::string str = "";
		if (cycles == 0)
		{
			uint8_t opcode = readMemory1B(PC);
			uint8_t imm8 = readMemory1B(PC + 1);
			uint16_t tmp;

			str += "Ox" + HEX(PC) + " : 0x" + HEX(opcode) + " : ";

			switch (opcode)
			{
			case 0xa9: // LDA imm8
				str += "LDA #$" + HEX(imm8);
				A = imm8;
				SET_FLAG(FLAG_N, (A >> 7));
				SET_FLAG(FLAG_Z, (A == 0));
				cycles = 2;
				PC += 2;

				break;

			case 0x69: // ADC imm8
				str += "ADC #$" + HEX(imm8);
				tmp = (uint16_t)A + (uint16_t)imm8 + (uint16_t)GET_FLAG(FLAG_C);
				A = (uint8_t)tmp;
				SET_FLAG(FLAG_C, (tmp > 0xff));
				SET_FLAG(FLAG_V, ((~((uint16_t)A ^ (uint16_t)imm8) & ((uint16_t)A ^ (uint16_t)tmp)) >> 7));
				SET_FLAG(FLAG_Z, (tmp == 0));
				SET_FLAG(FLAG_N, (A >> 7));
				cycles = 2;
				PC += 2;

				break;

			case 0x85: // STA zeropage
				str += "STA $" + HEX(imm8);
				writeMemory1B(imm8, A);
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