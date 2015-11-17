#pragma once
#include "IGenericVCDevice.h"
#include "ExtensionFilterBase.h"
#include "IDiscernible.h"
#include "SourceDecorator.h"

namespace TopGear
{
	namespace Win
	{
		template<class T>
		class ExtensionVCDevice : public SourceDecorator, public IDiscernible<ExtensionFilterBase>
		{
			static_assert(std::is_base_of<ExtensionFilterBase, T>::value, "Class T must derive from ExtensionFilterBase");
		public:
			virtual std::string GetDeviceInfo() override
			{
				return validator ? validator->GetDeviceInfo() : "";
			}

			virtual const std::shared_ptr<ExtensionFilterBase>& GetValidator() const override
			{
				return validator;
			}

			explicit ExtensionVCDevice(std::shared_ptr<IGenericVCDevice> &device)
				: SourceDecorator(device)
			{
				if (source != nullptr)
					validator = std::make_shared<T>(device);
			}
			virtual ~ExtensionVCDevice() {}
		protected:
			std::shared_ptr<ExtensionFilterBase> validator;
		};
	}
}