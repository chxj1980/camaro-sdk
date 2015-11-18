#pragma once
#include "StandardUVCFilter.h"
#include "IDiscernible.h"
#include "DeviceDecorator.h"

namespace TopGear
{
	class StandardVCDevice : public DeviceDecorator, public IDiscernible<StandardUVCFilter>
	{
	public:
		virtual const std::shared_ptr<StandardUVCFilter> &GetValidator() const override { return validator; }
		explicit StandardVCDevice(std::shared_ptr<IGenericVCDevice> &device)
			: DeviceDecorator(device)
		{
			if (component != nullptr)
				validator = std::make_shared<StandardUVCFilter>(device);
		}
		virtual ~StandardVCDevice() {}
	protected:
		std::shared_ptr<StandardUVCFilter> validator;
	};
}