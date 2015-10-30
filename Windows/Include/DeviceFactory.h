#pragma once
#include <vector>
#include <chrono>
#include "IGenericVCDevice.h"
#include "IDeviceFactory.h"

namespace TopGear
{
	namespace Win
	{
		template<class T>
		class DeviceFactory : public IDeviceFactory<T>
		{
		public:
			static std::vector<IGenericVCDeviceRef> EnumerateDevices();
		private:
			static const std::chrono::milliseconds InitialTime;
			DeviceFactory() = default;
		protected:
			~DeviceFactory() = default;
		};
	}
}


