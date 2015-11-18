#pragma once
#include <vector>
#include "IGenericVCDevice.h"

namespace TopGear
{
	class GenericVCDevice : public IGenericVCDevice
	{
	public:
		virtual std::shared_ptr<ISource>& GetSource() override { return source; }
		virtual std::string GetSymbolicLink() override;
		virtual std::string GetFriendlyName() override;
		virtual std::string GetDeviceInfo() override { return ""; }
		explicit GenericVCDevice(std::shared_ptr<ISource> &vsource);
		virtual ~GenericVCDevice();
	protected:
		std::shared_ptr<ISource> source;
		std::string symbolicLink;
		std::string name;
	};
}

