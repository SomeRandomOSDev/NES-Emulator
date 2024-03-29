#include "CPU_RP2A03.hpp"

void CPU_RP2A03::cycle()
{
	HandleInput();

	if (cycles == 0)
	{
		uint8_t opcode = read_1B(PC);
		uint8_t arg8 = read_1B(PC + 1);
		uint16_t arg16 = read_2B(PC + 1);
		uint16_t tmp16;
		uint8_t tmp8, tmp8_2;
		bool tmp1;
		uint8_t PCPage = ((PC + 2) >> 8);
		bool pageBoundaryCrossed = false;

		//str += "$" + HEX_2B(PC) + " : $" + HEX_1B(opcode) + " : ";

		switch (opcode)
		{
		case 0x00: // BRK imm8
			//str += "BRK #$" + HEX(arg8);

			INTERRUPT(PC + 2, BRK_VECTOR, true);

			cycles = 7;

			break;

		case 0x01: // ORA (indirect, X)
			//str += "ORA ($" + HEX(arg8) + ", X)";

			tmp8 = read_1B(indexedIndirectXAddress(arg8));

			A |= tmp8;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 6;
			PC += 2;

			break;

		case 0x05: // ORA zeropage
			//str += "ORA $" + HEX(arg8);

			tmp8 = read_1B(arg8);

			A |= tmp8;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 3;
			PC += 2;

			break;

		case 0x06: // ASL zeropage
			//str += "ASL $" + HEX(arg8);

			tmp8 = read_1B(arg8);

			SET_FLAG(FLAG_C, tmp8 >> 7);

			tmp8 <<= 1;

			write_1B(arg8, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 5;
			PC += 2;

			break;

		case 0x08: // PHP
			//str += "PHP";

			push_1B(SR | (1 << FLAG_B) | (1 << FLAG_1));

			cycles = 3;
			PC += 1;

			break;

		case 0x09: // ORA imm8
			//str += "ORA #$" + HEX(arg8);

			A |= arg8;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 2;
			PC += 2;

			break;

		case 0x0a: // ASL A
			//str += "ASL A";

			SET_FLAG(FLAG_C, A >> 7);

			A <<= 1;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 2;
			PC += 1;

			break;

		case 0x0d: // ORA absolute
			//str += "ORA $" + HEX(arg16);

			A |= read_1B(arg16);

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4;
			PC += 3;

			break;

		case 0x0e: // ASL absolute
			//str += "ASL $" + HEX(arg16);

			tmp8 = read_1B(arg16);

			SET_FLAG(FLAG_C, tmp8 >> 7);

			tmp8 <<= 1;

			write_1B(arg16, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 6;
			PC += 3;

			break;

		case 0x10: // BPL relative
			//str += "BPL $" + HEX(arg8);

			cycles = 2;

			if (!GET_FLAG(FLAG_N))
			{
				cycles++;
				PC += (int8_t)arg8;
			}

			PC += 2;

			if (PCPage != (PC >> 8))
				cycles++;

			break;

		case 0x11: // ORA (indirect), Y
			//str += "ORA ($" + HEX(arg8) + "), Y";

			A |= readIndirectIndexedY(arg8, pageBoundaryCrossed);

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 5 + pageBoundaryCrossed;
			PC += 2;

			break;

		case 0x15: // ORA zeropage, X
			//str += "ORA $" + HEX(arg8) + ", X";

			A |= read_1B(zeropageIndexedXAddress(arg8));

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4;
			PC += 2;

			break;

		case 0x16: // ASL zeropage, X
			//str += "ASL $" + HEX(arg8) + ", X";

			tmp16 = zeropageIndexedXAddress(arg8);
			tmp8 = read_1B(tmp16);

			SET_FLAG(FLAG_C, tmp8 >> 7);

			tmp8 <<= 1;

			write_1B(tmp16, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 6;
			PC += 2;

			break;

		case 0x18: // CLC
			//str += "CLC";

			SET_FLAG_0(FLAG_C);

			cycles = 2;
			PC += 1;

			break;

		case 0x19: // ORA absolute, Y
			//str += "ORA $" + HEX(arg16) + ", Y";

			A |= read_1B(absoluteIndexedYAddress(arg16, pageBoundaryCrossed));

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0x1d: // ORA absolute, X
			//str += "ORA $" + HEX(arg16) + ", X";

			A |= read_1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed));

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0x1e: // ASL absolute, X
			//str += "ASL $" + HEX(arg16) + ", X";

			tmp16 = absoluteIndexedXAddress(arg16, pageBoundaryCrossed);

			tmp8 = read_1B(tmp16);

			SET_FLAG(FLAG_C, tmp8 >> 7);

			tmp8 <<= 1;

			write_1B(tmp16, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 7;
			PC += 3;

			break;

		case 0x20: // JSR absolute
			//str += "JSR $" + HEX(arg16);

			push_2B(PC + 2);

			PC = arg16;

			cycles = 6;

			break;

		case 0x21: // AND (indirect, X)
			//str += "AND ($" + HEX(arg8) + ", X)";

			A &= read_1B(indexedIndirectXAddress(arg8));

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 6;
			PC += 2;

			break;

		case 0x24: // BIT zeropage
			//str += "BIT $" + HEX(arg8);

			tmp8 = read_1B(arg8);

			SET_FLAG(6, (tmp8 >> 6) & 1);
			SET_FLAG(7, (tmp8 >> 7) & 1);

			SET_FLAG(FLAG_Z, (tmp8 & A) == 0);

			cycles = 3;
			PC += 2;

			break;

		case 0x25: // AND zeropage
			//str += "AND $" + HEX(arg8);

			A &= read_1B(arg8);

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 3;
			PC += 2;

			break;

		case 0x26: // ROL zeropage
			//str += "ROL $" + HEX(arg8);

			tmp8 = read_1B(arg8);

			tmp1 = GET_FLAG(FLAG_C);

			SET_FLAG(FLAG_C, tmp8 >> 7);
			tmp8 <<= 1;
			tmp8 |= (uint8_t)tmp1;

			write_1B(arg8, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 5;
			PC += 2;

			break;

		case 0x28: // PLP
			//str += "PLP";

			SR = pull_1B();
			SET_FLAG_0(FLAG_B);
			SET_FLAG_1(FLAG_1);

			cycles = 4;
			PC += 1;

			break;

		case 0x29: // AND imm8
			//str += "AND #$" + HEX(arg8);

			A &= arg8;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 2;
			PC += 2;

			break;

		case 0x2a: // ROL A
			//str += "ROL A";

			tmp1 = GET_FLAG(FLAG_C);

			SET_FLAG(FLAG_C, (A >> 7));
			A <<= 1;
			A |= (uint8_t)tmp1;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 2;
			PC++;

			break;

		case 0x2c: // BIT absolute
			//str += "BIT $" + HEX(arg16);

			tmp8 = read_1B(arg16);

			SET_FLAG(6, (tmp8 >> 6) & 1);
			SET_FLAG(7, (tmp8 >> 7) & 1);

			SET_FLAG(FLAG_Z, (tmp8 & A) == 0);

			cycles = 4;
			PC += 3;

			break;

		case 0x2d: // AND absolute
			//str += "AND $" + HEX(arg16);

			A &= read_1B(arg16);

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4;
			PC += 3;

			break;

		case 0x2e: // ROL absolute
			//str += "ROL $" + HEX(arg16);

			tmp8 = read_1B(arg16);

			tmp1 = GET_FLAG(FLAG_C);

			SET_FLAG(FLAG_C, tmp8 >> 7);
			tmp8 <<= 1;
			tmp8 |= (uint8_t)tmp1;

			write_1B(arg16, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 6;
			PC += 3;

			break;

		case 0x30: // BMI relative
			//str += "BMI $" + HEX(arg8);

			cycles = 2;

			if (GET_FLAG(FLAG_N))
			{
				cycles++;
				PC += (int8_t)arg8;
			}
			PC += 2;
			if (PCPage != (PC >> 8))
				cycles++;

			break;

		case 0x31: // AND (indirect), Y
			//str += "AND ($" + HEX(arg8) + "), Y";

			A &= readIndirectIndexedY(arg8, pageBoundaryCrossed);

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 5 + pageBoundaryCrossed;
			PC += 2;

			break;

		case 0x35: // AND zeropage, Y
			//str += "AND $" + HEX(arg8) + ", Y";

			A &= read_1B(zeropageIndexedYAddress(arg8));

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4;
			PC += 2;

			break;

		case 0x36: // ROL zeropage, X
			//str += "ROL $" + HEX(arg8) + ", X";

			tmp16 = zeropageIndexedXAddress(arg8);
			tmp8 = read_1B(tmp16);

			tmp1 = GET_FLAG(FLAG_C);

			SET_FLAG(FLAG_C, tmp8 >> 7);
			tmp8 <<= 1;
			tmp8 |= (uint8_t)tmp1;

			write_1B(tmp16, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 6;
			PC += 2;

			break;

		case 0x38: // SEC
			//str += "SEC";

			SET_FLAG_1(FLAG_CARRY);

			cycles = 2;
			PC += 1;

			break;

		case 0x39: // AND absolute, Y
			//str += "AND $" + HEX(arg16) + ", Y";

			A &= read_1B(absoluteIndexedYAddress(arg16, pageBoundaryCrossed));

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0x3d: // AND absolute, X
			//str += "AND $" + HEX(arg16) + ", X";

			A &= read_1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed));

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0x3e: // ROL absolute, X
			//str += "ROL $" + HEX(arg16) + ", X";

			tmp16 = absoluteIndexedXAddress(arg16, pageBoundaryCrossed);
			tmp8 = read_1B(tmp16);

			tmp1 = GET_FLAG(FLAG_C);

			SET_FLAG(FLAG_C, tmp8 >> 7);
			tmp8 <<= 1;
			tmp8 |= (uint8_t)tmp1;

			write_1B(tmp16, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 7;
			PC += 3;

			break;

		case 0x40: // RTI
			//str += "RTI";

			SR = pull_1B();
			//SET_FLAG_0(FLAG_I);
			//SET_FLAG_0(FLAG_1);
			PC = pull_2B();

			cycles = 6;
			//PC++;

			break;

		case 0x41: // EOR (indirect, X)
			//str += "EOR ($" + HEX(arg8) + ", X)";

			tmp8 = read_1B(indexedIndirectXAddress(arg8));
			A ^= tmp8;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 6;
			PC += 2;

			break;

		case 0x45: // EOR zeropage
			//str += "EOR $" + HEX(arg8);

			tmp8 = read_1B(arg8);
			A ^= tmp8;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 3;
			PC += 2;

			break;

		case 0x46: // LSR zeropage
			//str += "LSR $" + HEX(arg8);

			tmp8 = read_1B(arg8);

			SET_FLAG(FLAG_C, tmp8 & 1);

			tmp8 >>= 1;

			write_1B(arg8, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 5;
			PC += 2;

			break;

		case 0x48: // PHA
			//str += "PHA";

			push_1B(A);

			PC++;
			cycles = 3;

			break;

		case 0x49: // EOR imm8
			//str += "EOR #$" + HEX(arg8);

			A ^= arg8;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 2;
			PC += 2;

			break;

		case 0x4a: // LSR A
			//str += "LSR A";

			SET_FLAG(FLAG_C, A & 1);

			A >>= 1;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 2;
			PC += 1;

			break;

		case 0x4c: // JMP absolute
			//str += "JMP $" + HEX(arg16);

			cycles = 3;
			//PC += 3;

			PC = arg16;

			break;

		case 0x4d: // EOR absolute
			//str += "EOR $" + HEX(arg16);

			A ^= read_1B(arg16);

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4;
			PC += 3;

			break;

		case 0x4e: // LSR absolute
			//str += "LSR $" + HEX(arg16);

			tmp8 = read_1B(arg16);

			SET_FLAG(FLAG_C, tmp8 & 1);

			tmp8 >>= 1;

			write_1B(arg16, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 6;
			PC += 3;

			break;

		case 0x50: // BVC relative
			//str += "BVC $" + HEX(arg8);

			cycles = 2;

			if (!GET_FLAG(FLAG_V))
			{
				PC += (int8_t)arg8;
				cycles++;
			}

			PC += 2;

			if (PCPage != (PC >> 8))
				cycles++;

			break;

		case 0x51: // EOR (indirect), Y
			//str += "EOR ($" + HEX(arg8) + "), Y";

			tmp8 = readIndirectIndexedY(arg8, pageBoundaryCrossed);
			A ^= tmp8;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 5 + pageBoundaryCrossed;
			PC += 2;

			break;

		case 0x55: // EOR zeropage, X
			//str += "EOR $" + HEX(arg8) + ", X";

			tmp8 = read_1B(zeropageIndexedXAddress(arg8));
			A ^= tmp8;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4;
			PC += 2;

			break;

		case 0x56: // LSR zeropage, X
			//str += "LSR $" + HEX(arg8) + ", X";

			tmp8 = read_1B(zeropageIndexedXAddress(arg8));

			SET_FLAG(FLAG_C, tmp8 & 1);

			tmp8 >>= 1;

			write_1B(zeropageIndexedXAddress(arg8), tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 6;
			PC += 2;

			break;

		case 0x58: // CLI
			SET_FLAG_0(FLAG_I);

			cycles = 2;
			PC++;

			break;

		case 0x59: // EOR absolute, Y
			//str += "EOR $" + HEX(arg16) + ", Y";

			tmp8 = read_1B(absoluteIndexedYAddress(arg16, pageBoundaryCrossed));
			A ^= tmp8;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0x5d: // EOR absolute, X
			//str += "EOR $" + HEX(arg16) + ", X";

			tmp8 = read_1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed));
			A ^= tmp8;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0x5e: // LSR absolute, X
			//str += "LSR $" + HEX(arg16) + ", X";

			tmp16 = absoluteIndexedXAddress(arg16, pageBoundaryCrossed);
			tmp8 = read_1B(tmp16);

			SET_FLAG(FLAG_C, tmp8 & 1);

			tmp8 >>= 1;

			write_1B(tmp16, tmp8);

			SET_FLAG(FLAG_N, 0);
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 7;
			PC += 3;

			break;

		case 0x60: // RTS
			//str += "RTS";

			PC = pull_2B();

			cycles = 6;
			PC++;

			break;

		case 0x61: // ADC (indirect, X)
			//str += "ADC ($" + HEX(arg8) + ", X)";

			tmp8 = read_1B(indexedIndirectXAddress(arg8));
			tmp16 = (uint16_t)A + (uint16_t)tmp8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ tmp8) & 0x80));
			SET_FLAG(FLAG_Z, (tmp16 == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 6;
			PC += 2;

			break;

		case 0x65: // ADC zeropage
			//str += "ADC $" + HEX(arg8);

			tmp8 = read_1B(arg8);
			tmp16 = (uint16_t)A + (uint16_t)tmp8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ tmp8) & 0x80));
			SET_FLAG(FLAG_Z, (tmp16 == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 3;
			PC += 2;

			break;

		case 0x66: // ROR zeropage
			//str += "ROR $" + HEX(arg8);

			tmp8 = read_1B(arg8);

			tmp1 = GET_FLAG(FLAG_C);

			SET_FLAG(FLAG_C, tmp8 & 1);
			tmp8 >>= 1;
			tmp8 |= ((uint8_t)tmp1 << 7);

			write_1B(arg8, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 5;
			PC += 2;

			break;

		case 0x68: // PLA
			//str += "PLA";

			A = pull_1B();

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4;
			PC++;

			break;

		case 0x69: // ADC imm8
			//str += "ADC #$" + HEX(arg8);

			tmp16 = (uint16_t)A + (uint16_t)arg8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ arg8) & 0x80));
			SET_FLAG(FLAG_Z, (tmp16 == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 2;
			PC += 2;

			break;

		case 0x6a: // ROR A
			//str += "ROR A";

			tmp8 = A;

			tmp1 = GET_FLAG(FLAG_C);

			SET_FLAG(FLAG_C, tmp8 & 1);
			tmp8 >>= 1;
			tmp8 |= ((uint8_t)tmp1 << 7);

			A = tmp8;

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 2;
			PC += 1;

			break;

		case 0x6c: // JMP (indirect)
			//str += "JMP ($" + HEX(arg16) + ")";

			cycles = 5;
			//PC += 3;

			PC = read_2B(arg16);

			//{
			//	uint8_t pc_lo = read_1B(PC + 1);
			//	uint8_t pc_hi = read_1B(PC + 2);

			//	PC = ((uint16_t)pc_hi << 8) | pc_lo;
			//}

			break;

		case 0x6d: // ADC absolute
			//str += "ADC $" + HEX(arg16);

			tmp8 = read_1B(arg16);
			tmp16 = (uint16_t)A + (uint16_t)tmp8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ tmp8) & 0x80));
			SET_FLAG(FLAG_Z, (tmp16 == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 4;
			PC += 3;

			break;

		case 0x6e: // ROR absolute
			//str += "ROR $" + HEX(arg16);

			tmp8 = read_1B(arg16);

			tmp1 = GET_FLAG(FLAG_C);

			SET_FLAG(FLAG_C, tmp8 & 1);
			tmp8 >>= 1;
			tmp8 |= ((uint8_t)tmp1 << 7);

			write_1B(arg16, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 6;
			PC += 3;

			break;

		case 0x70: // BVS relative
			//str += "BPL $" + HEX(arg8);

			cycles = 2;

			if (GET_FLAG(FLAG_V))
			{
				cycles++;
				PC += (int8_t)arg8;
			}

			PC += 2;

			if (PCPage != (PC >> 8))
				cycles++;

			break;

		case 0x71: // ADC (indirect), Y
			//str += "ADC ($" + HEX(arg8) + "), Y";

			tmp8 = readIndirectIndexedY(arg8, pageBoundaryCrossed);
			tmp16 = (uint16_t)A + (uint16_t)tmp8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ tmp8) & 0x80));
			SET_FLAG(FLAG_Z, (tmp16 == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 5 + pageBoundaryCrossed;
			PC += 2;

			break;

		case 0x75: // ADC zeropage, X
			//str += "ADC ($" + HEX(arg8) + "), X";

			tmp8 = read_1B(zeropageIndexedXAddress(arg8));
			tmp16 = (uint16_t)A + (uint16_t)tmp8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ tmp8) & 0x80));
			SET_FLAG(FLAG_Z, (tmp16 == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 4;
			PC += 2;

			break;

		case 0x76: // ROR zeropage, X
			//str += "ROR $" + HEX(arg8) + ", X";

			tmp16 = zeropageIndexedXAddress(arg8);

			tmp8 = read_1B(tmp16);

			tmp1 = GET_FLAG(FLAG_C);

			SET_FLAG(FLAG_C, tmp8 & 1);
			tmp8 >>= 1;
			tmp8 |= ((uint8_t)tmp1 << 7);

			write_1B(tmp16, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 6;
			PC += 2;

			break;

		case 0x78: // SEI
			//str += "SEI";

			SET_FLAG_1(FLAG_I);

			cycles = 2;
			PC++;

			break;

		case 0x79: // ADC absolute, Y
			//str += "ADC ($" + HEX(arg16) + "), Y";

			tmp8 = read_1B(absoluteIndexedYAddress(arg16, pageBoundaryCrossed));
			tmp16 = (uint16_t)A + (uint16_t)tmp8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ tmp8) & 0x80));
			SET_FLAG(FLAG_Z, (tmp16 == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0x7d: // ADC absolute, X
			//str += "ADC ($" + HEX(arg16) + "), X";

			tmp8 = read_1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed));
			tmp16 = (uint16_t)A + (uint16_t)tmp8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ tmp8) & 0x80));
			SET_FLAG(FLAG_Z, (tmp16 == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0x7e: // ROR absolute, X
			//str += "ROR $" + HEX(arg16) + ", X";

			tmp16 = absoluteIndexedXAddress(arg16, pageBoundaryCrossed);

			tmp8 = read_1B(tmp16);

			tmp1 = GET_FLAG(FLAG_C);

			SET_FLAG(FLAG_C, tmp8 & 1);
			tmp8 >>= 1;
			tmp8 |= ((uint8_t)tmp1 << 7);

			write_1B(tmp16, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 7;
			PC += 3;

			break;

		case 0x81: // STA (indirect, X)
			//str += "STA ($" + HEX(arg8) + ", X)";

			write_1B(indexedIndirectXAddress(arg8), A);

			cycles = 6;
			PC += 2;

			break;

		case 0x84: // STY zeropage
			//str += "STY $" + HEX(arg8);

			write_1B(arg8, Y);

			cycles = 3;
			PC += 2;

			break;

		case 0x85: // STA zeropage
			//str += "STA $" + HEX(arg8);

			write_1B(arg8, A);

			cycles = 3;
			PC += 2;

			break;

		case 0x86: // STX zeropage
			//str += "STX $" + HEX(arg8);

			write_1B(arg8, X);

			cycles = 3;
			PC += 2;

			break;

		case 0x88: // DEY
			//str += "DEY";

			Y--;

			cycles = 2;
			PC++;

			SET_FLAG(FLAG_N, (Y >> 7));
			SET_FLAG(FLAG_Z, (Y == 0));

			break;

		case 0x8a: // TXA
			//str += "TXA";

			A = X;

			cycles = 2;
			PC++;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			break;

		case 0x8c: // STY absolute
			//str += "STY $" + HEX(arg16);

			write_1B(arg16, Y);

			cycles = 4;
			PC += 3;

			break;

		case 0x8d: // STA absolute
			//str += "STA $" + HEX(arg16);

			write_1B(arg16, A);

			cycles = 4;
			PC += 3;

			break;

		case 0x8e: // STX absolute
			//str += "STX $" + HEX(arg16);

			write_1B(arg16, X);

			cycles = 4;
			PC += 3;

			break;

		case 0x90: // BCC relative
			//str += "BPL $" + HEX(arg8);

			cycles = 2;

			if (!GET_FLAG(FLAG_C))
			{
				cycles++;
				PC += (int8_t)arg8;
			}

			PC += 2;

			if (PCPage != (PC >> 8))
				cycles++;

			break;

		case 0x91: // STA (indirect), Y
			//str += "STA ($" + HEX(arg8) + "), Y";

			write_1B(indirectIndexedYAddress(arg8), A);

			cycles = 6;
			PC += 2;

			break;

		case 0x94: // STY zeropage, X
			//str += "STY $" + HEX(arg8) + ", X";

			write_1B(zeropageIndexedXAddress(arg8), Y);

			cycles = 4;
			PC += 2;

			break;

		case 0x95: // STA zeropage, X
			//str += "STA $" + HEX(arg8) + ", X";

			write_1B(zeropageIndexedXAddress(arg8), A);

			cycles = 4;
			PC += 2;

			break;

		case 0x96: // STX zeropage, Y
			//str += "STX $" + HEX(arg8) + ", Y";

			write_1B(zeropageIndexedYAddress(arg8), X);

			cycles = 4;
			PC += 2;

			break;

		case 0x98: // TYA
			//str += "TYA";

			A = Y;

			cycles = 2;
			PC++;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			break;

		case 0x99: // STA absolute, Y
			//str += "STA $" + HEX(arg16) + ", Y";

			write_1B(absoluteIndexedYAddress(arg16, pageBoundaryCrossed), A);

			cycles = 5;
			PC += 3;

			break;

		case 0x9a: // TXS
			//str += "TXS";

			S = X;

			cycles = 2;
			PC++;

			SET_FLAG(FLAG_N, (S >> 7));
			SET_FLAG(FLAG_Z, (S == 0));

			break;

		case 0x9d: // STA absolute, X
			//str += "STA $" + HEX(arg16) + ", X";

			write_1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed), A);

			cycles = 5;
			PC += 3;

			break;

		case 0xa0: // LDY imm8
			//str += "LDY #$" + HEX(arg8);

			Y = arg8;

			SET_FLAG(FLAG_N, (Y >> 7));
			SET_FLAG(FLAG_Z, (Y == 0));

			cycles = 2;
			PC += 2;

			break;

		case 0xa1: // LDA (indirect, X)
			//str += "LDA ($" + HEX(arg8) + ", X)";

			A = read_1B(indexedIndirectXAddress(arg8));

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 6;
			PC += 2;

			break;

		case 0xa2: // LDX imm8
			//str += "LDX #$" + HEX(arg8);

			X = arg8;

			SET_FLAG(FLAG_N, (X >> 7));
			SET_FLAG(FLAG_Z, (X == 0));

			cycles = 2;
			PC += 2;

			break;

		case 0xa4: // LDY zeropage
			//str += "LDY $" + HEX(arg8);

			Y = read_1B(arg8);

			SET_FLAG(FLAG_N, (Y >> 7));
			SET_FLAG(FLAG_Z, (Y == 0));

			cycles = 3;
			PC += 2;

			break;

		case 0xa5: // LDA zeropage
			//str += "LDA $" + HEX(arg8);

			A = read_1B(arg8);

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 3;
			PC += 2;

			break;

		case 0xa6: // LDX zeropage
			//str += "LDX $" + HEX(arg8);

			X = read_1B(arg8);

			SET_FLAG(FLAG_N, (X >> 7));
			SET_FLAG(FLAG_Z, (X == 0));

			cycles = 3;
			PC += 2;

			break;

		case 0xa8: // TAY
			//str += "TAY";

			Y = A;

			cycles = 2;
			PC++;

			SET_FLAG(FLAG_N, (Y >> 7));
			SET_FLAG(FLAG_Z, (Y == 0));

			break;

		case 0xa9: // LDA imm8
			//str += "LDA #$" + HEX(arg8);

			A = arg8;

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 2;
			PC += 2;

			break;

		case 0xaa: // TAX
			//str += "TAX";

			X = A;

			cycles = 2;
			PC++;

			SET_FLAG(FLAG_N, (X >> 7));
			SET_FLAG(FLAG_Z, (X == 0));

			break;

		case 0xac: // LDY absolute
			//str += "LDY $" + HEX(arg16);

			Y = read_1B(arg16);

			SET_FLAG(FLAG_N, (Y >> 7));
			SET_FLAG(FLAG_Z, (Y == 0));

			cycles = 4;
			PC += 3;

			break;

		case 0xad: // LDA absolute
			//str += "LDA $" + HEX(arg16);

			A = read_1B(arg16);

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4;
			PC += 3;

			break;

		case 0xae: // LDX absolute
			//str += "LDX $" + HEX(arg16);

			X = read_1B(arg16);

			SET_FLAG(FLAG_N, (X >> 7));
			SET_FLAG(FLAG_Z, (X == 0));

			cycles = 4;
			PC += 3;

			break;

		case 0xb0: // BCS relative
			//str += "BCS $" + HEX(arg8);

			cycles = 2;

			if (GET_FLAG(FLAG_C))
			{
				cycles++;
				PC += (int8_t)arg8;
			}

			PC += 2;

			if (PCPage != (PC >> 8))
				cycles++;

			break;

		case 0xb1: // LDA (indirect), Y
			//str += "LDA ($" + HEX(arg8) + "), Y";

			A = readIndirectIndexedY(arg8, pageBoundaryCrossed);

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 5 + pageBoundaryCrossed;
			PC += 2;

			break;

		case 0xb4: // LDY zeropage, X
			//str += "LDY $" + HEX(arg8) + ", X";

			Y = read_1B(zeropageIndexedXAddress(arg8));

			SET_FLAG(FLAG_N, (Y >> 7));
			SET_FLAG(FLAG_Z, (Y == 0));

			cycles = 4;
			PC += 2;

			break;

		case 0xb5: // LDA zeropage, X
			//str += "LDA $" + HEX(arg8) + ", X";

			A = read_1B(zeropageIndexedXAddress(arg8));

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4;
			PC += 2;

			break;

		case 0xb6: // LDX zeropage, Y
			//str += "LDX $" + HEX(arg8) + ", Y";

			X = read_1B(zeropageIndexedYAddress(arg8));

			SET_FLAG(FLAG_N, (X >> 7));
			SET_FLAG(FLAG_Z, (X == 0));

			cycles = 4;
			PC += 2;

			break;

		case 0xb8: // CLV
			//str += "CLV";

			SET_FLAG_0(FLAG_V);

			cycles = 2;
			PC++;

			break;

		case 0xb9: // LDA absolute, Y
			//str += "LDA $" + HEX(arg16) + ", Y";

			A = read_1B(absoluteIndexedYAddress(arg16, pageBoundaryCrossed));

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0xba: // TSX
			//str += "TSX";

			X = S;

			cycles = 2;
			PC++;

			//SET_FLAG(FLAG_N, (S >> 7));
			//SET_FLAG(FLAG_Z, (S == 0));

			break;

		case 0xbc: // LDY absolute, X
			//str += "LDY $" + HEX(arg16) + ", X";

			Y = read_1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed));

			SET_FLAG(FLAG_N, (Y >> 7));
			SET_FLAG(FLAG_Z, (Y == 0));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0xbd: // LDA absolute, X
			//str += "LDA $" + HEX(arg16) + ", X";

			A = read_1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed));

			SET_FLAG(FLAG_N, (A >> 7));
			SET_FLAG(FLAG_Z, (A == 0));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0xbe: // LDX absolute, Y
			//str += "LDX $" + HEX(arg16) + ", Y";

			X = read_1B(absoluteIndexedYAddress(arg16, pageBoundaryCrossed));

			SET_FLAG(FLAG_N, (X >> 7));
			SET_FLAG(FLAG_Z, (X == 0));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0xc0: // CPY imm8
			//str += "CPY #$" + HEX(arg8);

			tmp8 = Y - arg8;

			SET_FLAG(FLAG_C, Y >= arg8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 2;
			PC += 2;

			break;

		case 0xc1: // CMP (indirect, X)
			//str += "CMP ($" + HEX(arg8) + ", X)";

			tmp8_2 = read_1B(indexedIndirectXAddress(arg8));

			tmp8 = A - tmp8_2;

			SET_FLAG(FLAG_C, A >= tmp8_2);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 6;
			PC += 2;

			break;

		case 0xc4: // CPY zeropage
			//str += "CPY $" + HEX(arg8);

			tmp8_2 = read_1B(arg8);

			tmp8 = Y - tmp8_2;

			SET_FLAG(FLAG_C, Y >= tmp8_2);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 3;
			PC += 2;

			break;

		case 0xc5: // CMP zeropage
			//str += "CMP $" + HEX(arg8);

			tmp8_2 = read_1B(arg8);

			tmp8 = A - tmp8_2;

			SET_FLAG(FLAG_C, A >= tmp8_2);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 3;
			PC += 2;

			break;

		case 0xc6: // DEC zeropage
			//str += "DEC $" + HEX(arg8);

			tmp8 = read_1B(arg8) - 1;
			write_1B(arg8, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 5;
			PC += 2;

			break;

		case 0xc8: // INY
			//str += "INY";

			Y++;

			cycles = 2;
			PC++;

			SET_FLAG(FLAG_N, (Y >> 7));
			SET_FLAG(FLAG_Z, (Y == 0));

			break;

		case 0xc9: // CMP imm8
			//str += "CMP #$" + HEX(arg8);

			tmp8 = A - arg8;

			SET_FLAG(FLAG_C, A >= arg8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 2;
			PC += 2;

			break;

		case 0xca: // DEX
			//str += "DEX";

			X--;

			cycles = 2;
			PC++;

			SET_FLAG(FLAG_N, (X >> 7));
			SET_FLAG(FLAG_Z, (X == 0));

			break;

		case 0xcc: // CPY absolute
			//str += "CPY $" + HEX(arg16);

			tmp8_2 = read_1B(arg16);

			tmp8 = Y - tmp8_2;

			SET_FLAG(FLAG_C, Y >= tmp8_2);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 4;
			PC += 3;

			break;

		case 0xcd: // CMP absolute
			//str += "CMP $" + HEX(arg16);

			tmp8_2 = read_1B(arg16);

			tmp8 = A - tmp8_2;

			SET_FLAG(FLAG_C, A >= tmp8_2);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 4;
			PC += 3;

			break;

		case 0xce: // DEC absolute
			//str += "DEC $" + HEX(arg16);

			tmp8 = read_1B(arg16) - 1;
			write_1B(arg16, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 6;
			PC += 3;

			break;

		case 0xd0: // BNE relative
			//str += "BNE $" + HEX(arg8);

			cycles = 2;

			if (!GET_FLAG(FLAG_Z))
			{
				cycles++;
				PC += (int8_t)arg8;
			}

			PC += 2;

			if (PCPage != (PC >> 8))
				cycles++;

			break;

		case 0xd1: // CMP (indirect), Y
			//str += "CMP ($" + HEX(arg8) + "), Y";

			tmp8_2 = readIndirectIndexedY(arg8, pageBoundaryCrossed);

			tmp8 = A - tmp8_2;

			SET_FLAG(FLAG_C, A >= tmp8_2);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 5 + pageBoundaryCrossed;
			PC += 2;

			break;

		case 0xd5: // CMP zeropage, X
			//str += "CMP $" + HEX(arg8) + ", X";

			tmp8_2 = read_1B(zeropageIndexedXAddress(arg8));

			tmp8 = A - tmp8_2;

			SET_FLAG(FLAG_C, A >= tmp8_2);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 4;
			PC += 2;

			break;

		case 0xd6: // DEC zeropage, X
			//str += "DEC $" + HEX(arg8) + ", X";

			tmp16 = zeropageIndexedXAddress(arg8);
			tmp8 = read_1B(tmp16) - 1;
			write_1B(tmp16, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 6;
			PC += 2;

			break;

		case 0xd8: // CLD
			//str += "CLD";

			SET_FLAG_0(FLAG_D);

			cycles = 2;
			PC++;

			break;

		case 0xd9: // CMP absolute, Y
			//str += "CMP $" + HEX(arg16) + ", Y";

			tmp8_2 = read_1B(absoluteIndexedYAddress(arg16, pageBoundaryCrossed));

			tmp8 = A - tmp8_2;

			SET_FLAG(FLAG_C, A >= tmp8_2);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0xdd: // CMP absolute, X
			//str += "CMP $" + HEX(arg16) + ", X";

			tmp8_2 = read_1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed));

			tmp8 = A - tmp8_2;

			SET_FLAG(FLAG_C, A >= tmp8_2);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0xde: // DEC absolute, X
			//str += "DEC $" + HEX(arg16) + ", X";

			tmp8 = read_1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed)) - 1;
			write_1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed), tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 7;
			PC += 3;

			break;

		case 0xe0: // CPX imm8
			//str += "CPX #$" + HEX(arg8);

			tmp8 = X - arg8;

			SET_FLAG(FLAG_C, X >= arg8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 2;
			PC += 2;

			break;

		case 0xe1: // SBC (indirect, X)
			//str += "SBC ($" + HEX(arg8) + ", X)";

			tmp8 = ~read_1B(indexedIndirectXAddress(arg8));
			tmp16 = (uint16_t)A + (uint16_t)tmp8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			//SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ tmp8) & 0x80));
			SET_FLAG(FLAG_V, ((tmp16 ^ (uint16_t)A) & (tmp16 ^ tmp8) & 0x80) > 0);
			SET_FLAG(FLAG_Z, (A == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 6;
			PC += 2;

			break;

		case 0xe4: // CPX zeropage
			//str += "CPX $" + HEX(arg8);

			tmp8_2 = read_1B(arg8);

			tmp8 = X - tmp8_2;

			SET_FLAG(FLAG_C, X >= tmp8_2);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 3;
			PC += 2;

			break;

		case 0xe5: // SBC zeropage
			//str += "SBC $" + HEX(arg8);

			tmp8 = ~read_1B(arg8);
			tmp16 = (uint16_t)A + (uint16_t)tmp8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ tmp8) & 0x80));
			SET_FLAG(FLAG_Z, (tmp16 == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 3;
			PC += 2;

			break;

		case 0xe6: // INC zeropage
			//str += "INC $" + HEX(arg8);

			tmp8 = read_1B(arg8) + 1;
			write_1B(arg8, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 5;
			PC += 2;

			break;

		case 0xe8: // INX
			//str += "INX";

			X++;

			cycles = 2;
			PC++;

			SET_FLAG(FLAG_N, (X >> 7));
			SET_FLAG(FLAG_Z, (X == 0));

			break;

		case 0xe9: // SBC imm8
			//str += "SBC #$" + HEX(arg8);

			arg8 ^= 0xff;
			tmp16 = (uint16_t)A + (uint16_t)arg8 + GET_FLAG(FLAG_C);
			A = (uint8_t)(/*(int16_t)*/tmp16);

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ arg8) & 0x80));
			SET_FLAG(FLAG_Z, (A == 0));
			SET_FLAG(FLAG_N, (A >> 7) & 1);

			cycles = 2;
			PC += 2;

			break;

		case 0xea: // NOP
			//str += "NOP";

			PC++;
			cycles = 2;
			break;

		case 0xec: // CPX absolute
			//str += "CPX $" + HEX(arg16);

			tmp8_2 = read_1B(arg16);

			tmp8 = X - tmp8_2;

			SET_FLAG(FLAG_C, X >= tmp8_2);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 4;
			PC += 3;

			break;

		case 0xed: // SBC absolute
			//str += "SBC $" + HEX(arg16);

			tmp8 = ~read_1B(arg16);
			tmp16 = (uint16_t)A + (uint16_t)tmp8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ tmp8) & 0x80));
			SET_FLAG(FLAG_Z, (tmp16 == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 4;
			PC += 3;

			break;

		case 0xee: // INC absolute
			//str += "INC $" + HEX(arg16);

			tmp8 = read_1B(arg16) + 1;
			write_1B(arg16, tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 6;
			PC += 3;

			break;

		case 0xf0: // BEQ relative
			//str += "BEQ $" + HEX(arg8);

			cycles = 2;

			if (GET_FLAG(FLAG_Z))
			{
				cycles++;
				PC += (int8_t)arg8;
			}

			PC += 2;

			if (PCPage != (PC >> 8))
				cycles++;

			break;

		case 0xf1: // SBC (indirect), Y
			//str += "SBC ($" + HEX(arg8) + "), Y";

			tmp8 = ~readIndirectIndexedY(arg8, pageBoundaryCrossed);
			tmp16 = (uint16_t)A + (uint16_t)tmp8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ tmp8) & 0x80));
			SET_FLAG(FLAG_Z, (tmp16 == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 5 + pageBoundaryCrossed;
			PC += 2;

			break;

		case 0xf5: // SBC zeropage, X
			//str += "SBC ($" + HEX(arg8) + "), X";

			tmp8 = ~read_1B(zeropageIndexedXAddress(arg8));
			tmp16 = (uint16_t)A + (uint16_t)tmp8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ tmp8) & 0x80));
			SET_FLAG(FLAG_Z, (tmp16 == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 4;
			PC += 2;

			break;

		case 0xf6: // INC zeropage, X
			//str += "INC $" + HEX(arg8) + ", X";

			tmp8 = read_1B(zeropageIndexedXAddress(arg8)) + 1;
			write_1B(zeropageIndexedXAddress(arg8), tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 6;
			PC += 2;

			break;

		case 0xf8: // SED
			//str += "SED";

			SET_FLAG_1(FLAG_D);

			cycles = 2;
			PC++;

			break;

		case 0xf9: // SBC absolute, Y
			//str += "SBC ($" + HEX(arg16) + "), Y";

			tmp8 = ~read_1B(absoluteIndexedYAddress(arg16, pageBoundaryCrossed));
			tmp16 = (uint16_t)A + (uint16_t)tmp8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ tmp8) & 0x80));
			SET_FLAG(FLAG_Z, (tmp16 == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0xfd: // SBC absolute, X
			//str += "SBC ($" + HEX(arg16) + "), X";

			tmp8 = ~read_1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed));
			tmp16 = (uint16_t)A + (uint16_t)tmp8 + (uint16_t)GET_FLAG(FLAG_C);
			A = (uint8_t)tmp16;

			SET_FLAG(FLAG_C, (tmp16 > 0xff));
			SET_FLAG(FLAG_V, ((A ^ tmp16) & (tmp16 ^ tmp8) & 0x80));
			SET_FLAG(FLAG_Z, (tmp16 == 0));
			SET_FLAG(FLAG_N, (A >> 7));

			cycles = 4 + pageBoundaryCrossed;
			PC += 3;

			break;

		case 0xfe: // INC absolute, X
			//str += "INC $" + HEX(arg16) + ", X";

			tmp8 = read_1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed)) + 1;
			write_1B(absoluteIndexedXAddress(arg16, pageBoundaryCrossed), tmp8);

			SET_FLAG(FLAG_N, (tmp8 >> 7));
			SET_FLAG(FLAG_Z, (tmp8 == 0));

			cycles = 7;
			PC += 3;

			break;

		default:
			//str += "Illegal opcode";

			stopCPU = true;

			break;
		}
	}

	cycles--;
}