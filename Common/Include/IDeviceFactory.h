#pragma once
#include "IGenericVCDevice.h"
#include <vector>

namespace TopGear
{
	template<class T>
	class IDeviceFactory
	{
		static_assert(std::is_base_of<IGenericVCDevice, T>::value,
			"Class T must derive from IGenericVCDevice");
	public:
		static std::vector<IGenericVCDevicePtr> EnumerateDevices() = delete;
	protected:
		IDeviceFactory() = default;
		~IDeviceFactory() = default;
	};
}
