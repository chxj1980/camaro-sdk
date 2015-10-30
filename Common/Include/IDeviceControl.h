#pragma once

#include <cstdint>
#include <string>


namespace TopGear
{
	class IDeviceControl
	{
	public:
		virtual ~IDeviceControl() = default;

		virtual int SetSensorTrigger(uint8_t level) = 0;
		virtual int SetResyncNumber(uint16_t resyncNum) = 0;
		virtual int QueryDeviceRole() = 0;
		virtual std::string QueryDeviceInfo() = 0;
	};
}
