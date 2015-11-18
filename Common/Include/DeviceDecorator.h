#pragma once
#include "IGenericVCDevice.h"

namespace TopGear
{
	class DeviceDecorator : public IGenericVCDevice
	{
	public:
		virtual std::shared_ptr<ISource> &GetSource() override
		{
			return component->GetSource();
		}
		virtual std::string GetSymbolicLink() override
		{
			return component->GetSymbolicLink();
		}
		virtual std::string GetFriendlyName() override
		{
			return component->GetFriendlyName();
		}
		virtual std::string GetDeviceInfo() override
		{
			return component->GetDeviceInfo();
		}
		virtual ~DeviceDecorator() = default;
	protected:
		std::shared_ptr<IGenericVCDevice> component;
		explicit DeviceDecorator(std::shared_ptr<IGenericVCDevice> &device)
			: component(device)
		{
		}
	};
}
