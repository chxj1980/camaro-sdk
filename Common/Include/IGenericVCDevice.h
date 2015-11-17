#pragma once
#include <memory>
#include <string>

namespace TopGear
{
	class IGenericVCDevice
	{
	public:
		virtual std::string GetSymbolicLink() = 0;
		virtual std::string GetFriendlyName() = 0;
		virtual std::string GetDeviceInfo() = 0;
		virtual ~IGenericVCDevice() = default;
	};

	typedef std::shared_ptr<IGenericVCDevice> IGenericVCDevicePtr;
}
