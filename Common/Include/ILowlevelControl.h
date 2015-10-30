#pragma once

#include <cstdint>

namespace TopGear
{
	class ILowlevelControl
	{
	public:
		virtual ~ILowlevelControl() = default; // make dtor virtual

		virtual int SetRegisters(uint16_t regaddr[], uint16_t regval[], int num) = 0;
		virtual int GetRegisters(uint16_t regaddr[], uint16_t regval[], int num) = 0;
		//single register
		virtual int SetRegister(uint16_t regaddr, uint16_t regval) = 0;
		virtual int GetRegister(uint16_t regaddr, uint16_t &regval) = 0;
	};
}