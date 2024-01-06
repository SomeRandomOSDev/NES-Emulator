#pragma once

#include <sstream>

namespace
{
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

#define GET_FLAG(bit)           ((P & (1 << bit)) >> bit)
#define SET_FLAG_1(bit)           P |= (1 << bit)
#define SET_FLAG_0(bit)         P &= ~((uint8_t)(1 << bit))
#define TOGGLE_FLAG(bit)        P ^= (1 << bit)
#define SET_FLAG(bit, value)    SET_FLAG_0(bit); P |= (value << bit)

#define FLAG_C 0
#define FLAG_Z 1
#define FLAG_I 2
#define FLAG_D 3
#define FLAG_B 4
#define FLAG_1 5
#define FLAG_V 6
#define FLAG_N 7

#define FLAG_CARRY             FLAG_C
#define FLAG_ZERO              FLAG_Z
#define FLAG_INTERRUPT_DISABLE FLAG_I
#define FLAG_DECIMAL           FLAG_D


#define FLAG_OVERFLOW          FLAG_V
#define FLAG_NEGATIVE          FLAG_N
}