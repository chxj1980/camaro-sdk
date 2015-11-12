#pragma once
#include "IGenericVCDevice.h"
#include "ExtensionFilterBase.h"
#include "IMSource.h"

namespace TopGear
{
	namespace Win
	{
		typedef IDiscernibleVCDevice<ExtensionFilterBase> IExtensionDevice;

		template<class T>
		class ExtensionVCDevice : public IExtensionDevice, public IMSource
		{
			static_assert(std::is_base_of<ExtensionFilterBase, T>::value, "Class T must derive from ExtensionFilterBase");
		public:
			virtual IMFActivate* GetActivator() override
			{
				return source ? source->GetActivator() : nullptr;
			}
			virtual IMFMediaSource* GetSource() override
			{
				return source ? source->GetSource() : nullptr;
			}
			virtual std::string GetSymbolicLink() override
			{
				return genericDevice->GetSymbolicLink();
			}
			virtual std::string GetFriendlyName() override
			{
				return genericDevice->GetFriendlyName();
			}
			virtual std::string GetDeviceInfo() override
			{
				return validator->GetDeviceInfo();
			}

			virtual const std::shared_ptr<ExtensionFilterBase>& GetValidator() const override
			{
				return validator;
			}

			explicit ExtensionVCDevice(std::shared_ptr<IGenericVCDevice> &device)
				: genericDevice(device)
			{
				source = std::dynamic_pointer_cast<IMSource>(genericDevice);
				validator = std::make_shared<T>(source->GetSource());
			}
			virtual ~ExtensionVCDevice() {}
		protected:
			std::shared_ptr<IGenericVCDevice> genericDevice;
			std::shared_ptr<IMSource> source;
			std::shared_ptr<ExtensionFilterBase> validator;
		};
	}
}