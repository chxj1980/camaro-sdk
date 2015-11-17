#pragma once
#include "IMSource.h"

namespace TopGear
{
	namespace Win
	{
		class SourceDecorator : public IMSource
		{
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
				return source ? source->GetSymbolicLink() : "";
			}
			virtual std::string GetFriendlyName() override
			{
				return source ? source->GetFriendlyName() : "";
			}
			virtual std::string GetDeviceInfo() override
			{
				return source ? source->GetDeviceInfo() : "";
			}
			virtual ~SourceDecorator() = default;
		protected:
			std::shared_ptr<IMSource> source;

			explicit SourceDecorator(std::shared_ptr<IGenericVCDevice> &device)
			{
				source = std::dynamic_pointer_cast<IMSource>(device);
			}
		};
	}
}
