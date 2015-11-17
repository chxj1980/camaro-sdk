#pragma once
#include "IGenericVCDevice.h"
#include "IValidation.h"

namespace TopGear
{
	namespace Win
	{
		class StandardUVCFilter : public IValidation
		{
		public:
			virtual bool IsValid() const override { return isValid; }
			explicit StandardUVCFilter(std::shared_ptr<IGenericVCDevice> &device);
		private:
			bool isValid;
		};
	}
}

