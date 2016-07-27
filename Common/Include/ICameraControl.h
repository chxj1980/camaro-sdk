#pragma once

#include <cstdint>

namespace TopGear
{
	class ICameraControl
	{
	public:
		virtual ~ICameraControl() = default; // make dtor virtual
        
		virtual int Flip(bool vertical, bool horizontal) = 0;

        //Shutter control in micro-seconds
        //
        virtual int GetShutter(uint32_t &val) = 0;
        virtual int SetShutter(uint32_t val) = 0;

        //Exposure control in EV value, specially 0 for manual exposure
        //Some camera could be unsupported
        virtual int GetExposure(bool &ae, float &ev) = 0;
        virtual int SetExposure(bool ae, float ev = 1.0f) = 0;

        virtual int GetIris(float &ratio) = 0;
        virtual int SetIris(float ratio) = 0;

        virtual int GetGain(float &gainR, float &gainG, float &gainB) = 0;
        virtual int SetGain(float gainR, float gainG, float gainB) = 0;
	};
}
