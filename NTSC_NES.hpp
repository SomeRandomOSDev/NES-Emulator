#pragma once

#include "PPU_RP2C02G.hpp"
#include "CPU_RP2A03.hpp"

#include "Mapper_0.hpp"

class NTSC_NES // NTSC NES with RP2A03/RP2C02G
{
public:
	NTSC_NES() : cpu(ppu, mapper, settings), ppu(mapper, settings)
	{
		powerUp();
	}

	void powerUp()
	{
		cpu.powerUp();
		ppu.powerUp();

		log.push_back("POWER UP");
	}

	void reset()
	{
		cpu.reset();
		ppu.reset();

		log.push_back("RESET");
	}

	void loadFromiNES(std::string fileName)
	{
		log.push_back("Loading \"" + fileName + "\"...");

		std::ifstream f(fileName, std::ios::binary);
		if (!f) return;
		std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(f), {});
		f.close();

		iNESHeader header;
		memcpy(&header, &buffer[0], /*sizeof(header)*/16 * sizeof(uint8_t));

		uint8_t mapper_lo = (header.flags6 >> 4), mapper_hi = (header.flags7 >> 4);
		uint8_t mapperNb = (mapper_lo | (mapper_hi << 4));

		bool trainer = (header.flags6 >> 2) & 1;

		//PRGRAM = (header.flags6 >> 1) & 1;

		uint32_t PRGROM_location = 16 + (512 * trainer), PRGROM_size = 16384 * header.PRGROMSize;
		uint32_t CHRROM_location = PRGROM_location + PRGROM_size, CHRROM_size = 8192 * header.CHRROMSize;

		cpu.stopCPU = false;
		//mapper = Other;
		mapper = std::make_shared<Mapper>();
		switch (mapperNb)
		{
		case 0:
		{
			Mapper_0 mapper0(header.PRGROMSize);

			mapper0.mirroring = Mirroring(header.flags6 & 1);

			memcpy(&mapper0.PRGROM_lo, &buffer[PRGROM_location], 16 * KB);
			if(header.PRGROMSize == 2)
				memcpy(&mapper0.PRGROM_hi, &buffer[PRGROM_location + 16 * KB], 16 * KB);
			memcpy(&mapper0.CHRROM[0],  &buffer[CHRROM_location], 8 * KB);

			log.push_back("Loaded successfully : ");
			log.push_back("mapper 0 | " + std::string(mapper0.mirroring == Horizontal ? "Horizontal" : "Vertical") + " mirroring.");

			mapper = std::make_shared<Mapper_0>(mapper0);

			break;
		}

		default:
			log.push_back("Unknown mapper (mapper " + std::to_string(mapperNb) + ").");
			cpu.stopCPU = true;
			break;
		}

		cpu.PC = cpu.read_2B(RESET_VECTOR);
	}

	void cycle()
	{
		ppu.cycle();

		if (ppu.waitForNMI)
		{
			cpu.NMI();
			ppu.waitForNMI = false;
		}

		if((ppu.totalCycles % 3) == 0 && !cpu.stopCPU)
			cpu.cycle();
	}

	void frame()
	{
		while (!ppu.frameFinished)
			//for (uint32_t i = 0; i < 5369318; i++)
			cycle();

		ppu.frameFinished = false;
	}

public:
	CPU_RP2A03 cpu;
	PPU_RP2C02G ppu;

	std::shared_ptr<Mapper> mapper;

	std::vector<std::string> log;
	EmulationSettings settings;
};