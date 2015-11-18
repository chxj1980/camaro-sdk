#pragma once
#include "IGenericVCDevice.h"
#include "IExtensionLite.h"
#include "IDiscernible.h"
#include "DeviceDecorator.h"

namespace TopGear
{
	template<class T>
	class ExtensionVCDevice : public DeviceDecorator, public IDiscernible<IExtensionLite>
	{
		static_assert(std::is_base_of<IExtensionLite, T>::value, "Class T must derive from IExtensionLite");
	public:
		virtual std::string GetDeviceInfo() override
		{
			return validator ? validator->GetDeviceInfo() : "";
		}

		virtual const std::shared_ptr<IExtensionLite>& GetValidator() const override
		{
			return validator;
		}

		explicit ExtensionVCDevice(std::shared_ptr<IGenericVCDevice> &device)
			: DeviceDecorator(device)
		{
			if (component != nullptr)
				validator = std::make_shared<T>(device);
		}
		virtual ~ExtensionVCDevice() {}
	protected:
		std::shared_ptr<IExtensionLite> validator;
	};
}