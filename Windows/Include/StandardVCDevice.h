#pragma once
#include "StandardUVCFilter.h"
#include "IDiscernible.h"
#include "SourceDecorator.h"

namespace TopGear
{
	namespace Win
	{
		class StandardVCDevice : public SourceDecorator, public IDiscernible<StandardUVCFilter>
		{
		public:
			virtual const std::shared_ptr<StandardUVCFilter> &GetValidator() const override { return validator; }
			explicit StandardVCDevice(std::shared_ptr<IGenericVCDevice> &device)
				: SourceDecorator(device)
			{
				if (source != nullptr)
					validator = std::make_shared<StandardUVCFilter>(device);
			}
			virtual ~StandardVCDevice() {}
		protected:
			std::shared_ptr<StandardUVCFilter> validator;
		};
	}
}