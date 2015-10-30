#pragma once

#include <cstdint>

namespace TopGear
{
	class ICameraControl
	{
	public:
		virtual ~ICameraControl() = default; // make dtor virtual

		virtual int Flip(bool vertical, bool horizontal) = 0;
		virtual int GetExposure(uint16_t &val) = 0;
		virtual int SetExposure(uint16_t val) = 0;
		virtual int GetGain(uint16_t &gainR, uint16_t &gainG, uint16_t &gainB) = 0;
		virtual int SetGain(uint16_t gainR, uint16_t gainG, uint16_t gainB) = 0;
	};
}
