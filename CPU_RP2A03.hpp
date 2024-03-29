#pragma once

#include "util.hpp"

#include "PPU_RP2C02G.hpp"
#include "APU_RP2A03.hpp"

class CPU_RP2A03
{
public:
	CPU_RP2A03(PPU_RP2C02G& _ppu, APU_RP2A03& _apu, std::shared_ptr<Mapper>& _mapper, EmulationSettings& _settings) : 
	ppu(_ppu), apu(_apu), mapper(_mapper), settings(_settings)
	{
		powerUp();
	}

	void powerUp()
	{
		stopCPU = true;

		A = X = Y = 0;
		SR = 0x34;
		S = 0xfd;
		write_1B(0x4017, 0);
		write_1B(0x4015, 0);
		for (uint16_t i = 0x4000; i <= 0x400f; i++)
			write_1B(i, 0);
		for (uint16_t i = 0x4010; i <= 0x4013; i++)
			write_1B(i, 0);

		for (uint16_t i = 0x000; i < 0x800; i++)
			write_1B(i, randomByte(re));

		cycles = 0;

		controllerLatch1 = false;
	}

	void reset()
	{
		stopCPU = true;

		S -= 3;
		//SR |= 0x04;
		SET_FLAG_1(FLAG_INTERRUPT_DISABLE);
		cycles = 9;
		PC = read_1B(RESET_VECTOR);

		controllerLatch1 = false;
	}

	void write_1B(uint16_t address, uint8_t value)
	{
		if (address < 0x2000)
		{
			memory[address % 0x800] = value;
			return;
		}
		else if (address < 0x4000) // PPU registers
		{
			switch (address % 8)
			{
			case 0x0000: // PPU CTRL
				ppu.reg_ppu_control = value;

				LOOPY_SET_NAMETABLE(ppu.t,
				REG_GET_FLAG(ppu.reg_ppu_control, PPU_CTRL_NAMETABLE_ADDRESS_Y) * 2 +
				REG_GET_FLAG(ppu.reg_ppu_control, PPU_CTRL_NAMETABLE_ADDRESS_X));

				break;

			case 0x0001: // PPU MASK
				ppu.reg_ppu_mask = value;

				break;

			case 0x0003:
				ppu.OAMAddress = value;

				break;

			case 0x0004:
				*ppu.OAMGetByte(ppu.OAMAddress) = value;
				ppu.OAMAddress++;

				break;

			case 0x0005: // PPU SCROLL
				if (ppu.w == 0) // X
				{
					ppu.x = (value & 0b111);
					LOOPY_SET_COARSE_X(ppu.t, (value >> 3));
				}
				else        // Y
				{
					LOOPY_SET_FINE_Y(ppu.t, (value & 0b111));
					LOOPY_SET_COARSE_Y(ppu.t, (value >> 3));
				}

				ppu.w ^= true;

				break;

			case 0x0006: // PPU ADDRESS
				if (ppu.w == 0)
				{
					ppu.t &= 0x00ff;
					ppu.t |= ((uint16_t)value & 0b01111111) << 8;
				}
				else
				{
					ppu.t &= 0xff00;
					ppu.t |= value;
					ppu.v = ppu.t;
				}

				ppu.w ^= true;

				break;

			case 0x0007: // PPU DATA
				ppu.write_1B(ppu.v, value);

				ppu.v += REG_GET_FLAG(ppu.reg_ppu_control, PPU_CTRL_VRAM_ADDRESS_INCREMENT_DIRECTION) ? 32 : 1;

				break;
			}

			return;
		}
		else if (address < 0x4018) // APU and IO registers
		{
			switch (address)
			{
			case 0x4000:
			{
				uint8_t duty = (value >> 6);
				float volume = (value & 0x0f) / 15.f;
				bool constantVolume = (value >> 4) & 1;
				bool loop = (value >> 5) & 1;

				if (!constantVolume)
					volume = 1;

				apu.pulse_1.volume = volume;
				apu.pulse_1.constantVolume = constantVolume;
				apu.pulse_1.envelopeLoop = loop;

				if (duty == 0)
					apu.pulse_1.duty = 0.125f;
				else
					apu.pulse_1.duty = 0.25f * duty;

				break;
			}

			case 0x4001:
			{
				bool enabled = (value >> 7);
				uint8_t sweepPeriod = ((value >> 4) & 0b111) + 1;
				bool negate = (value >> 3) & 1;
				uint8_t shiftCount = value & 0b111;

				apu.pulse_1.sweepEnabled = enabled;
				apu.pulse_1.sweepPeriod = sweepPeriod;
				apu.pulse_1.sweepShiftCount = shiftCount;
				apu.pulse_1.sweepNegate = negate;

				break;
			}

			case 0x4002:
				apu.pulse_1.t &= 0x0700;
				apu.pulse_1.t |= value;

				break;

			case 0x4003:
				apu.pulse_1.t &= 0x00ff;
				apu.pulse_1.t |= ((value & 0b111) << 8);

				apu.pulse_1.lengthCounter = APU_LengthLookup[value >> 3];
				apu.pulse_1.volume = 1;

				break;

			case 0x4004:
			{
				uint8_t duty = (value >> 6);
				float volume = (value & 0x0f) / 15.f;
				bool constantVolume = (value >> 4) & 1;
				bool loop = (value >> 5) & 1;

				if (!constantVolume)
					volume = 1;

				apu.pulse_2.volume = volume;
				apu.pulse_2.constantVolume = constantVolume;
				apu.pulse_2.envelopeLoop = loop;

				if (duty == 0)
					apu.pulse_2.duty = 0.125f;
				else
					apu.pulse_2.duty = 0.25f * duty;

				break;
			}

			case 0x4005:
			{
				bool enabled = (value >> 7);
				uint8_t sweepPeriod = ((value >> 4) & 0b111) + 1;
				bool negate = (value >> 3) & 1;
				uint8_t shiftCount = value & 0b111;

				apu.pulse_2.sweepEnabled = enabled;
				apu.pulse_2.sweepPeriod = sweepPeriod;
				apu.pulse_2.sweepShiftCount = shiftCount;
				apu.pulse_2.sweepNegate = negate;

				break;
			}

			case 0x4006:
				apu.pulse_2.t &= 0x0700;
				apu.pulse_2.t |= value;

				break;

			case 0x4007:
				apu.pulse_2.t &= 0x00ff;
				apu.pulse_2.t |= ((value & 0b111) << 8);

				apu.pulse_2.lengthCounter = APU_LengthLookup[value >> 3];
				apu.pulse_2.volume = 1;

				break;

			case 0x4014:	// OAM DMA
				for (unsigned int i = 0; i < 256; i++)
				{
					uint16_t dmaAddress = ((uint16_t)value << 8) | i;

					*ppu.OAMGetByte(ppu.OAMAddress + i) = read_1B(dmaAddress);
				}

				//cycles = 513; // or 514
				cycles += 512;

				break;

			case 0x4015:
			{
				bool DMC = (value >> 4) & 1;
				bool noise = (value >> 3) & 1;
				bool triangle = (value >> 2) & 1;
				bool pulse_2 = (value >> 1) & 1;
				bool pulse_1 = value & 1;

				apu.pulse_1.enable = pulse_1;
				apu.pulse_2.enable = pulse_2;

				break;
			}

			case 0x4016:
				controllerLatch1 = (value & 1);

				break;

			case 0x4017:
				apu.frameCounterMode = (value >> 7);
				apu.interruptInhibit = (value >> 6) & 1;

				break;

			default:
				break;
			}

			return;
		}
		else if (address < 0x4020) // APU and I/O functionality that is normally disabled
			return;
		else
		{
			mapper->CPU_write_1B(address, value);
			return;
		}
	}

	uint8_t read_1B(uint16_t address)
	{
		if (address < 0x2000)
			return memory[address % 0x800];
		else if (address < 0x4000) // PPU registers
		{
			uint8_t regNb = address % 8;

			switch (regNb)
			{
			case 0x0002: // PPU STATUS
			{
				ppu.w = false;

				uint8_t value = ppu.reg_ppu_status;

				REG_SET_FLAG_0(ppu.reg_ppu_status, PPU_STATUS_VBLANK);

				return value;
			}

			case 0x0004:
				return *ppu.OAMGetByte(ppu.OAMAddress);

			case 0x0007:	// PPU DATA
				if (address < 0x3f00)
				{
					uint8_t value = ppu.readBuffer;
					ppu.readBuffer = ppu.read_1B(ppu.v);
					ppu.v += REG_GET_FLAG(ppu.reg_ppu_control, PPU_CTRL_VRAM_ADDRESS_INCREMENT_DIRECTION) ? 32 : 1;
					return value;
				}
				else
				{
					ppu.readBuffer = ppu.read_1B(ppu.v);
					ppu.v += REG_GET_FLAG(ppu.reg_ppu_control, PPU_CTRL_VRAM_ADDRESS_INCREMENT_DIRECTION) ? 32 : 1;
					return ppu.readBuffer;
				}
			}

			return 0;
		}
		else if (address < 0x4018) // APU and IO registers
		{
			switch (address)
			{
			case 0x4015:
				return (apu.frameInterrupt << 6) | ((apu.pulse_2.lengthCounter > 0) << 1) | (apu.pulse_1.lengthCounter > 0);

			case 0x4016:
			{
				uint8_t value = (controller1ShiftRegister & 1);
				controller1ShiftRegister >>= 1;
				return value;
			}

			default:

				return 0;
			}
		}
		else if (address < 0x4020) // APU and I/O functionality that is normally disabled
			return 0;
		else
			return mapper->CPU_read_1B(address);
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

	void push_1B(uint8_t value)
	{
		write_1B(S + 0x100, value);
		S--;
	}

	uint8_t pull_1B()
	{
		S++;
		return read_1B(S + 0x100);
	}

	void push_2B(uint16_t value)
	{
		push_1B(value >> 8);
		push_1B(value & 0xff);
	}

	uint16_t pull_2B()
	{
		uint8_t lo = pull_1B();
		uint8_t hi = pull_1B();

		return (((uint16_t)hi << 8) | lo);
	}

	void NMI()
	{
		if (REG_GET_FLAG(ppu.reg_ppu_control, PPU_CTRL_NMI) && !stopCPU)
		{
			push_2B(PC);
			REG_SET_FLAG_0(SR, FLAG_B);
			REG_SET_FLAG_1(SR, FLAG_I);
			push_1B(SR);

			PC = read_2B(NMI_VECTOR);

			cycles = 8;
		}
	}

	void IRQ()
	{
		if (!GET_FLAG(FLAG_I) && !stopCPU)
		{
			push_2B(PC);
			REG_SET_FLAG_1(SR, FLAG_I);
			push_1B(SR);

			PC = read_2B(IRQ_VECTOR);

			cycles = 7;
		}
	}

	void INTERRUPT(uint16_t returnAddress, uint16_t isrAddress, bool B_FLAG)
	{
		if (GET_FLAG(FLAG_I))
		{
			PC = returnAddress;
		}
		else
		{
			push_2B(returnAddress);
			SET_FLAG_1(FLAG_I);
			push_1B((SR & (uint8_t)(~(1 << FLAG_B))) | (B_FLAG << FLAG_B));

			PC = read_2B(isrAddress);
		}
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
		return read_1B((d + X) & 0xff) + ((uint16_t)read_1B((d + X + 1) & 0xff) << 8);
	}

	uint16_t indirectIndexedYAddress(uint8_t d)
	{
		/*uint16_t lo = read_1B(d) + Y;
		pageBoundaryCrossed = lo > 0xff;
		return lo + (read_1B((d + 1) % 256) * 256);*/
		return read_1B(d) + Y + ((uint16_t)read_1B((d + 1) & 0xff) << 8);
	}

	uint8_t readIndirectIndexedY(uint8_t d, bool& pageBoundaryCrossed)
	{
		uint16_t lo = read_1B(d);
		pageBoundaryCrossed = lo > 0xff;
		return read_1B(lo + ((uint16_t)read_1B((d + 1) % 256) << 8) + Y);
	}

	void HandleInput()
	{
		if (controllerLatch1)
		{
			for (unsigned int i = 0; i < 8; i++)
			{
				controller1ShiftRegister &= ~(1 << i);
				controller1ShiftRegister |= ((sf::Keyboard::isKeyPressed(keyMap[7 - i])) << i);
			}

			if (settings.blockImpossibleInputs)
			{
				if (REG_GET_FLAG(controller1ShiftRegister, 4) && REG_GET_FLAG(controller1ShiftRegister, 5))
				{
					REG_SET_FLAG_0(controller1ShiftRegister, 4);
					REG_SET_FLAG_0(controller1ShiftRegister, 5);
				}
				if (REG_GET_FLAG(controller1ShiftRegister, 7) && REG_GET_FLAG(controller1ShiftRegister, 6))
				{
					REG_SET_FLAG_0(controller1ShiftRegister, 7);
					REG_SET_FLAG_0(controller1ShiftRegister, 6);
				}
			}
		}
	}

	void cycle();

public:
	uint8_t memory[8 * KB];

	uint8_t A, X, Y, SR, S;
	uint16_t PC;
	uint16_t cycles;

	bool controllerLatch1;
	uint8_t controller1ShiftRegister;

	bool stopCPU;

	APU_RP2A03& apu;
	PPU_RP2C02G& ppu;
	std::shared_ptr<Mapper>& mapper;

	EmulationSettings& settings;
};